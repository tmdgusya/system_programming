#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <fifo_path>\n", argv[0]);
        return 1;
    }

    // Open with O_RDWR so that read() blocks (or returns EAGAIN) instead of returning 0
    // when no other writer is connected.
    int fd = open(argv[1], O_RDWR | O_NONBLOCK);
    if (fd == -1) {
        if (errno == ENOENT) {
            // FIFO doesn't exist, create it
            if (mkfifo(argv[1], 0666) == -1) {
                perror("mkfifo");
                return 1;
            }
            // Try opening again
            fd = open(argv[1], O_RDWR | O_NONBLOCK);
        }
        if (fd == -1) {
            perror("open");
            return 1;
        }
    }

    printf("Waiting for data on %s...\n", argv[1]);

    char buf[1024];
    while (1) {
        ssize_t ret = read(fd, buf, sizeof(buf));
        
        if (ret == -1) {
            if (errno == EAGAIN) {
                printf(".");
                fflush(stdout);
                usleep(100000); // 100ms
                continue;
            }
            perror("read");
            break;
        } else if (ret == 0) {
            // With O_RDWR, this shouldn't happen unless we close the write end ourselves?
            // Actually, for FIFO opened O_RDWR, read returns 0 only if we close the fd?
            // Or if the pipe is broken?
            // In any case, handle it.
            printf("\nWriter closed or EOF\n");
            break;
        } else if (ret > 0) {
            buf[ret] = '\0';
            printf("\nReceived: \"%s\"\n", buf);
            break;
        }
    }

    close(fd);
    return 0;
}