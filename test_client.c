#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>


#define PORT "54327"

int transmit(int socket, char* data){
    //Set up the variables for sending data
    int total_bytes_sent = 0;
    int bytes_to_send = strlen(data);
    int bytes_remaining = bytes_to_send;

    //Loop until all bytes have been sent
    while(total_bytes_sent < bytes_to_send){
        //Send the message
        int bytes_sent = send(
            socket, 
            data + total_bytes_sent,
            bytes_remaining,
            0
        );

        if(bytes_sent != -1){
            total_bytes_sent += bytes_sent;
            bytes_remaining -= bytes_sent;
        }else{
            //Some error occured. Print an error message
            fprintf(stderr, "transmit: Error on sending data\n");
            return 1;
        }
    }

    return 0;
}

int connect_to_server(){
    //Set up the addrinfo list for using getaddrinfo
    struct addrinfo* addr_list = NULL;
    //Construct the hints struct
    struct addrinfo hints = {0};
    //Use IPv6
    hints.ai_family = AF_INET6;
    //Use TCP
    hints.ai_socktype = SOCK_STREAM;
    //Use the default TCP protocol
    hints.ai_protocol = 0;

    //Get the address info for localhost
    int getaddrinfo_result = getaddrinfo(
        "localhost",
        PORT,
        &hints,
        &addr_list
    );

    //Check that the function worked
    if(getaddrinfo_result != 0){
        printf("Error code %i on getaddrinfo\n", getaddrinfo_result);
        return -1;
    }

    //Set up the socket to connect to the server using IPv6 and TCP
    int server_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if(server_socket == -1){
        fprintf(stderr, "Failed to create a socket to connect to the server\n");
        return -1;
    }

    int connect_result = -1;
    struct addrinfo* current = addr_list;
    //Loop through the list returned by getaddrinfo until an address 
    //that can be connected to is found
    while(connect_result == -1 && current != NULL){
        //Attempt to connect to the address held in current
        connect_result = connect(
            server_socket,
            current->ai_addr,
            current->ai_addrlen
        );
        current = current->ai_next;
    }

    if(connect_result == -1){
        fprintf(stderr, "Failed to connect to the server\n");
        return -1;
    }

    return server_socket;
}

//Recieves a message from the socket. Stores the message in the provided message pointer
//Returns 0 on success, -1 on a failure
int recieve_message(int socket, char** message){
    size_t peek_size = 1025;
    char* peek_text = (char* ) malloc(sizeof(char) * peek_size);
    int total_bytes_recieved = 0;
    int max_bytes_remaining = peek_size;

    char eot[2] = {4, '\0'};

    //Determines the length of text to read
    while(strstr(peek_text, eot) == NULL){
        if(max_bytes_remaining == 0){
            max_bytes_remaining += peek_size;
            peek_size *= 2;
            peek_text = (char *) realloc(peek_text, peek_size);
        }

        //Recieve data from the client
        int bytes_recieved = recv(
            socket,
            peek_text + total_bytes_recieved,
            max_bytes_remaining,
            MSG_PEEK
        );

        if(bytes_recieved > 0){
            //Client sent data, record how much was recieved
            total_bytes_recieved += bytes_recieved;
            max_bytes_remaining -= bytes_recieved;
        }else if(bytes_recieved == 0){
            //Client sent no data
            return -1;
        }else{
            //recv had an error
            fprintf(stderr, "recieve_message: Error receiving bytes, recv returned %i\n", bytes_recieved);
            return -1;
        }
    }
    
    char* transmission_end = strstr(peek_text, eot);
    transmission_end[0] = '\0';

    *message = peek_text;
    return 0;
}

int main(int argc, char* argv[]){
    
    char* message;
    if(argc < 2){
        message = "3:obj1~5:obj2~10:obj3~15";
    }else{
        message = argv[1];
    }
    int server_socket = connect_to_server();

    if(server_socket < 0){
        printf("Failed to connect to server\n");
        return -1;
    }
     
    printf("Sending message %s\n", message);

    int message_length = strlen(message);
    message[message_length + 1] = '\0';
    message[message_length] = 4;
    int result = transmit(server_socket, argv[1]);
    if(result != 0){
        printf("Failed to send message\n");
    }else{
        printf("Message sent\n");
    }

    char* bubble_letters;
    int recv_result = recieve_message(server_socket, &bubble_letters);
    if(recv_result < 0){
        printf("Failed to recieve response\n");
        return -1;
    }
    printf("Recieved message:\n%s\n", bubble_letters);

    shutdown(server_socket, SHUT_RDWR);
}