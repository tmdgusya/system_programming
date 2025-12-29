#include <stdio.h>

int main(void) {
    FILE *in;
    int c;

    /* 읽기 전용으로 파일 열기 */
    in = fopen("data", "r");
    if (!in) {
        perror("fopen");
        return 1;
    }

    /* 파일에서 문자 하나씩 읽기 */
    while ((c = fgetc(in)) != EOF) {
        putchar(c);
    }

    /* EOF와 에러 상태 확인 */
    if (ferror(in)) {
        /* 읽기 에러 발생 */
        perror("fgetc error");
        fclose(in);
        return 1;
    }

    if (feof(in)) {
        /* EOF 도달 (정상 종료) */
        printf("\n[INFO] EOF에 도달했습니다 (정상 종료)\n");
    }

    if (fclose(in)) {
        perror("fclose");
        return 1;
    }

    return 0;
}
