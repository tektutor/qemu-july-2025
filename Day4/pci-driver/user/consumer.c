// user/consumer.c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {
    int fd;
    char buf[256] = {0};
    ssize_t n;

    fd = open("/dev/simplepci", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    n = read(fd, buf, sizeof(buf) - 1);
    if (n < 0) {
        perror("read");
        close(fd);
        return 1;
    }

    printf("Consumer: Message read from driver: %s\n", buf);
    close(fd);
    return 0;
}
