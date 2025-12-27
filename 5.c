#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    int fd = open("log.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    const char *msg = "Hello, O_APPEND!\n";
    ssize_t n = write(fd, msg, strlen(msg));
    if (n < 0) {
        perror("write");
        return 1;
    }

    close(fd);
    printf("Wrote %zd bytes\n", n);
    return 0;
}
