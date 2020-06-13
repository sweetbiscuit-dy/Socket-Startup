#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define DEFAULT_PORT 6666

int main(int argc, char ** argv) {
    /**
     * listening socket: serverfd
     * data tx socket: acceptfd
     */
    int serverfd, acceptfd;

    struct sockaddr_in local_addr; // address info of the server
    struct sockaddr_in cli_addr; // address info of client

    unsigned int sin_size, listen_num = 10;
    if((serverfd = sock(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("server socket construction failure");
        return -1;
    }

    printf("server socket constructed\n");

    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(DEFAULT_PORT); // mind that the port should be converted network byte order
    /**
     * the useage of INADDR_ANY: it's 0.0.0.0 after convertion which means all IPs of the local machine, because
     * the machine might be equipped with multiple NICs. 
     * suppose that the machine has three NICs which are connected to three different networks, then the machine
     * has three different ip addresses. if an app needs to listen to a certain port, which NIC should it monitor?
     * if the socket binds to a specific ip address, the app can only monitor the port of the NIC to which the ip
     *  address attatches. if you want to monitor all three NICs, you should bind three ip addresses and this equals
     * to manage three sockets which is rather complicated.
     * so there is INADDR_ANY, you only need to bind the socket to INADDR_ANY and manage only one socket. no matter
     * from which NIC the data arrives, socket can receive it from certain port. 
     */
    local_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(local_addr.sin_zero), sizeof local_addr.sin_zero);
    if(bind(serverfd, (struct sockaddr *)&local_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind failure");
        return -2;
    }
    printf("bind is ok\n");

    if(listen(serverfd, listen_num) == -1) {
        perror("listen failure");
        return -3;
    }
    printf("listen is ok\n");

    fd_set client_fdset; // the set of monitoring file desceiptors
    int maxsock; // the largest number of monitoring file desceiptors
    struct timeval tv; // timeout
    int client_sockfd[5]; // active sockfd
    bzero((void *)client_sockfd, sizeof(client_sockfd));
    int conn_amount = 0; // the number of descriptors
    maxsock = serverfd;
    char buffer[1024]; // recv buffer
    int ret = 0;

    while(1) {
        FD_ZERO(&client_fdset); // initialization
        FD_SET(serverfd, &client_fdset);
        // set the timeout value to be 30 sec
        tv.tv_sec = 30;
        tv.tv_usec = 0;

        // 
        for(int i = 0; i < 5; i++) {
            if(client_sockfd[i] != 0) {
                FD_SET(client_sockfd[i], &client_fdset);
            }
        }
        printf("put sockfd in fdset\n");

        // int select(int maxfdp, fd_set *readfds, fd_set *writefds, fd_set *errorfds, struct timeval *timeout);
        ret = select(maxsock + 1, &client_fdset, NULL, NULL, &tv);
        if(ret < 0) { // error case
            perror("select error\n");
            break;
        }
        else if (ret == 0) { // timeout case
            printf("timeout\n");
        }

        // ret is a positive integer, now polling every file descriptor
        for(int i = 0; i < conn_amount; i++) {
            // check if the client_sockfd[i] in the client_fdset is readable
            if(FD_ISSET(client_sockfd[i], &client_fdset)) {
                printf("start to rece from client [%d] :\n", i);
                /** prototype： int recv(SOCKET S, char FAR *buf, int len, int flags);
                 * s: the peer socket descriptor
                 * buf: buffer area, stores the data received from the recv function
                 * flag: usually set to 0
                 */
                ret = recv(client_sockfd[i], buffer, 1024, 0);
                if(ret <= 0) {
                    printf("client [%d] closes\n", i);
                    close(client_sockfd[i]);
                    FD_CLR(client_sockfd[i], &client_fdset);
                    client_sockfd[i] = 0;
                }
                else {
                    printf("recv from client [%d] : %s\n", i, buffer);
                }
            }
        }

        // check if there is a new connection request. if there is, make the connection and add it to the client_sockfd
        if(FD_ISSET(serverfd, &client_fdset)) {
            // accept the connection
            struct sockaddr_in client_addr;
            size_t size = sizeof (struct sockaddr_in);
            int sock_client = accept(serverfd, (struct sockaddr*)(&client_addr), (unsigned int*)(&size));
            if(sock_client < 0) {
                perror("accept failure\n");
                continue;
            }

            if(conn_amount < 5) {
                client_sockfd[conn_amount++] = sock_client;
                bzero(buffer, sizeof(buffer));
                strcpy(buffer, "this is the server! welcome\n");
                send(sock_client, buffer, 1024, 0);
                printf("new connection client [%d] %s: %d\n", conn_amount, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                bzero(buffer, sizeof(buffer));
                ret = recv(sock_client, buffer, sizeof(buffer), 0);

                if(ret < 0) {
                    perror("recv error!\n");
                    close(serverfd);
                    return -1;
                }
                printf("recv : %s\n", buffer);

                if(sock_client > maxsock) {
                    maxsock = sock_client;
                }
                else {
                    printf("max connections. quit\n");
                    break;
                }
            }
        }
    }

    for(int i = 0; i < 5; i++) {
        if(client_sockfd[i] != 0) {
            close(client_sockfd[i]);
        }
    }
    close(serverfd);
    return 0;
}