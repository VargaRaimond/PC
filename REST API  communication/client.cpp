// Copyright Varga Raimond 2020
#include "helpers.h"
#include "requests.h"

char* extract_session_cookie(char* response) {
    // find connect session cookie
    char* ptr = strstr(response, "connect.sid");
    // remove everything after it
    ptr = strtok(ptr, ";");
    return ptr;
}

// function that checks if a string contains only numbers
bool is_positive_number(string s) {
    for (auto it = s.begin(); it != s.end(); it++) {
        if (!isdigit(*it)) {
            return false;
        }
    }
    return true;
}

char* register_or_login_client(char* server_name, int sockfd, string command) {
    json login;
    string buffer;
    // add command to use same function for register and login
    string path = "/api/v1/tema/auth/" + command;
    string type = "application/json";
    // interaction with client - get username and password and build json
    printf("Enter username: ");
    getline(cin, buffer);
    login["username"] = buffer;

    printf("Enter password: ");
    getline(cin, buffer);
    login["password"] = buffer;
    // create message and send to server
    char* message = compute_post_request(server_name, path.c_str(), type.c_str(),
                            login, 2, NULL, 0, NULL);
    send_to_server(sockfd, message);
    // get response from server and check if we got any error
    char* response = receive_from_server(sockfd);
    if (basic_extract_json_response(response) != NULL) {
        // extract and print error
        json error = json::parse(basic_extract_json_response(response));
        printf("%s\n", error["error"].get<string>().c_str());
    } else {
        // print success messages and return cookies if it's login
        if (command == "register") {
            printf("User successfully registered!\n");
        }
        if (command == "login") {
            printf("Welcome back, %s!\n",
                login["username"].get<string>().c_str());
            return extract_session_cookie(response);
        }
    }
    free(message);
    free(response);
    return NULL;
}

void add_book(char* server_name, int sockfd, char* auth) {
    json book;
    string buffer;
    string path = "/api/v1/tema/library/books";
    string type = "application/json";

    // interaction with client - build json
    printf("Enter title: ");
    getline(cin, buffer);
    book["title"] = buffer;

    printf("Enter author: ");
    getline(cin, buffer);
    book["author"] = buffer;

    printf("Enter genre: ");
    getline(cin, buffer);
    book["genre"] = buffer;

    printf("Enter page_count: ");
    getline(cin, buffer);
    while(!is_positive_number(buffer)) {
        printf("Page count needs to be a positive number!\nEnter page_count: ");
        getline(cin, buffer);
    }
    book["page_count"] = buffer;

    printf("Enter publisher: ");
    getline(cin, buffer);
    book["publisher"] = buffer;
    // create message and send to server
    char* message = compute_post_request(server_name, path.c_str(),
                                type.c_str(), book, 5, NULL, 0, auth);
    send_to_server(sockfd, message);
    // get response from server and check if we got any error
    char* response = receive_from_server(sockfd);
    if (basic_extract_json_response(response) != NULL) {
        // extract and print error
        json error = json::parse(basic_extract_json_response(response));
        printf("%s\n", error["error"].get<string>().c_str());
         // explain to client that access to library is needed for current action
        if (auth == NULL) {
            printf("Get access to library and try again!\n");
        }
    } else {
        // print success messages and return cookies if it's login
        printf("You added the book %s!\n", book["title"].get<string>().c_str());
    }
    free(message);
    free(response);
}

void logout_client(char* server_name, int sockfd, char* session_cookie) {
    string path = "/api/v1/tema/auth/logout";
    char* message;
    if (session_cookie != NULL) {
        message = compute_request("GET", server_name, path.c_str(),
                    NULL, &session_cookie, 1, NULL);
    } else {
        message = compute_request("GET", server_name, path.c_str(),
                    NULL, NULL, 0, NULL);
    }
    send_to_server(sockfd, message);
    char* response = receive_from_server(sockfd);
    if (basic_extract_json_response(response) != NULL) {
        // extract and print error
        json error = json::parse(basic_extract_json_response(response));
        printf("%s\n", error["error"].get<string>().c_str());
    } else {
        // announce client that he logged out
        printf("You logged out!\n");
    }
    free(response);
    free(message);
}

char* enter_library(char* server_name, int sockfd, char* session_cookie) {
    char* message;
    string path = "/api/v1/tema/library/access";
    // compute message with or without session cookie
    if (session_cookie != NULL) {
        message = compute_request("GET", server_name, path.c_str(),
                    NULL, &session_cookie, 1, NULL);
    } else {
        message = compute_request("GET", server_name, path.c_str(),
                    NULL, NULL, 0, NULL);
    }
    send_to_server(sockfd, message);
    // extract only json from response
    json response = json::parse(basic_extract_json_response(receive_from_server(sockfd)));
    // print error or return the jwt token we received
    if (response.contains("error")) {
        printf("%s\n", response["error"].get<string>().c_str());
    } else {
        printf("Access to library was granted!\n");
        return strdup(response["token"].get<string>().c_str());
    }

    free(message);
    return NULL;
}

