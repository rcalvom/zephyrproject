#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#ifndef __ZEPHYR__

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
// #include <zephyr/net/socket.h>
// #include <zephyr/kernel.h>

#endif

#include <dlfcn.h>
#include "portability_layer.h"


#define BIND_PORT 4242

#define MAX_SOCKET_ARRAY 10
int socketArray[MAX_SOCKET_ARRAY];
int socketCounter = 3;
int ip_version;
unsigned short int port_global;


int reset_socket_array(void){
    int sizeSocketArray = socketCounter - 3;
    if (sizeSocketArray > 0) {
        int counter;
        for (counter = 3; counter < socketCounter; counter++) {
            int *pcb = &socketArray[counter];
            close(pcb);
        }
        memset(socketArray, 0, MAX_SOCKET_ARRAY * sizeof(int));
    }
    socketCounter = 3;
    printf("PacketDrill Handler Task Reset..\n");
    return sizeSocketArray;
}


int socket_syscall(int domain){
    printf("Socket create in Zephyr\n");
    int serv;
    serv = socket(domain, SOCK_STREAM, IPPROTO_TCP);
    ip_version = domain == AF_INET ? 4 : 6;
    if (serv < 0) {
        printf("Error in \"socket_create\" instruction");
        return -1;
    } else {
        socketArray[socketCounter] = serv;
        return socketCounter++;
    }
}
int bind_syscall(int index, unsigned short int port){
    printf("Socket bind in Zephyr\n");
    struct sockaddr_in bind_addr;
    int serv = socketArray[index];
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind_addr.sin_port = htons(port);
    // port = port;
    if (bind(serv, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
        printf("Error: bind: %d\n", errno);
        return -1;
    } else {
        port_global = port;
        return 0;
    }
}
int listen_syscall(int index){
    printf("Socket listen in Zephyr\n");
    int serv = socketArray[index];
    if (listen(serv, 5) < 0) {
        printf("Error in \"socket_listen\" instruction");
        return -1;
    } else {
        return 0;
    }
}
struct SyscallResponsePackage accept_syscall(int index){
    printf("Socket accept in Zephyr\n");
    struct SyscallResponsePackage syscallResponse;
    int serv = socketArray[index];
    int response;
    struct sockaddr_in client_addr;
    struct sockaddr_in6 client_addr_6;
    socklen_t client_addr_len = sizeof(client_addr);
    int client = accept(serv, (struct sockaddr *)&client_addr,
                &client_addr_len);
    if (client < 0) {
        printf("error: accept: %d\n", errno);
    }
    socketArray[socketCounter] = client;
    response = socketCounter;
    socketCounter++;
    struct AcceptResponsePackage acceptResponse;
    if (ip_version == 6) {
        struct sockaddr_in6 addr;
        addr.sin6_family = AF_INET6;
        addr.sin6_port = port_global;
        memcpy(&addr.sin6_addr, &client_addr_6.sin6_addr, sizeof(struct in6_addr));
        acceptResponse.addr6 = addr;
        acceptResponse.addrlen = sizeof(struct sockaddr_in6);
    } else {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = port_global;
        memcpy(&addr.sin_addr, &client_addr.sin_addr, sizeof(struct in_addr));
        acceptResponse.addr = addr;
        acceptResponse.addrlen = sizeof(struct sockaddr_in);
    }
    syscallResponse.result = response;
    syscallResponse.acceptResponse = acceptResponse;
    return syscallResponse;
}
int connect_syscall(int index, struct in_addr address, unsigned short int port){
    printf("Socket connect in Zephyr\n");
    struct in_addr dest_ipaddr;
    int serv = socketArray[index];
    memcpy(&dest_ipaddr, &address, sizeof(struct in_addr));
    if (connect(serv, &address, sizeof(struct in_addr)) < 0) {
        printf("Error in \"socket_connect\" instruction");
        return -1;
    } else {
        return 0;
    }
}
int read_syscall(int index){
    char buf[128];
    int serv = socketArray[index];
    int len = recv(serv, buf, sizeof(buf), 0);
    if (len < 0) {
        printf("Error in \"socket_read\" instruction");
        return -1;
    }
    return 0;
}
int write_syscall(int index, void *buffer, unsigned long size){
    int response;
    int serv = socketArray[index];
    response = send(serv, buffer, size, 0);
    if (response < 0) {
        printf("Error in \"socket_write\" instruction");
        return -1;
    }
    return 0;
}
int close_syscall(int index){
    close(socketArray[index]);
    return 0;
}
int init_syscall(){
    return reset_socket_array();
}

void main(void){
	int a = 0;
	printf("AAA: %i", *(&a + 1));
    void *handle = dlopen(getenv("PORTABILITY_LAYER_PATH"), RTLD_NOW | RTLD_LOCAL | RTLD_NODELETE);
    if(handle == NULL){
		const char* error = dlerror();
        printf("Error importing portability layer %s\n", error);
    }
    packetdrill_run_syscalls_fn run_syscalls = dlsym(handle, "run_syscalls");
    struct packetdrill_syscalls args;
    args.socket_syscall = socket_syscall;
    args.bind_syscall = bind_syscall;
    args.listen_syscall = listen_syscall;
    args.accept_syscall = accept_syscall;
    args.connect_syscall = connect_syscall;
    args.write_syscall = write_syscall;
    args.read_syscall = read_syscall;
    args.close_syscall = close_syscall;
    args.init_syscall = init_syscall;
    run_syscalls(&args);
}

