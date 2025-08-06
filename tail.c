#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define BUFFER_SIZE 4096
#define DEFAULT_LINES 10

char buffer[BUFFER_SIZE];

void tail(int fileDescriptor, int numLinesToShow) {
  int i, bytesRead, totalLines = 0;
  int bufferStart = 0, bufferEnd = 0;

  while ((bytesRead = read(fileDescriptor, buffer + bufferEnd, sizeof(buffer) - bufferEnd)) > 0) {
    bufferEnd += bytesRead;

    for (i = 0; i < bytesRead; i++) {
      if (buffer[bufferEnd - bytesRead + i] == '\n') {
        totalLines++;
        if (totalLines > numLinesToShow) {
          while (bufferStart < bufferEnd - bytesRead + i && buffer[bufferStart] != '\n') {
            bufferStart++;
          }
          bufferStart++; // Skip the newline character
        }
      }
    }

    if (bufferEnd == sizeof(buffer)) {
      memmove(buffer, buffer + bufferStart, bufferEnd - bufferStart);
      bufferEnd -= bufferStart;
      bufferStart = 0;
    }
  }

  for (i = bufferStart; i < bufferEnd; i++) {
    printf(1, "%c", buffer[i]);
  }

  if (buffer[bufferEnd - 1] != '\n') {
    printf(1, "\n");
  }
}

int main(int argc, char *argv[]) {
  int fileDescriptor = 0;  // Default: stdin
  int numLinesToShow = DEFAULT_LINES;
  char *inputFileName = 0;

  if (argc > 1 && argv[1][0] == '-') {
    numLinesToShow = atoi(&argv[1][1]);
    if (numLinesToShow < 0) {
      numLinesToShow = DEFAULT_LINES;
    }
    if (argc > 2) {
      inputFileName = argv[2];
    }
  } else if (argc > 1) {
    inputFileName = argv[1];
  }

  if (numLinesToShow == 0) {
    exit();
  }

  if (inputFileName && numLinesToShow != 0) {
    fileDescriptor = open(inputFileName, O_RDONLY);
    if (fileDescriptor < 0) {
      printf(2, "tail: cannot open %s\n", inputFileName);
      exit();
    }
    numLinesToShow -= 1;  // Adjusting to match behavior
  }

  tail(fileDescriptor, numLinesToShow);

  if (fileDescriptor != 0) {
    close(fileDescriptor);
  }

  exit();
  
}