void get_books_summary(char* server_name, int sockfd, char* auth) {
    string path = "/api/v1/tema/library/books";
    char* message = compute_request("GET", server_name, path.c_str(),
                        NULL, NULL, 0, auth);
    send_to_server(sockfd, message);
    // get response from server and check if we got any error
    char* response = receive_from_server(sockfd);
    if (basic_extract_json_list(response) == NULL) {
        json error = json::parse(basic_extract_json_response(response));
        printf("%s\n", error["error"].get<string>().c_str());
        // explain to client that access to library is needed for current action
        if (auth == NULL) {
            printf("Get access to library and try again!\n");
        }
    } else {
        // check if we have any books and print basic info about them
        json books = json::parse(basic_extract_json_list(response));
        if (books.size() == 0) {
            printf("Currently you have 0 books!\n");
        } else {
            printf("Currently you have %lu books:\n", books.size());
            for (json::iterator it = books.begin(); it != books.end(); ++it) {
                printf("%d: %s\n", (*it)["id"].get<int>(), 
                        (*it)["title"].get<string>().c_str());
            }
        }
    }
    free(message);
    free(response);
}

void modify_book(char* server_name, int sockfd, char* auth, string request) {
    string path = "/api/v1/tema/library/books/";
    string buffer;
    // add id to path
    printf("Enter id: ");
    getline(cin, buffer);
    while(!is_positive_number(buffer)) {
        printf("Id needs to be a positive number!\nEnter id: ");
        getline(cin, buffer);
    }
    path += buffer;

    char* message = compute_request(request, server_name, path.c_str(),
                        NULL, NULL, 0, auth);
    send_to_server(sockfd, message);

    char* response = receive_from_server(sockfd);
    if (basic_extract_json_list(response) == NULL) {
        // if we don't have a json list check if it's an error
        if (basic_extract_json_response(response) != NULL) {
            json error = json::parse(basic_extract_json_response(response));
            printf("%s\n", error["error"].get<string>().c_str());
            // explain to client that access to library is needed for current action
            if (auth == NULL) {
                printf("Get access to library and try again!\n");
            }
        } else {
            // if there is no json it means we successfully deleted a book
            printf("Book was deleted!\n");
        }
    } else {
        // book is returned as json list with one element
        json book = json::parse(basic_extract_json_list(response));
        printf("Book info:\n");
        json::iterator it = book.begin();
        // format json for client
        printf("%s by %s, published by %s, %s, %d pages.\n",
            (*it)["title"].get<string>().c_str(),
            (*it)["author"].get<string>().c_str(),
            (*it)["publisher"].get<string>().c_str(),
            (*it)["genre"].get<string>().c_str(),
            (*it)["page_count"].get<int>());
    }
    free(message);
    free(response);
}

void identify_command(string command, int sockfd) {
    char* server_name = new char[50];
    server_name = strdup("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com");
    static char* session_cookie = NULL;
    static char* auth = NULL;

    if (command == "register" || command == "login") {
        if (command == "login" && session_cookie != NULL) {
            printf("One user is already online!\n");
            return;
        }
        char* cookie = register_or_login_client(server_name, sockfd, command);
        if (cookie != NULL) {
            // update session cookie
            session_cookie = cookie;
        }
        return;
    }
    if (command == "logout") {
        logout_client(server_name, sockfd, session_cookie);
        // reset session cookie and auth;
        session_cookie = NULL;
        auth = NULL;
        return;
    }
    if (command == "enter_library") {
        char* aux = enter_library(server_name, sockfd, session_cookie);
        if (aux != NULL) {
            auth = aux;
        }
        return;
    }
    if (command == "get_books") {
        get_books_summary(server_name, sockfd, auth);
        return;
    }
    if (command == "add_book") {
        add_book(server_name, sockfd, auth);
        return;
    }
    if (command == "get_book") {
        modify_book(server_name, sockfd, auth, "GET");
        return;
    }
    if (command == "delete_book") {
        modify_book(server_name, sockfd, auth, "DELETE");
        return;
    }
    printf("Command format not recognised.\n");
}

int main(int argc, char *argv[]) {
    int sockfd;
    string command;
    while(1) {
        // get command from client
        cout << "Enter command: ";
        getline(cin, command);
        if (command == "exit") {
            break;
        }
        // keep connection up
        sockfd = open_connection(strdup(SERVER_IP), 8080, AF_INET,
                    SOCK_STREAM, 0);
        // identify command and interact with server and client
        identify_command(command, sockfd);

    }
    close(sockfd);
    return 0;
}
