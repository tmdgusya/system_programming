#include <stdio.h>
#include <string.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    const char *FILENAME = "iovec_test.txt";
    
    // 1. 쓰기(Write) 준비: 3개의 문장
    char *str1 = "First sentence: Hello.\n";
    char *str2 = "Second sentence: I love Linux.\n";
    char *str3 = "Third sentence: Goodbye.\n";

    // 쓰기용 iovec 배열 설정
    struct iovec iov_write[3];
    
    iov_write[0].iov_base = str1;
    iov_write[0].iov_len = strlen(str1);
    
    iov_write[1].iov_base = str2;
    iov_write[1].iov_len = strlen(str2);
    
    iov_write[2].iov_base = str3;
    iov_write[2].iov_len = strlen(str3);

    // 파일 열기 (쓰기/읽기 모드)
    int fd = open(FILENAME, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // 2. writev 호출: 흩어진 3개의 문자열을 한 번에 파일에 씀 (Gather Output)
    ssize_t written = writev(fd, iov_write, 3);
    printf("Wrote %ld bytes to file.\n", written);

    // ---------------------------------------------------------

    // 파일 오프셋을 다시 처음으로 돌림 (읽기 위해)
    lseek(fd, 0, SEEK_SET);

    // ---------------------------------------------------------

    // 3. 읽기(Read) 준비: 데이터를 담을 3개의 빈 버퍼
    // 읽어올 때는 정확한 분리를 위해 길이를 맞춰줍니다.
    char buf1[50], buf2[50], buf3[50];
    
    // 읽기용 iovec 배열 설정
    struct iovec iov_read[3];

    iov_read[0].iov_base = buf1;
    iov_read[0].iov_len = strlen(str1); // 첫 번째 문장 길이만큼만 읽도록 설정

    iov_read[1].iov_base = buf2;
    iov_read[1].iov_len = strlen(str2); // 두 번째 문장 길이만큼

    iov_read[2].iov_base = buf3;
    iov_read[2].iov_len = strlen(str3); // 세 번째 문장 길이만큼

    // 4. readv 호출: 파일 내용을 3개의 버퍼에 나눠서 담음 (Scatter Input)
    ssize_t read_bytes = readv(fd, iov_read, 3);
    printf("Read %ld bytes from file.\n\n", read_bytes);

    // 문자열 출력을 위해 null terminator 추가 (readv는 자동으로 추가 안 해줌)
    buf1[iov_read[0].iov_len] = '\0';
    buf2[iov_read[1].iov_len] = '\0';
    buf3[iov_read[2].iov_len] = '\0';

    // 결과 확인
    printf("--- Buffer Contents ---\n");
    printf("Buffer 1: %s", buf1);
    printf("Buffer 2: %s", buf2);
    printf("Buffer 3: %s", buf3);

    close(fd);
    return 0;
}
