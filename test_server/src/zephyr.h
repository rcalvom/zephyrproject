

int zephyr_socket(int domain);
int zephyr_bind(int fd, unsigned short int port);
int zephyr_listen(int fd);
int zephyr_accept(int fd, unsigned short int *port);
int zephyr_connect(int fd, unsigned int *addr);
int zephyr_read(int fd);
int zephyr_write(int fd, void* buffer, unsigned long size);
int zephyr_close(int fd);


