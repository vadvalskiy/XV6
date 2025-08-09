#include "types.h"
#include "stat.h"
#include "user.h"

// Defining the maximum length for a single line.
// NOTE: As per the feedback, lines longer than this will be truncated.
#define MAX_LINE_LEN 1024
// Defining the size of the buffer for reading from the file.
// Reading in larger chunks is more efficient than one character at a time.
#define READ_BUF_SIZE 1024

/**
 * @brief Parses the number of lines from a command-line argument (e.g., "-20").
 * @param arg The string argument, which must start with '-'.
 * @return The parsed positive integer, or -1 on error.
 */
int parse_num(char *arg) {
    if (arg[0] != '-')
        return -1;

    int num = 0;
    int i;
    for (i = 1; arg[i]; i++) {
        if (arg[i] < '0' || arg[i] > '9') {
            return -1; // Invalid character found.
        }
        num = num * 10 + (arg[i] - '0');
    }
    return num;
}

int main(int argc, char *argv[]) {
    int NUM = 10; // Default number of lines to show.
    int fd = 0;   // File descriptor, defaulting to stdin.
    int i;

    // --- Argument Parsing ---
    // This section determines the number of lines (NUM) and the file to open (fd)
    // based on the command-line arguments.
    if (argc == 1) {
        // Usage: tail -> reads from stdin
        fd = 0;
    } else if (argc == 2) {
        if (argv[1][0] == '-') {
            // Usage: tail -N -> reads from stdin
            NUM = parse_num(argv[1]);
            if (NUM < 0) {
                printf(2, "tail: invalid number\n");
                exit();
            }
            fd = 0;
        } else {
            // Usage: tail [file] -> reads from file
            NUM = 10;
            if ((fd = open(argv[1], 0)) < 0) {
                printf(2, "tail: cannot open %s\n", argv[1]);
                exit();
            }
        }
    } else if (argc == 3) {
        if (argv[1][0] == '-') {
            // Usage: tail -N [file] -> reads from file
            NUM = parse_num(argv[1]);
            if (NUM < 0) {
                printf(2, "tail: invalid number\n");
                exit();
            }
            if ((fd = open(argv[2], 0)) < 0) {
                printf(2, "tail: cannot open %s\n", argv[2]);
                exit();
            }
        } else {
            printf(2, "usage: tail [-NUM] [file]\n");
            exit();
        }
    } else {
        printf(2, "usage: tail [-NUM] [file]\n");
        exit();
    }

    if (NUM == 0) {
        exit(); // Nothing to do if 0 lines are requested.
    }

    // Allocating a circular buffer (array of char pointers) to store the last NUM lines.
    char **lines = malloc(NUM * sizeof(char *));
    if (lines == 0) {
        printf(2, "tail: malloc failed\n");
        exit();
    }
    for (i = 0; i < NUM; i++) {
        lines[i] = 0; // Initializing all pointers to null.
    }

    // --- File Processing ---
    char line_buf[MAX_LINE_LEN];      // Buffer to build up a single line.
    int line_pos = 0;                 // Current position in line_buf.
    int total_lines = 0;              // Total lines encountered so far.
    int ignoring_long_line = 0;       // Flag to skip remainder of a truncated line.

    char read_buf[READ_BUF_SIZE];     // Buffer for efficient chunk-based reading.
    int bytes_read;

    // This loop reads the file in chunks into read_buf for better performance.
    while ((bytes_read = read(fd, read_buf, sizeof(read_buf))) > 0) {
        for (i = 0; i < bytes_read; i++) {
            char c = read_buf[i];

            // If we are skipping a long line, just look for the next newline.
            if (ignoring_long_line) {
                if (c == '\n') {
                    ignoring_long_line = 0; // Found the end, stops ignoring.
                }
                continue; // Skips processing this character.
            }

            if (c == '\n') { // A complete line has been found.
                line_buf[line_pos] = '\0'; // Null-terminate the string.

                // Allocating memory for the completed line and copy it from the buffer.
                char *line = malloc(line_pos + 1);
                if (line == 0) {
                    printf(2, "tail: malloc failed\n");
                    exit();
                }
                memmove(line, line_buf, line_pos + 1);

                // Using a circular array to store the latest lines.
                int index = total_lines % NUM;
                if (lines[index] != 0) {
                    free(lines[index]); // Freeing the old line that is being replaced.
                }
                lines[index] = line;
                total_lines++;
                line_pos = 0; // Resetting buffer for the next line.

            } else { // Character is not a newline.
                if (line_pos < MAX_LINE_LEN - 1) {
                    line_buf[line_pos++] = c; // Adding character to the current line buffer.
                } else {
                    // The line buffer is full, so the line is too long and must be truncated.
                    // We save the truncated part as a line.
                    printf(2, "tail: warning: line truncated\n");

                    line_buf[line_pos] = '\0';
                    char *line = malloc(line_pos + 1);
                    if (line == 0) {
                        printf(2, "tail: malloc failed\n");
                        exit();
                    }
                    memmove(line, line_buf, line_pos + 1);

                    int index = total_lines % NUM;
                    if (lines[index] != 0) {
                        free(lines[index]);
                    }
                    lines[index] = line;
                    total_lines++;
                    line_pos = 0;

                    // Setting a flag to ignore all subsequent characters until we find a newline.
                    ignoring_long_line = 1;
                }
            }
        }
    }

    // After the loop, processing any remaining text in the buffer (for files that don't end with a newline).
    if (line_pos > 0) {
        line_buf[line_pos] = '\0';
        char *line = malloc(line_pos + 1);
        if (line == 0) {
            printf(2, "tail: malloc failed\n");
            exit();
        }
        memmove(line, line_buf, line_pos + 1);

        int index = total_lines % NUM;
        if (lines[index] != 0) {
            free(lines[index]);
        }
        lines[index] = line;
        total_lines++;
    }

    // --- Output and Cleanup ---
    // Determining how many lines to print and where to start in the circular buffer.
    int count = total_lines >= NUM ? NUM : total_lines;
    int start = total_lines >= NUM ? total_lines % NUM : 0;

    // Printing the stored lines in the correct order.
    for (i = 0; i < count; i++) {
        int line_idx = (start + i) % NUM;
        printf(1, "%s\n", lines[line_idx]);
    }

    // Freeing all dynamically allocated memory to prevent leaks.
    for (i = 0; i < NUM; i++) {
        if (lines[i] != 0) {
            free(lines[i]);
        }
    }
    free(lines);

    if (fd != 0) {
        close(fd);
    }
    exit();
}
