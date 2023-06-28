#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


#include "portability_layer.h"
#include "log.h"

char *getSocketName() {
    char *socket_name;
    const char *interface_name = getenv("TAP_INTERFACE_NAME");
    if (interface_name != NULL) {
        int len = strlen(interface_name) + strlen("/tmp/socket-") + 1;
        socket_name = malloc(len * sizeof(char));
        snprintf(socket_name, len, "/tmp/socket-%s", interface_name);
    } else {
        socket_name = strdup("/tmp/socket-default");
    }
    return socket_name;
}

void run_syscalls(struct packetdrill_syscalls *interface){

    log_info("Creating socket to Paketdrill");
    char *socket_name = getSocketName();
    unlink(socket_name);
    int sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(sfd == -1){
        log_error("Error creating socket to Packetdrill");
        exit(EXIT_FAILURE);
    }
    log_info("Socket to Paketdrill created");

    log_info("Binding ports in socket to Paketdrill");
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, socket_name);
    free(socket_name);
    if(bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1){
        log_error("Error binding Packetdrill socket to port");
        exit(EXIT_FAILURE);
    }
    log_info("Ports to Paketdrill bound");

    log_info("Listening for incoming connection from Packetdrill ...");
    if (listen(sfd, 1) == -1) {
        log_error("Error listening on Packedrill socket");
        exit(EXIT_FAILURE);
    }

    for(;;){
        int cfd = accept(sfd, NULL, NULL);
        if(cfd == -1){
            log_error("Error accepting connection from PacketDrill");
            exit(EXIT_FAILURE);
        }

        log_info("Conection accepted");
        ssize_t numRead = 0;
        struct SyscallPackage syscallPackage;
        while ((numRead = read(cfd, &syscallPackage, sizeof(struct SyscallPackage))) > 0) {
            if (syscallPackage.bufferedMessage == 1) {
                void *buffer = malloc(syscallPackage.bufferedCount);
                ssize_t bufferCount = read(cfd, buffer, syscallPackage.bufferedCount);
                if (bufferCount <= 0) {
                    printf("Error reading buffer content from socket\n");
                } else if (bufferCount != syscallPackage.bufferedCount) {
                    printf("Count of buffer not equal to expected count.\n");
                } else {
                    printf("Successfully read buffer count from socket.\n");
                }
                syscallPackage.buffer = buffer;
            }

            log_info("Packetdrill command received: %s", syscallPackage.syscallId);

            struct SyscallResponsePackage syscallResponse;
            int result = 0;
            if(strcmp(syscallPackage.syscallId, "socket_create") == 0){
                result = interface->socket_syscall(syscallPackage.socketPackage.domain);
            } else if (strcmp(syscallPackage.syscallId, "socket_bind") == 0) {
                result = interface->bind_syscall(syscallPackage.bindPackage.sockfd, syscallPackage.bindPackage.addr.sin_port);
            } else if (strcmp(syscallPackage.syscallId, "socket_listen") == 0) {
                result = interface->listen_syscall(syscallPackage.listenPackage.sockfd);
            } else if (strcmp(syscallPackage.syscallId, "socket_accept") == 0) {
                struct SyscallResponsePackage response;
                response = interface->accept_syscall(syscallPackage.acceptPackage.sockfd);
                result = response.result;
                syscallResponse.acceptResponse = response.acceptResponse;
            } else if (strcmp(syscallPackage.syscallId, "socket_connect") == 0) {
                result = interface->connect_syscall(syscallPackage.connectPackage.sockfd, syscallPackage.connectPackage.addr.sin_addr, syscallPackage.connectPackage.addr.sin_port);
            } else if (strcmp(syscallPackage.syscallId, "socket_write") == 0) {
                result = interface->write_syscall(syscallPackage.writePackage.sockfd, syscallPackage.buffer, syscallPackage.bufferedCount);
            } else if (strcmp(syscallPackage.syscallId, "socket_read") == 0) {
                result = interface->read_syscall(syscallPackage.readPackage.sockfd);
            } else if (strcmp(syscallPackage.syscallId, "socket_close") == 0){
                result = interface->close_syscall(syscallPackage.closePackage.sockfd);
            } else if (strcmp(syscallPackage.syscallId, "freertos_init") == 0) {
                result = interface->init_syscall();
            }

            log_info("Syscall response buffer received: %d...\n", syscallResponse.result);
            syscallResponse.result = result;
            
            ssize_t numWrote = send(cfd, &syscallResponse, sizeof(struct SyscallResponsePackage), MSG_NOSIGNAL);
            if (numWrote == -1) {
                log_error("Error writing socket response with errno %d...\n", errno);
            } else {
                log_info("Successfully wrote socket response to Packetdrill...\n");
            }
            if(syscallPackage.bufferedMessage == 1){
                free(syscallPackage.buffer);
            }
            if(strcmp(syscallPackage.syscallId, "socket_accept") == 0){
                result = interface->accepted_callback();
            }
            if(strcmp(syscallPackage.syscallId, "socket_listen") == 0){
                result = interface->listened_callback();
            }
            if(strcmp(syscallPackage.syscallId, "socket_close") == 0){
                result = interface->closed_callback();
            }
        }
        if(numRead == 0){
            log_info("Execution completed!");
            log_info("About to unlink socket");
            if(close(cfd) == -1){
                log_error("Error closing socket\n");
                //exit(EXIT_FAILURE);
            }
            //exit(EXIT_SUCCESS);
        }else if(numRead == -1){
            log_error("Error reading from socket\n");
            //exit(EXIT_FAILURE);
        }

    }

}