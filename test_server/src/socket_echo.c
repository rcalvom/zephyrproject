#include <zephyr/kernel.h>
#include <dlfcn.h>

#include "portability_layer.h"
#include "netif.h"
#include "zephyr.h"



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
            //z_impl_zsock_close(*pcb);
        }
        memset(socketArray, 0, MAX_SOCKET_ARRAY * sizeof(int));
    }
    socketCounter = 3;
    printk("PacketDrill Handler Task Reset..\n");
    return sizeSocketArray;
}


int socket_syscall(int domain){
    printk("Socket create in Zephyr\n");
    int serv;
    serv = zephyr_socket(domain);
    //ip_version = domain == AF_INET ? 4 : 6;
    ip_version = 4;
    if (serv < 0) {
        printk("Error in \"socket_create\" instruction");
        return -1;
    } else {
        socketArray[socketCounter] = serv;
        return socketCounter++;
    }
}
int bind_syscall(int index, unsigned short int port){
    printk("Socket bind in Zephyr\n");
    //struct sockaddr_in bind_addr;
    int serv = socketArray[index];
    /*bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind_addr.sin_port = htons(port);*/
    if (zephyr_bind(serv, port) < 0) {
        printk("Error: bind: %d\n", errno);
        return -1;
    } else {
        port_global = port;
        return 0;
    }
}
int listen_syscall(int index){
    printk("Socket listen in Zephyr\n");
    int serv = socketArray[index];
    if (zephyr_listen(serv) < 0) {
        printk("Error in \"socket_listen\" instruction");
        return -1;
    } else {
        return 0;
    }
}
struct SyscallResponsePackage accept_syscall(int index){
    printk("Socket accept in Zephyr\n");
    struct SyscallResponsePackage syscallResponse;
    int serv = socketArray[index];
    int response;
    struct sockaddr_in client_addr;
    struct sockaddr_in6 client_addr_6;
    socklen_t client_addr_len = sizeof(client_addr);
    unsigned short int port;
    printk("About to sleep in accept...\n");
    int client = zephyr_accept(serv, &port);
    printk("Waking up from accept...\n");
    if (client < 0) {
        printk("error: accept: %d\n", errno);
    }
    socketArray[socketCounter] = client;
    response = socketCounter;
    socketCounter++;
    struct AcceptResponsePackage acceptResponse;
    if (ip_version == 6) {
        struct sockaddr_in6 addr;
        addr.sin6_family = AF_INET6;
        addr.sin6_port = htons(port_global);
        memcpy(&addr.sin6_addr, &client_addr_6.sin6_addr, sizeof(struct in6_addr));
        acceptResponse.addr6 = addr;
        acceptResponse.addrlen = sizeof(struct sockaddr_in6);
    } else {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = port;
        addr.sin_addr.s_addr = inet_addr("192.0.2.2");
        //memcpy(&addr.sin_addr, &client_addr.sin_addr, sizeof(struct in_addr));
        acceptResponse.addr = addr;
        acceptResponse.addrlen = sizeof(struct sockaddr_in);
    }
    syscallResponse.result = response;
    syscallResponse.acceptResponse = acceptResponse;
    return syscallResponse;
}


int connect_syscall(int index, struct in_addr address, unsigned short int port){
    printk("Socket connect in Zephyr\n");
    struct in_addr dest_ipaddr;
    int serv = socketArray[index];
    memcpy(&dest_ipaddr, &address, sizeof(struct in_addr));
    if (connect(serv, &address, sizeof(struct in_addr)) < 0) {
        printk("Error in \"socket_connect\" instruction");
        return -1;
    } else {
        return 0;
    }
    return 0;
}
int read_syscall(int index){
    char buf[128];
    int serv = socketArray[index];
    int len = recv(serv, buf, sizeof(buf), 0);
    if (len < 0) {
        printk("Error in \"socket_read\" instruction");
        return -1;
    }
    return 0;
}
int write_syscall(int index, void *buffer, unsigned long size){
    int response;
    int serv = socketArray[index];
    response = send(serv, buffer, size, 0);
    if (response < 0) {
        printk("Error in \"socket_write\" instruction");
        return -1;
    }
    return 0;
}
int close_syscall(int index){
    z_impl_zsock_close(socketArray[index]);
    return 0;
}
int init_syscall(){
    return reset_socket_array();
}

void main(void){
    network_interface_init();
    void *handle = dlopen(getenv("PORTABILITY_LAYER_PATH"), RTLD_NOW | RTLD_LOCAL | RTLD_NODELETE);
    if(handle == NULL){
		const char* error = dlerror();
        printk("Error importing portability layer %s\n", error);
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

