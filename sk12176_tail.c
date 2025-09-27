#include "types.h"
#include "stat.h"
#include "user.h"

#define DEFAULT_N 10

// Custom strdup for xv6
char* strdup_xv6(const char *s) {
    int len = 0;
    while (s[len]) len++;
    char *p = (char*)malloc(len + 1);
    if (!p) return 0;
    for (int i = 0; i <= len; i++) p[i] = s[i];
    return p;
}

// Circular buffer for last n lines
struct LineBuffer {
    char **lines;
    int capacity, start, count;
};

struct LineBuffer* init_buffer(int capacity) {
    struct LineBuffer *buf = (struct LineBuffer*)malloc(sizeof(*buf));
    if (!buf) return 0;
    buf->lines = (char**)malloc(capacity * sizeof(char*));
    if (!buf->lines) { free(buf); return 0; }
    buf->capacity = capacity;
    buf->start = buf->count = 0;
    return buf;
}

void free_buffer(struct LineBuffer *buf) {
    if (!buf) return;
    for (int i = 0; i < buf->count; i++)
        free(buf->lines[(buf->start + i) % buf->capacity]);
    free(buf->lines);
    free(buf);
}

void reset_buffer(struct LineBuffer *buf) {
    for (int i = 0; i < buf->count; i++)
        free(buf->lines[(buf->start + i) % buf->capacity]);
    buf->start = buf->count = 0;
}

void add_line(struct LineBuffer *buf, char *line) {
    if (!line) return;
    if (buf->count == buf->capacity) {
        free(buf->lines[buf->start]);
        buf->lines[buf->start] = line;
        buf->start = (buf->start + 1) % buf->capacity;
    } else {
        buf->lines[(buf->start + buf->count) % buf->capacity] = line;
        buf->count++;
    }
}

// Read lines dynamically from fd
void read_tail(int fd, struct LineBuffer *buf) {
    int cap = 128, len = 0;
    char *line = malloc(cap), c;
    if (!line) return;

    while (read(fd, &c, 1) > 0) {
        if (len + 1 >= cap) {
            char *new_line = malloc(cap * 2);
            if (!new_line) { free(line); return; }
            for (int i = 0; i < len; i++) new_line[i] = line[i];
            free(line);
            line = new_line;
            cap *= 2;
        }
        line[len++] = c;
        if (c == '\n') {
            line[len] = '\0';
            char *new_line = strdup_xv6(line);
            if (!new_line) { free(line); return; }
            add_line(buf, new_line);
            len = 0;
            line[0] = '\0'; // Reset buffer
        }
    }
    if (len > 0) {
        line[len] = '\0';
        char *new_line = strdup_xv6(line);
        if (new_line) add_line(buf, new_line);
    }
    free(line);
}

// Print lines in order
void print_lines(struct LineBuffer *buf, char *fname, int multi) {
    if (multi && fname) printf(1, "==> %s <==\n", fname);
    for (int i = 0; i < buf->count; i++)
        printf(1, "%s", buf->lines[(buf->start + i) % buf->capacity]);
}

// Parse -n or -N arguments
int parse_n(int argc, char *argv[], int *start_idx) {
    int n = DEFAULT_N;
    *start_idx = 1;
    if (argc > 1 && argv[1][0] == '-') {
        char *num_str;
        if (argv[1][1] == 'n') {
            if (argc < 3) {
                printf(2, "tail: missing argument for -n\n");
                exit();
            }
            num_str = argv[2];
            *start_idx = 3;
        } else {
            num_str = argv[1] + 1;
            *start_idx = 2;
        }
        n = atoi(num_str);
        if (n <= 0 || num_str[0] == '\0') {
            printf(2, "tail: invalid number of lines: %s\n", num_str);
            exit();
        }
    }
    return n;
}

int main(int argc, char *argv[]) {
    int n, start_idx;
    n = parse_n(argc, argv, &start_idx);
    struct LineBuffer *buf = init_buffer(n);
    if (!buf) {
        printf(2, "tail: memory allocation failed\n");
        exit();
    }

    int multi = (argc - start_idx > 1);
    if (start_idx >= argc) {
        read_tail(0, buf);
        print_lines(buf, 0, 0);
        free_buffer(buf);
        exit();
    }

    for (int i = start_idx; i < argc; i++) {
        int fd = open(argv[i], 0);
        if (fd < 0) {
            printf(2, "tail: cannot open %s\n", argv[i]);
            continue;
        }
        reset_buffer(buf);
        read_tail(fd, buf);
        print_lines(buf, argv[i], multi);
        close(fd);
    }
    free_buffer(buf);
    exit();
}