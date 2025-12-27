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

    const char *msg = "Hello, with fsync!\n";
    ssize_t n = write(fd, msg, strlen(msg));
    if (n < 0) {
        perror("write");
        close(fd);
        return 1;
    }

    // 디스크에 실제로 쓰여질 때까지 대기
    if (fsync(fd) < 0) {
        perror("fsync");
        close(fd);
        return 1;
    }

    close(fd);
    printf("Wrote %zd bytes and synced to disk\n", n);
    return 0;
}
