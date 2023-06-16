#include "zephyr.h"

#include <zephyr/net/socket.h>

int zephyr_socket(int domain){
    return zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

int zephyr_bind(int fd, unsigned short int port){
    struct sockaddr_in bind_addr;
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    bind_addr.sin_port = port;
    return zsock_bind(fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr));
}

int zephyr_listen(int fd){
    return zsock_listen(fd, 5);
}

int zephyr_accept(int fd, unsigned short int *port){
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int result = zsock_accept(fd, (struct sockaddr *)&client_addr, &client_addr_len);
    *port = client_addr.sin_port;
    return result;
}