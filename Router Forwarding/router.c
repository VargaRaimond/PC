#include "rtable.h"

	struct arp_entry * arp_table;
	int arp_table_len;


struct arp_entry *get_arp_entry(__u32 ip) {
    /* TODO 2: Implement */
    for(int i = 0; i < arp_table_len; i++) {
    	if(arp_table[i].ip == ip) {
    		return &arp_table[i];
    	}
    }
    return NULL;
}


void echo_reply(packet *pkt) {

	// extract all headers
	struct ether_header *eth_hdr = (struct ether_header *)pkt->payload;
	struct iphdr *ip_hdr = (struct iphdr *)(pkt->payload + sizeof(struct ether_header));
	struct icmphdr *icmp_hdr = (struct icmphdr *)(pkt->payload + sizeof(struct ether_header) + sizeof(struct iphdr));
 	
 	//printf("LEN:%ld\n", (sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr)));
 	int interface = pkt->interface;
	// prepare ttl
	ip_hdr->ttl = 64;

	// switch source with destination to reply
	ip_hdr->daddr = ip_hdr ->saddr;
	ip_hdr->saddr = inet_addr(get_interface_ip(interface));

	// recalculate ip checksum
	ip_hdr->check = 0;
	ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));

	// change type to reply
	icmp_hdr->type = ICMP_ECHOREPLY;
	icmp_hdr->code = 0;

	// recalculate icmp checksum
	icmp_hdr->checksum = 0;
	icmp_hdr->checksum = ip_checksum(icmp_hdr, (pkt->len - sizeof(struct ether_header) - sizeof(struct iphdr)));

	// prepare ethernet header
	struct arp_entry *match = get_arp_entry(ip_hdr->daddr);
	memcpy(eth_hdr -> ether_dhost, match -> mac, 6);

	uint8_t *mac = malloc(6 * sizeof(uint8_t));
	get_interface_mac(interface, mac);
	memcpy(eth_hdr -> ether_shost, mac, 6);

	send_packet(interface, pkt);

	}

 

void icmp_response(packet *pkt, int type, int code) {
	// extract all headers
	struct ether_header *eth_hdr = (struct ether_header *)pkt->payload;
	struct iphdr *ip_hdr = (struct iphdr *)(pkt->payload + sizeof(struct ether_header));
	struct icmphdr *icmp_hdr = (struct icmphdr *)(pkt->payload + sizeof(struct ether_header) + sizeof(struct iphdr));
	/* I found that these packets should include the ip header and 8 more bytes after icmp header
	so the host that gets this reply can find a link between packets*/
	//memcpy((icmp_hdr + sizeof(struct icmphdr)), ip_hdr, (sizeof(struct iphdr) + 8));
 	
 	pkt->len = sizeof(struct ether_header) + sizeof(struct iphdr)
		+ sizeof(struct icmphdr);
 	//printf("LEN:%ld\n", (sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr)));
 	int interface = pkt->interface;
	// prepare ttl
	ip_hdr->ttl = 64;
	ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr));
	ip_hdr->protocol = IPPROTO_ICMP;
	ip_hdr->version = 4;
	ip_hdr-> ihl = 5;

	// switch source with destination to reply
	uint32_t aux = ip_hdr->daddr;
	ip_hdr->daddr = ip_hdr ->saddr;
	ip_hdr->saddr = aux;

	// recalculate ip checksum
	ip_hdr->check = 0;
	ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));

	// change type to reply
	icmp_hdr->type = type;
	icmp_hdr->code = code;

	// recalculate icmp checksum
	icmp_hdr->checksum = 0;
	icmp_hdr->checksum = ip_checksum(icmp_hdr, sizeof(struct icmphdr));

	// prepare ethernet header
	eth_hdr->ether_type = htons(ETHERTYPE_IP);
	struct arp_entry *match = get_arp_entry(ip_hdr->daddr);
	memcpy(eth_hdr -> ether_dhost, match -> mac, 6);

	uint8_t *mac = malloc(6 * sizeof(uint8_t));
	get_interface_mac(interface, mac);
	memcpy(eth_hdr -> ether_shost, mac, 6);

	send_packet(interface, pkt);
} 

int main(int argc, char *argv[]) {	
	packet m;
	int rc;

	init();

	// root of trie which represents routing table
	struct TrieNode* root = getNode();
	read_rtable(&root);

	arp_table = malloc(100 * sizeof(struct arp_entry));
	parse_arp_table();

	while (1) {
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");
		/* Students will write code here */
		struct ether_header *eth_head = (struct ether_header*)(m.payload);

		// check packet type
		if(ntohs(eth_head->ether_type) == ETHERTYPE_IP) {
			struct iphdr *ip_hdr = (struct iphdr*)(m.payload + sizeof(struct ether_header));
 
			// check checksum
			if(ip_checksum(ip_hdr, sizeof(struct iphdr))) {
				continue;
			}
			// check ttl
			if(ip_hdr-> ttl <= 1) {
				icmp_response(&m, ICMP_TIMXCEED, 0);
				continue;
			}
			/* check if the packet is for one of router interfaces and then
			check if it's icmp request to prepare a reply*/
			if(ip_hdr -> daddr == inet_addr(get_interface_ip(m.interface))) {
					struct icmphdr *icmp_hdr = (struct icmphdr *)(m.payload + sizeof(struct ether_header) + sizeof(struct iphdr));
					if(icmp_hdr -> type == ICMP_ECHO) {
						echo_reply(&m);
					}
					continue;
				}


			// find best route
			struct route_table_entry *best_route = get_best_route(ntohl(ip_hdr -> daddr), root);
			if(best_route == NULL) {
				icmp_response(&m, ICMP_DEST_UNREACH, 0);
				continue;
			}
			// change ttl and checksum
			ip_hdr -> ttl--;
			ip_hdr -> check = 0;
			ip_hdr -> check = ip_checksum(ip_hdr, sizeof(struct iphdr));

			/* TODO 7: Find matching ARP entry and update Ethernet addresses */
			struct arp_entry *match = get_arp_entry(ip_hdr->daddr);
			memcpy(eth_head -> ether_dhost, match -> mac, 6);

			uint8_t *mac = malloc(6 * sizeof(uint8_t));
			get_interface_mac(best_route -> interface, mac);
			memcpy(eth_head -> ether_shost, mac, 6);

			/* TODO 8: Forward the pachet to best_route->interface */
			send_packet(best_route -> interface, &m);

		}

		if(ntohs(eth_head->ether_type) == ETHERTYPE_ARP) {
			struct arphdr *arp_hdr = (struct arphdr*)(m.payload + sizeof(struct ether_header));

			// ARP request
			if(arp_hdr->ar_op == ARP_REQUEST) {

			} 

			// ARP reply
			if(arp_hdr->ar_op == ARP_REPLY) {

			}
		}
	}
	free_all(root);
	free(arp_table);
	return 0;
}
