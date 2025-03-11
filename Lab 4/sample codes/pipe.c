#include <stdio.h>
#include <unistd.h>

#define BUFSIZE 80

int main()
{
    int fd[2], n = 0, i;
    char line[BUFSIZE];

    pipe(fd);

    if (fork() == 0)
    {
        close(fd[0]);
        for (i = 0; i < 10; i++)
        {
            sprintf(line, "%d", n);
            write(fd[1], line, BUFSIZE);
            printf("Child writes: %d\n", n);
            n++;
            sleep(2);
        }
    }
    else
    {
        close(fd[1]);
        for (i = 0; i < 10; i++)
        {
            printf("\t\t\t Parent trying to read pipe\n");
            read(fd[0], line, BUFSIZE);
            sscanf(line, "%d", &n);
            printf("\t\t\t Parent reads: %d\n", n);
        }
    }
}