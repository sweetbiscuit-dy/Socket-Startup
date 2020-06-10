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
     * has three different ip addresses. if an app needs to
     */
    local_addr.sin_addr.s_addr = INADDR_ANY;


}