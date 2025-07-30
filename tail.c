#include "types.h"
#include "stat.h"
#include "user.h"

#define MAXLINE 1024

int parse_num(char *arg)
{
    if (arg[0] != '-')
        return -1;

    int num = 0;
    int i;
    for (i = 1; arg[i]; i++)
    {
        if (arg[i] < '0' || arg[i] > '9')
        {
            return -1;
        }
        num = num * 10 + (arg[i] - '0');
    }
    return num;
}

int main(int argc, char *argv[])
{
    int NUM = 10;
    int fd = 0;
    int idx = 0;
    int n, pos = 0;
    int i;
    int c;
    char buf[MAXLINE];

    if (argc == 1)
    {
        fd = 0;
    }
    else if (argc == 2)
    {
        if (argv[1][0] == '-')
        {
            NUM = parse_num(argv[1]);
            if (NUM < 0)
            {
                printf(2, "tail_hs5544: invalid number\n");
                exit();
            }
            fd = 0;
        }
        else
        {
            NUM = 10;
            if ((fd = open(argv[1], 0)) < 0)
            {
                printf(2, "tail_hs5544: cannot open %s\n", argv[1]);
                exit();
            }
        }
    }
    else if (argc == 3)
    {
        if (argv[1][0] == '-')
        {
            NUM = parse_num(argv[1]);
            if (NUM < 0)
            {
                printf(2, "tail_hs5544: invalid number\n");
                exit();
            }
            if ((fd = open(argv[2], 0)) < 0)
            {
                printf(2, "tail_hs5544: cannot open %s\n", argv[2]);
                exit();
            }
        }
        else
        {
            printf(2, "usage: tail_hs5544 [-NUM] [file]\n");
            exit();
        }
    }
    else
    {
        printf(2, "usage: tail_hs5544 [-NUM] [file]\n");
        exit();
    }

    if (NUM == 0)
    {
        exit();
    }

    char **lines = malloc(NUM * sizeof(char *));
    if (lines == 0)
    {
        printf(2, "tail_hs5544: malloc failed\n");
        exit();
    }
    for (i = 0; i < NUM; i++)
    {
        lines[i] = 0;
    }

    pos = 0;
    while ((n = read(fd, &c, 1)) > 0)
    {
        if (c == '\n')
        {
            buf[pos] = '\0';
            char *line = malloc(pos + 1);
            if (line == 0)
            {
                printf(2, "tail_hs5544: malloc failed\n");
                exit();
            }
            memmove(line, buf, pos + 1);

            int index = idx % NUM;
            if (lines[index] != 0)
            {
                free(lines[index]);
            }
            lines[index] = line;
            idx++;
            pos = 0;
        }
        else
        {
            if (pos < MAXLINE - 1)
            {
                buf[pos++] = c;
            }
            else
            {
                buf[pos] = '\0';
                char *line = malloc(pos + 1);
                if (line == 0)
                {
                    printf(2, "tail_hs5544: malloc failed\n");
                    exit();
                }
                memmove(line, buf, pos + 1);
                int index = idx % NUM;
                if (lines[index] != 0)
                {
                    free(lines[index]);
                }
                lines[index] = line;
                idx++;
                pos = 0;
                while ((n = read(fd, &c, 1)) > 0 && c != '\n')
                    ;
            }
        }
    }

    if (pos > 0)
    {
        buf[pos] = '\0';
        char *line = malloc(pos + 1);
        if (line == 0)
        {
            printf(2, "tail_hs5544: malloc failed\n");
            exit();
        }
        memmove(line, buf, pos + 1);
        int index = idx % NUM;
        if (lines[index] != 0)
        {
            free(lines[index]);
        }
        lines[index] = line;
        idx++;
    }

    int count = idx >= NUM ? NUM : idx;
    int start = idx >= NUM ? idx % NUM : 0;

    for (i = 0; i < count; i++)
    {
        int line_idx = (start + i) % NUM;
        printf(1, "%s\n", lines[line_idx]);
    }

    for (i = 0; i < NUM; i++)
    {
        if (lines[i] != 0)
        {
            free(lines[i]);
        }
    }
    free(lines);

    if (fd != 0)
    {
        close(fd);
    }
    exit();
}
