#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    int fd;
    char *filename = argv[1];

    fd = creat(filename, 0644);

    if (fd == -1) {
        perror("creat");
        return 1;
    }

    close(fd);
}
