#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#include "fileio.h"
#include "editor.h"

// Macros
#define gotoxy(l, c) printf("\033[%d;%dH", (l), (c)) // move terminal cursor
#define clear_screen() printf("\033[2J\033[H")  // clear screen

// Cursor position variables
int cx_val = 0;  // cursor column (x position)
int cy_val = 0;  // cursor row (y position)
int* cx = &cx_val;
int* cy = &cy_val;

// Editor message flags
int confirmExit = 0;
int fileSaved = 0; 

struct termios original_termios;

void disableRawMode() {
    // Restore the original terminal attributes
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
}

void enableRawMode() {
    // Get the current terminal attributes
    tcgetattr(STDIN_FILENO, &original_termios);
    // Ensure disableRawMode is called on exit
    atexit(disableRawMode);

    struct termios raw = original_termios;
    raw.c_lflag &= ~(ECHO | ICANON);

    // Use the cfmakeraw function to set all necessary raw mode flags
    cfmakeraw(&raw);

    // Apply the new settings immediately (TCSANOW)
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

int running = 1;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    // turn off line buffering
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    

    // Declarations for loading the text file
    char filePath[strlen(argv[1]) + 1];
    strcpy(filePath, argv[1]);
    long fileSize;
    char* p_fileData = loadFile(filePath);
    fileSize = strlen(p_fileData);
    if (p_fileData) {
        printf("File size %ld bytes\n", fileSize);
        printf("File contents:\n%.*s\n", (int)fileSize, p_fileData);
    }

    // Initialize b_textFileData with loaded data
    char *b_textFileData;
    if (p_fileData) {
        b_textFileData = malloc(fileSize + 1);
        if (b_textFileData) {
            memcpy(b_textFileData, p_fileData, fileSize);
            b_textFileData[fileSize] = '\0';
        }
        free(p_fileData);
    } else {
        b_textFileData = malloc(1);
        b_textFileData[0] = '\0';
    }

    enableRawMode();

    char c;
    char seq[3];  // for escape sequences
    
    // Main loop for editor
    while (running)
    {   
        clear_screen();
        int numLines = countLines(b_textFileData);
        gotoxy(0,0);
        printText(b_textFileData);
        printf("\r\n");  // Ensure we end with proper line ending
        // Print editor message
        if (confirmExit > 0) printEditorMessage("Are you sure you want to exit?\n\rFile will not be saved\n");
        if (fileSaved == 1) printEditorMessage("File Saved");
        
        // Position cursor at text area (after line numbers)
        constrainCursor(b_textFileData);
        gotoxy(*cy + 1, *cx + 1);  // +1 because terminal coordinates start at 1,1
        
        // Read input
        ssize_t nread = read(STDIN_FILENO, &c, 1);
        if (nread == -1) continue;
        
        if (c == '\x1b') {  // Escape sequence start
            if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;
            
            if (seq[0] == '[') {
                switch (seq[1]) {
                    case 'A':  // Up arrow
                        (*cy)--;
                        break;
                    case 'B':  // Down arrow
                        (*cy)++;
                        break;
                    case 'C':  // Right arrow
                        (*cx)++;
                        break;
                    case 'D':  // Left arrow
                        (*cx)--;
                        break;
                }
            }
        } else if (c == 19) { // Ctrl+S Save 
            saveFile(filePath, b_textFileData);
            fileSaved = 1;
        } else if (c == 24) {  // Ctrl+X Exit
            if (fileSaved == 1) {
                running = 0;
            }
            else if (confirmExit < 1) {
                confirmExit++;
            } else {
                running = 0;
            }
        } else if (c == '\r') {  // Enter key
            char temp[2] = {'\n', '\0'};
            int index = getIndexFromCursor(b_textFileData, *cy, *cx);
            alterCharBuffer(&b_textFileData, index, temp);
        } else if (c == 127) {  // Backspace
            int index = getIndexFromCursor(b_textFileData, *cy, *cx);
            size_t len = strlen(b_textFileData);
            if (index > 0) {
                alterCharBuffer(&b_textFileData, index, "\b");
            }
        } else {
            if (confirmExit > 0) confirmExit = 0;
            if (fileSaved) fileSaved = 0;
            int index = getIndexFromCursor(b_textFileData, *cy, *cx);
            alterCharBuffer(&b_textFileData, index, &c);
        }
    }
    clear_screen();
    printf("\rProgram Exited\n");
    return EXIT_SUCCESS;
}