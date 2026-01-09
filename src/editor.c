#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

extern int *cx;
extern int *cy;

// Macros
#define gotoxy(l, c) printf("\033[%d;%dH", (l), (c)) // move terminal cursor

// Function to get terminal size
void getTerminalSize(int* rows, int* cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        // Fallback values if ioctl fails
        *rows = 24;
        *cols = 80;
        return;
    }
    *rows = ws.ws_row;
    *cols = ws.ws_col;
    // Ensure we have valid values
    if (*rows <= 0) *rows = 24;
    if (*cols <= 0) *cols = 80;
}

// Function to count lines in text
int countLines(const char* text) {
    if (!text || !*text) return 1;
    int lines = 1;
    for (const char* p = text; *p; p++) {
        if (*p == '\n') lines++;
    }
    return lines;
}

// Function to get line length at specific row
int getLineLength(const char* text, int row) {
    if (!text) return 0;
    
    int currentRow = 0;
    int lineStart = 0;
    int i = 0;
    
    while (text[i] && currentRow <= row) {
        if (text[i] == '\n' || text[i] == '\0') {
            if (currentRow == row) {
                return i - lineStart;
            }
            currentRow++;
            lineStart = i + 1;
        }
        i++;
    }
    
    // If we reach end and it's the target row
    if (currentRow == row) {
        return i - lineStart;
    }
    
    return 0;
}

// Function to print text from editor at the bottom of screen
void printEditorMessage(const char* text) {
    if (!text) return;
    int rows, cols;
    getTerminalSize(&rows, &cols);

    // Move cursor to bottom row at column 1 and print message
    printf("\x1b[%d;1H", rows);
    printf("\x1b[K");
    printf("%s", text);

    // Move cursor back to text area
    gotoxy(*cy + 1, *cx + 1);
}

// Function to print text with proper line endings for terminal display
void printText(const char* text) {
    if (!text) return;
    for (const char* p = text; *p; p++) {
        if (*p == '\n') {
            printf("\r\n");  // Use \r\n for terminal display
        } else {
            putchar(*p);
        }
    }
}

// Function to constrain cursor to text bounds
void constrainCursor(const char* text) {
    if (!text) {
        *cx = 0;
        *cy = 0;
        return;
    }

    int totalLines = countLines(text);

    // Constrain row
    if (*cy >= totalLines) *cy = totalLines - 1;
    if (*cy < 0) *cy = 0;

    // Constrain column
    int lineLen = getLineLength(text, *cy);
    if (*cx >= lineLen) *cx = lineLen;
    if (*cx < 0) *cx = 0;
}

// Function to get index in text from cursor position
int getIndexFromCursor(const char* text, int row, int col) {
    if (!text) return 0;
    int currentRow = 0;
    int index = 0;
    while (text[index] && currentRow < row) {
        if (text[index] == '\n') {
            currentRow++;
        }
        index++;
    }
    index += col;
    return index;
}

void alterCharBuffer(char** b_textFileData, int index, char newData[]) {
    size_t len = strlen(*b_textFileData);

    if (index < 0) {
        return;  // Safety check
    }
    if (index > (int)len) {
        index = len;  // Clamp index to buffer length
    }

    size_t newDataLen = strlen(newData);

    if (newData[0] == '\b') { // Backspace detection
        if (index > 0) {
            (*cx)--; // Move cursor left
            if (*cx < 0) {
                (*cy)--;
                *cx = getLineLength(*b_textFileData, *cy);
            }
            char* temp = realloc(*b_textFileData, len - 1); // shrink by 2
            if (temp) {
                *b_textFileData = temp;
                memmove(&(*b_textFileData)[index - 1], &(*b_textFileData)[index], len - index + 1); // +1 to include null terminator
            }
        }
        constrainCursor(*b_textFileData);
        return;

    } else if (newData[0] == 127) { // Delete detection
        if (index < (int)len) {
            char* temp = realloc(*b_textFileData, len); // shrink by 1
            if (temp) {
                *b_textFileData = temp;
                memmove(&(*b_textFileData)[index], &(*b_textFileData)[index + 1], len - index); // includes null terminator
            }
        }
        constrainCursor(*b_textFileData);
        return;
    } else if (newData[0] == '\n') { // Handle newline character
        char* temp = realloc(*b_textFileData, len + 2); // Grow buffer for \n and null terminator (\0)
        if (temp) {
            *b_textFileData = temp;
            memmove(&(*b_textFileData)[index + 1], &(*b_textFileData)[index], len - index + 1); // Shift data to the right
            (*b_textFileData)[index] = '\n';
            (*cx) = 0; // Reset column to 0
            (*cy)++;   // Move to the next line
        }
        constrainCursor(*b_textFileData);
        return;
    } else if (newDataLen > 0) {
        newDataLen--;
        char* temp = realloc(*b_textFileData, len + newDataLen + 1); // grow for new data + null
        if (temp) {
            *b_textFileData = temp;
            memmove(&(*b_textFileData)[index + newDataLen], &(*b_textFileData)[index], len - index + 1); // +1 for null terminator
            memcpy(&(*b_textFileData)[index], newData, newDataLen);
            (*cx) += newDataLen; // Move cursor right
        }
        constrainCursor(*b_textFileData);
        return;
    }
}