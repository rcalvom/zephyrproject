

int zephyr_socket(int domain);
int zephyr_bind(int fd, unsigned short int port);
int zephyr_listen(int fd);
int zephyr_accept(int fd, unsigned short int *port);

