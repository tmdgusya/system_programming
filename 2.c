#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    int fd;
    ssize_t ret;
    char buf[BUF_SIZE];
    ssize_t len = BUF_SIZE;
    char *ptr = buf;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    while (len != 0 && (ret = read(fd, ptr, len)) != 0) {
        if (ret == -1) {
            perror("read");
            break;
        }
        len -= ret;
        ptr += ret;
    }

    close(fd);

    // 버퍼 내용 출력
    printf("%.*s", (int)(BUF_SIZE - len), buf);

    return 0;
}
