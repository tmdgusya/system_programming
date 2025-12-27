#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    // O_SYNC: 모든 write가 동기적으로 수행됨
    // (write() 반환 시 이미 디스크에 기록됨)
    int fd = open("log.txt", O_WRONLY | O_CREAT | O_APPEND | O_SYNC, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    const char *msg = "Hello, with O_SYNC!\n";
    ssize_t n = write(fd, msg, strlen(msg));
    if (n < 0) {
        perror("write");
        close(fd);
        return 1;
    }

    // O_SYNC를 사용하면 write() 호출 시 이미 동기화됨
    // 따라서 fsync/fdatasync 불필요

    close(fd);
    printf("Wrote %zd bytes (already synced to disk)\n", n);
    return 0;
}
