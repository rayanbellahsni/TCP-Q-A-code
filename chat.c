#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>


#define PORT "3495" // the port client will be connecting to 
#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr); // Return IPv4 address
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr); // Return IPv6 address
}

int main(int argc,char *argv[])
{
    (void)argc;
    int sockfd, numbytes;  
    char buf[MAXDATASIZE]; // Buffer to store the data sent or received
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN]; // String to store the IP address of the server

    // Get server info to establish a connection
    memset(&hints, 0, sizeof hints); // Clear hints structure
    hints.ai_family = AF_UNSPEC; // Allow for both IPv4 and IPv6 addresses
    hints.ai_socktype = SOCK_STREAM; // Use a stream socket (TCP)

    // Get address information for the server (from the argument)
    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv)); // Handle error if getaddrinfo fails
        return 1;
    }

    // Loop through all the results and connect to the first available one
    for(p = servinfo; p != NULL; p = p->ai_next) {
        // Create a socket using the information in p
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket"); // Handle socket creation error
            continue;
        }

        // Try to connect to the server using the socket
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd); // Close socket on connection failure
            perror("client: connect"); // Handle connection error
            continue;
        }
        break; // Break the loop if successful connection is made
    }

    // If no valid connection was made, exit with an error
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    // Convert the server's address into a readable format and print it
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // Free the memory allocated by getaddrinfo

    // Communicate with the server in a loop
    while(1) {
        // Prompt user to ask a question or type 'exit' to quit
        printf("Client: Ask a question: (or 'exit' to quit): ");
        fgets(buf, MAXDATASIZE, stdin); // Get user input
        buf[strcspn(buf, "\n")] = 0; // Remove newline character from input

        // Exit the loop if user types 'exit'
        if (strcmp(buf, "exit") == 0) {
            break;
        }

        // Send the question to the server
        if (send(sockfd, buf, strlen(buf), 0) == -1) {
            perror("send"); // Handle send error
            close(sockfd);
            exit(1);
        }

        // Receive the server's response
        if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
            perror("recv"); // Handle receive error
            close(sockfd);
            exit(1);
        }

        buf[numbytes] = '\0'; // Null-terminate the received message
        printf("client: received '%s'\n", buf); // Display the response from the server
    }

    close(sockfd); // Close the socket after communication ends
    return 0; // Exit the program
}
