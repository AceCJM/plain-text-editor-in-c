#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

// Cursor position variables
int cx_val = 0;
int cy_val = 0;
int* cx = &cx_val;  // cursor column (x position)
int* cy = &cy_val;  // cursor row (y position)
int cursorIndex;

// Extern file data buffer
extern char *str;

// Function to count lines in text
int countLines() {
    if (!*str) return 1;
    int lines = 1;
    for (const char* p = str; *p; p++) {
        if (*p == '\n') lines++;
    }
    return lines;
}

// Function to get line length at specific row
int getLineLength() {
    if (!*str) return 0;
    
    int currentRow = 0;
    int lineStart = 0;
    int i = 0;
    
    while (str[i] && currentRow <= *cy) {
        if (str[i] == '\n' || str[i] == '\0') {
            if (currentRow == *cy) {
                return i - lineStart;
            }
            currentRow++;
            lineStart = i + 1;
        }
        i++;
    }
    
    // If we reach end and it's the target row
    if (currentRow == *cy) {
        return i - lineStart;
    }
    
    return 0;
}

// Function to constrain cursor to text bounds
void constrainCursor() {
    if (!*str) {
        *cx = 0;
        *cy = 0;
        return;
    }

    int totalLines = countLines();

    // Constrain row
    if (*cy >= totalLines) *cy = totalLines - 1;
    if (*cy < 0) *cy = 0;

    // Constrain column
    int lineLen = getLineLength();
    if (*cx >= lineLen) *cx = lineLen;
    if (*cx < 0) *cx = 0;
}

// Function to get index in text from cursor position
int getIndexFromCursor() {
    if (!*str) return 0;
    int currentRow = 0;
    int index = 0;
    while (str[index] && currentRow < *cy) {
        if (str[index] == '\n') {
            currentRow++;
        }
        index++;
    }
    index += *cx;
    return index;
}

void alterCharBuffer(char newData[]) {
    size_t len = strlen(str);
    cursorIndex = getIndexFromCursor();
    if (cursorIndex < 0) {
        return;  // Safety check
    }
    if (cursorIndex > (int)len) {
        cursorIndex = len;  // Clamp index to buffer length
    }

    size_t newDataLen = strlen(newData);

    if (newData[0] == '\b') { // Backspace detection
        if (cursorIndex > 0) {
            (*cx)--; // Move cursor left
            if (*cx < 0) {
                (*cy)--;
                *cx = getLineLength();
            }
            char* temp = realloc(str, len); // shrink by 1
            if (temp) {
                str = temp;
                memmove(&str[cursorIndex - 1], &str[cursorIndex], len - cursorIndex + 1); // +1 to include null terminator
            }
        }
        constrainCursor();
        return;

    } else if (newData[0] == 127) { // Delete detection
        if (cursorIndex < (int)len) {
            char* temp = realloc(str, len); // shrink by 1
            if (temp) {
                str = temp;
                memmove(&str[cursorIndex], &str[cursorIndex + 1], len - cursorIndex); // includes null terminator
            }
        }
        constrainCursor();
        return;
    } else if (newData[0] == '\r') { // Handle newline character
        char* temp = realloc(str, len + 2); // Grow buffer for \n and null terminator (\0)
        if (temp) {
            str = temp;
            memmove(&str[cursorIndex + 1], &str[cursorIndex], len - cursorIndex + 1); // Shift data to the right
            str[cursorIndex] = '\n';
            (*cx) = 0; // Reset column to 0
            (*cy)++;   // Move to the next line
        }
        constrainCursor();
        return;
    } else if (newData[0] == 9) { // Handle tab character change 
        char* temp = realloc(str, len + 4 + 5); // grow for tab space (4 spaces) + null
        if (temp) {
            str = temp;
            char tempData[] = "    ";
            memmove(&str[cursorIndex + 4], &str[cursorIndex], len - cursorIndex + 1);
            memcpy(&str[cursorIndex], tempData, 4);
            (*cx) += 4;
        }
        constrainCursor();
        return;
    } else if (newDataLen > 0) {
        char* temp = realloc(str, len + newDataLen + 1); // grow for new data + null
        if (temp) {
            str = temp;
            memmove(&str[cursorIndex + newDataLen], &str[cursorIndex], len - cursorIndex + 1); // +1 for null terminator
            memcpy(&str[cursorIndex], newData, newDataLen);
            (*cx) += 1; // Move cursor right
        }
        constrainCursor();
        return;
    }
}

