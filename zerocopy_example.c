/*
 * Zero-Copy I/O Example
 *
 * Problem: Traditional I/O does duplicated copy:
 *   - Read: kernel buffer -> user space buffer
 *   - Write: user space buffer -> kernel buffer
 *
 * Solution: Zero-copy eliminates intermediate user buffer copy
 *   - mmap(): returns pointer directly to kernel buffer
 *   - sendfile(): transfers directly between kernel buffers
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <pthread.h>

#define FILE_SIZE (4 * 1024)  /* 4KB test file */
#define NUM_THREADS 4

/* 전통적인 I/O - User buffer에 복사 발생 */
ssize_t traditional_read(int fd, void *buf, size_t count) {
    /* read() 시스템콜:
     * 1. 커널 버퍼에서 데이터를 읽음
     * 2. 사용자 공간 buf로 복사 (여기서 복사 발생!)
     */
    return read(fd, buf, count);
}

ssize_t traditional_write(int fd, const void *buf, size_t count) {
    /* write() 시스템콜:
     * 1. 사용자 공간 buf에서 커널 버퍼로 복사 (여기서 복사 발생!)
     * 2. 커널이 디스크에 기록
     */
    return write(fd, buf, count);
}

/* Zero-Copy I/O using mmap() - 복사 없음 */
void* mmap_zero_copy_reader(void *arg) {
    int thread_id = *(int*)arg;
    int fd;
    struct stat st;

    /* 파일 열기 */
    fd = open("zerocopy_test.txt", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return NULL;
    }

    /* 파일 크기 확인 */
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return NULL;
    }

    /*
     * mmap() 핵심:
     * - 파일 내용을 직접 메모리에 매핑
     * - 복사 없음! 포인터만 반환됨
     * - 사용자 버퍼 대신 커널 버퍼를 직접 접근
     */
    void *mapped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return NULL;
    }

    printf("[Thread %d] mmap()로 매핑 완료 - 포인터: %p\n", thread_id, mapped);
    printf("[Thread %d] 첫 64바이트 직접 읽기 (복사 없음):\n", thread_id);
    printf("[Thread %d] \"%.64s...\"\n", thread_id, (char*)mapped);

    /* 매핑 해제 - 실제 복사는 일어나지 않음 */
    munmap(mapped, st.st_size);
    close(fd);

    return NULL;
}

/* mmap으로 파일 쓰기 (read-write 매핑) */
void* mmap_zero_copy_writer(void *arg) {
    int thread_id = *(int*)arg;
    int fd;
    size_t file_size = FILE_SIZE;

    /* 파일 생성/열기 */
    fd = open("zerocopy_test.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        return NULL;
    }

    /* 파일 크기 확장 (sparse file 방지) */
    ftruncate(fd, file_size);

    /*
     * PROT_WRITE로 매핑:
     * - 쓰기 작업이 페이지 폴트 시점에 커널 버퍼에 직접 기록
     * - 사용자 버퍼 복사 없음
     */
    void *mapped = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return NULL;
    }

    printf("[Thread %d] mmap() 쓰기 모드 매핑 완료\n", thread_id);

    /* 직접 메모리에 쓰기 - 복사 없음! */
    snprintf(mapped, file_size,
        "[Thread %d] Zero-Copy 데이터 쓰기\n"
        "이 데이터는 사용자 버퍼를 거치지 않고\n"
        "커널 버퍼에 직접 기록됩니다.\n"
        "timestamp: %ld\n",
        thread_id, (long)time(NULL));

    /* msync로 디스크에 동기화 (필요시) */
    msync(mapped, file_size, MS_SYNC);

    printf("[Thread %d] 쓰기 완료 - 직접 메모리 접근\n", thread_id);

    munmap(mapped, file_size);
    close(fd);

    return NULL;
}

/*
 * sendfile() 사용 예시 (파일 -> 파일 전송, 커널 내에서 직접 전송)
 *
 * sendfile()은 두 파일 기술자 간의 데이터를 직접 전송:
 *   in_fd (파일) -> kernel buffer -> out_fd (파일/소켓)
 * 사용자 공간을 거치지 않음!
 */
