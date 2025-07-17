// user/producer.c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {
    int fd;
    const char *msg = "Hello from producer!";

    fd = open("/dev/simplepci", O_WRONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (write(fd, msg, strlen(msg)) < 0) {
        perror("write");
        close(fd);
        return 1;
    }

    printf("Producer: Message written to driver: %s\n", msg);
    close(fd);
    return 0;
}
