#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

/* 공유 파일 포인터 */
FILE *shared_file = NULL;

/* 스레드별 작업 ID를 출력하기 위한 mutex - 제거 가능 */

void safe_printf(int thread_id, const char *msg) {
    flockfile(stdout);  /* stdout에 lock 걸기 */
    printf(msg, thread_id);
    funlockfile(stdout);
}

void* writer_thread(void *arg) {
    int thread_id = *(int*)arg;
    printf("[Thread %d] 파일 쓰기 시작\n", thread_id);

    /* 파일 스트림에 lock을 걸고 쓰기 */
    flockfile(shared_file);

    printf("[Thread %d] 락 획득 - 파일에 쓰기 시작\n", thread_id);

    /* 쓰기 작업 시뮬레이션 (각 문자를 하나씩 fprintf로 출력) */
    for (int i = 0; i < 5; i++) {
        fprintf(shared_file, "Thread %d: message %d\n", thread_id, i);
        fflush(shared_file);  /* 버퍼flush */
        usleep(100000);  /* 100ms 대기 - 다른 스레드가 기다리게 함 */
    }

    printf("[Thread %d] 파일 쓰기 완료\n", thread_id);
    funlockfile(shared_file);

    return NULL;
}

void* reader_thread(void *arg) {
    int thread_id = *(int*)arg;

    printf("[Reader Thread %d] 파일 읽기 시도\n", thread_id);

    /* 파일 스트림에 lock을 걸고 읽기 */
    flockfile(shared_file);

    printf("[Reader Thread %d]락 획득 - 파일 읽기 시작\n", thread_id);

    /* 읽기 작업 시뮬레이션 */
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), shared_file) != NULL) {
        printf("[Reader Thread %d] 읽은 내용: %s", thread_id, buffer);
        usleep(50000);  /* 50ms 대기 */
    }

    printf("[Reader Thread %d] 파일 읽기 완료 (EOF)\n", thread_id);
    funlockfile(shared_file);

    return NULL;
}

int main(void) {
    pthread_t threads[4];
    int thread_ids[4] = {1, 2, 3, 4};

    /* 공유 파일 열기 */
    shared_file = fopen("shared_data.txt", "w+");
    if (!shared_file) {
        perror("fopen");
        return 1;
    }

    printf("=== flockfile/funlockfile 예시 ===\n");
    printf("스레드 1, 2는 파일에 쓰기를 시도합니다.\n");
    printf("스레드 3은 파일을 읽으려 하지만, 쓰기 스레드가 lock을 가지고 있어 기다립니다.\n\n");

    /* 쓰기 스레드 생성 (스레드 1, 2) */
    pthread_create(&threads[0], NULL, writer_thread, &thread_ids[0]);
    pthread_create(&threads[1], NULL, writer_thread, &thread_ids[1]);

    /* 잠시 대기 - 쓰기 스레드가 lock을 획득하게 함 */
    sleep(1);

    /* 읽기 스레드 생성 (스레드 3) - 쓰기 스레드가 lock을 가지고 있어 대기 */
    pthread_create(&threads[2], NULL, reader_thread, &thread_ids[2]);

    /* 쓰기 스레드 생성 (스레드 4) - 경합 상황 확인용 */
    pthread_create(&threads[3], NULL, writer_thread, &thread_ids[3]);

    /* 모든 스레드 종료 대기 */
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\n[Main] 모든 스레드 작업 완료\n");

    /* 파일 닫기 */
    if (fclose(shared_file) != 0) {
        perror("fclose");
        return 1;
    }

    /* 결과 파일 내용 출력 */
    printf("\n=== 최종 파일 내용 ===\n");
    shared_file = fopen("shared_data.txt", "r");
    if (shared_file) {
        int c;
        while ((c = fgetc(shared_file)) != EOF) {
            putchar(c);
        }
        fclose(shared_file);
    }

    return 0;
}
