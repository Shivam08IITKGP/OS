#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define INFNAME "dup.c"
#define OUTFNAME "DUP.c"

int main() {
    int ifd, ofd;
    char c;

    ifd = open(INFNAME, O_RDONLY);
    if (ifd < 0) {
        fprintf(stderr, "Unable to open input file in read mode...\n");
        exit(1);
    }
    fprintf(stderr, "New file descriptor obtained = %d\n", ifd);

    ofd = open(OUTFNAME, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (ofd < 0) {
        fprintf(stderr, "Unable to open output file in write mode...\n");
        exit(2);
    }
    fprintf(stderr, "New file descriptor obtained = %d\n", ofd);

    close(0);
    close(1);
    dup(ifd);
    close(ifd);
    dup(ofd);
    close(ofd);

    while (1) {
        scanf("%c", &c);
        if (feof(stdin)) break;
        if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
        printf("%c", c);
    }
    exit(0);
}

