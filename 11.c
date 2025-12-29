#include <stdio.h>

int main(void) {
    FILE *in, *out;
    struct pirate {
        char name[100];
        unsigned long booty;
        unsigned int beard_len;
    } p, blackbeard = {"Edward Teach", 950, 48};

    out = fopen("data", "w");
    if (!out) {
        perror("popen");
        return 1;
    }

    if (!fwrite(&blackbeard, sizeof (struct pirate), 1, out)) {
        perror("fwrite");
        fclose(out);
        return 1;
    }

    if (fclose(out)) {
        perror("fclose");
        return 1;
    }

    in = fopen("data", "r");
    if (!in) {
        perror("fopen");
        return 1;
    }

    if (!fread(&p, sizeof (struct pirate), 1, in)) {
        perror("fread");
        fclose(in);
        return 1;
    }

    printf("Name: %s\nBooty: %lu\nBeard Length: %u\n", p.name, p.booty, p.beard_len);

    if (fclose(in)) {
        perror("fclose");
        return 1;
    }

    return 0;
}