ssize_t sendfile_zero_copy(int out_fd, int in_fd, off_t *offset, size_t count) {
    /* sendfile() 시스템콜:
     * - in_fd에서 out_fd로 데이터 직접 전송
     * - 사용자 버퍼를 전혀 사용하지 않음!
     * - 완전히 커널 공간에서 처리
     *
     * 전통적 방식:
     *   read(in_fd, buf, n) + write(out_fd, buf, n)  -> 2번 복사
     *
     * sendfile 방식:
     *   sendfile(out_fd, in_fd, &offset, n)          -> 0번 복사
     */
    return sendfile(out_fd, in_fd, offset, count);
}

void test_sendfile(void) {
    int in_fd, out_fd;
    off_t offset = 0;

    /* 소스 파일 생성 */
    in_fd = open("zerocopy_test.txt", O_RDONLY);
    if (in_fd < 0) {
        perror("open source");
        return;
    }

    /* 대상 파일 생성 */
    out_fd = open("zerocopy_output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd < 0) {
        perror("open dest");
        close(in_fd);
        return;
    }

    struct stat st;
    fstat(in_fd, &st);

    printf("\n=== sendfile() Zero-Copy 전송 ===\n");
    printf("파일 크기: %ld 바이트\n", (long)st.st_size);

    /* sendfile으로 직접 전송 - 복사 없음! */
    ssize_t sent = sendfile_zero_copy(out_fd, in_fd, &offset, st.st_size);
    printf("전송 완료: %zd 바이트 (사용자 버퍼 미사용)\n", sent);

    close(in_fd);
    close(out_fd);
}

/* 전통적 I/O vs Zero-Copy 비교 */
void compare_io_methods(const char *filename) {
    int fd;
    char user_buf[4096];
    ssize_t bytes;

    printf("\n=== I/O 방식 비교: %s ===\n", filename);

    /* 전통적 I/O - 복사 발생 */
    fd = open(filename, O_RDONLY);
    if (fd >= 0) {
        bytes = traditional_read(fd, user_buf, sizeof(user_buf));
        printf("전통적 read(): %zd 바이트 읽음 (커널->사용자 복사)\n", bytes);
        close(fd);
    }

    /* Zero-Copy I/O (mmap) - 복사 없음 */
    fd = open(filename, O_RDONLY);
    if (fd >= 0) {
        struct stat st;
        fstat(fd, &st);

        void *mapped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (mapped != MAP_FAILED) {
            printf("mmap(): %ld 바이트 매핑 (복사 없음, 직접 포인터 접근)\n", (long)st.st_size);
            munmap(mapped, st.st_size);
        }
        close(fd);
    }
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS] = {1, 2, 3, 4};

    printf("=== Zero-Copy I/O 예제 ===\n");
    printf("문제: 전통적 I/O는 커널->사용자 복사 발생\n");
    printf("해결: mmap/sendfile로 포인터 직접 반환\n\n");

    /* 1. 쓰기 스레드로 파일 생성 */
    printf("--- Phase 1: mmap으로 파일 쓰기 ---\n");
    for (int i = 0; i < 2; i++) {
        pthread_create(&threads[i], NULL, mmap_zero_copy_writer, &thread_ids[i]);
        usleep(100000);  /* 파일 경합 방지 */
    }
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);

    /* 2. 읽기 스레드로 파일 읽기 (복사 없음) */
    printf("\n--- Phase 2: mmap으로 파일 읽기 (복사 없음) ---\n");
    for (int i = 0; i < 2; i++) {
        pthread_create(&threads[i], NULL, mmap_zero_copy_reader, &thread_ids[i]);
    }
    for (int i = 0; i < 2; i++) {
        pthread_join(threads[i], NULL);
    }

    /* 3. sendfile으로 파일 전송 */
    test_sendfile();

    /* 4. 비교 출력 */
    compare_io_methods("zerocopy_test.txt");

    printf("\n=== 요약 ===\n");
    printf("전통적 I/O: read()/write() - 커널<->사용자 버퍼 복사 발생\n");
    printf("Zero-Copy:  mmap() - 포인터로 직접 접근, 복사 없음\n");
    printf("           sendfile() - 커널 내에서 직접 전송\n");

    return 0;
}
