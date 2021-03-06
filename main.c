#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "serials.h"

#define MYPORT 1234    // the port users will be connecting to

#define BACKLOG 5     // how many pending connections queue will hold

#define BUF_SIZE 200

typedef struct _fdNode {
    int fd;
    struct _fdNode *pNext;
} fdNode;

fdNode *pClientListHead;    // accepted connection fd

void insertFd(int fd)
{
    fdNode *pNode, *pNext;
    int found;
    if (pClientListHead==NULL) {
        pNode = malloc(sizeof(fdNode));
        pNode->fd = fd;
        pNode->pNext = NULL;
        pClientListHead = pNode;
        return;
    }
    if (pClientListHead->fd==fd)
        return;
    pNode = pClientListHead;
    pNext = pNode->pNext;
    found = 0;
    while (pNext!=NULL) {
        if (pNext->fd==fd) {
            found = 1;
            break;
        }
        pNode = pNext;
        pNext = pNext->pNext;
    }
    if (!found) {
        pNext = malloc(sizeof(fdNode));
        pNext->fd = fd;
        pNext->pNext = NULL;
        pNode->pNext = pNext;
    }
}

void deleteFd(int fd)
{
    fdNode *pNode, *pNext;
    if (pClientListHead==NULL)
        return;
    if (pClientListHead->pNext==NULL) {
        if (pClientListHead->fd==fd) {
            free(pClientListHead);
            pClientListHead = NULL;
        }
        return;
    }
    pNode = pClientListHead;
    pNext = pNode->pNext;
    while (pNext!=NULL) {
        if (pNext->fd==fd) {
            pNode->pNext = pNext->pNext;
            free(pNext);
            break;
        }
        pNode = pNext;
        pNext = pNext->pNext;
    }
}

void showclient()
{
    int i;
    fdNode *pNode = pClientListHead;
    i = 0;
    while (pNode!=NULL) {
        printf("[%d]:%d  ", i, pNode->fd);
        pNode = pNode->pNext;
        i++;
    }
    printf("\n\n");
}

int main(void)
{
    int sock_fd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct sockaddr_in server_addr;    // server address information
    struct sockaddr_in client_addr; // connector's address information
    socklen_t sin_size;
    int yes = 1;
    char ip2serial_buf[BUF_SIZE+1], serial2ip_buf[BUF_SIZE+1];
    int ret;
    int i;
    int nRead, nWrite;
    fdNode *pNode;
    char *dev = "/dev/ttyUSB0";
    int serial_fd = openSerial(dev);

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }
    
    server_addr.sin_family = AF_INET;         // host byte order
    server_addr.sin_port = htons(MYPORT);     // short, network byte order
    server_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
    memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));

    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(sock_fd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("listen port %d\n", MYPORT);

    fd_set rset;
    int maxsock;
    struct timeval tv;

    pClientListHead = NULL;
    sin_size = sizeof(client_addr);
    maxsock = sock_fd;
    memset(&ip2serial_buf[BUF_SIZE], '\0', 1);
    memset(&serial2ip_buf[BUF_SIZE], '\0', 1);
        FD_ZERO(&rset);
    while (1) {

        // initialize file descriptor set
        FD_SET(sock_fd, &rset);
        FD_SET(serial_fd, &rset);

        // timeout setting
        tv.tv_sec = 30;
        tv.tv_usec = 0;
        // add active connection to fd set
        for (pNode = pClientListHead; pNode; pNode=pNode->pNext) {
            if (pNode->fd != 0) {
                FD_SET(pNode->fd, &rset);
            }
        }

        ret = select(maxsock + 1, &rset, NULL, NULL, &tv);
        if (ret < 0) {
            perror("select");
            break;
        } else if (ret == 0) {
            printf("timeout\n");
            continue;
        }

        // check every fd in the set
        i = 0;
        for (pNode = pClientListHead; pNode; pNode=pNode->pNext) {
            if (FD_ISSET(pNode->fd, &rset)) {
                nRead = recv(pNode->fd, ip2serial_buf, BUF_SIZE, 0);
                if (nRead <= 0) {        // client close
                    printf("client[%d] close\n", i);
                    close(pNode->fd);
                    FD_CLR(pNode->fd, &rset);
                    deleteFd(pNode->fd);
                    showclient();
                } else {        // receive data
                    if (nRead < BUF_SIZE)
                        memset(&ip2serial_buf[nRead], '\0', 1);
                    printf("ip to serial: %s", ip2serial_buf);
                    nWrite = write(serial_fd, ip2serial_buf, nRead);
                    printf("  nRead=%d, nWrite=%d\n", nWrite, nRead);
                }
            }
            i++;
        }

        // check whether a new connection comes
        if (FD_ISSET(sock_fd, &rset)) {
            new_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &sin_size);
            if (new_fd <= 0) {
                perror("accept");
                continue;
            }

            // add to fd queue
            insertFd(new_fd);
            showclient();
            if (new_fd > maxsock)
                maxsock = new_fd;
        }
        // check whether new bytes comes
        if (FD_ISSET(serial_fd, &rset)) {
            nRead = read(serial_fd, serial2ip_buf, BUF_SIZE);
            if (nRead<= 0) {
                perror("serial read");
                continue;
            } else {        // receive data
                if (nRead < BUF_SIZE)
                    memset(&serial2ip_buf[nRead], '\0', 1);
                printf("serial to ip: %s", serial2ip_buf);
                for (pNode = pClientListHead; pNode; pNode=pNode->pNext) {
                    if (pNode->fd != 0) {
                        nWrite = send(pNode->fd,serial2ip_buf, nRead, 0);
                        printf("  nRead=%d, nWrite=%d\n", nRead, nWrite);
                    }
                }
            }
        }
    }

    // close other connections
    for (pNode = pClientListHead; pNode; pNode=pNode->pNext) {
        if (pNode->fd != 0) {
            close(pNode->fd);
        }
    }
    close(sock_fd);
    close(serial_fd);
    exit(0);
}
