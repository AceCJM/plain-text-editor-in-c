#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "fileio.h"

// Macros
#define gotoxy(l, c) printf("\033[%d;%dH", (l), (c)) // move terminal cursor
#define clear_screen() printf("\033[2J\033[H")  // clear screen

// Forward declarations
void getTerminalSize(int* rows, int* cols);

// Cursor position variables
<<<<<<< HEAD
int cx_val = 0;  // cursor column (x position)
int cy_val = 0;  // cursor row (y position)
int confirm_exit_val = 0;
int file_saved_val = 0; 
int running_val = 1;
char file_path_val;

int* cx = &cx_val;
int* cy = &cy_val;
int* confirm_exit = &confirm_exit_val;
int* file_saved = &file_saved_val;
int* running = &running_val;
char* file_path = &file_path_val;
char *b_text_file_data;

=======
int cx = 0;  // cursor column (x position)
int cy = 0;  // cursor row (y position)

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

void printLineNumbers(int numLines) {
    int i = 0;
    while (i < numLines) {
        printf("%c\n", i);
        i++;
    }
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

// Function to constrain cursor to text bounds
void constrainCursor(const char* text) {
    int totalLines = countLines(text);
    
    // Constrain row
    if (cy >= totalLines) cy = totalLines - 1;
    if (cy < 0) cy = 0;
    
    // Constrain column
    int lineLen = getLineLength(text, cy);
    if (cx >= lineLen) cx = lineLen > 0 ? lineLen : 0;
    if (cx < 0) cx = 0;
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
    gotoxy(cy + 1, cx + 1);
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

int running = 1;
>>>>>>> parent of b78e7dd (Modularizes editor logic and improves buffer handling)

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    // turn off line buffering
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    

    // Declarations for loading the text file
    char file_path[strlen(argv[1]) + 1];
    strcpy(file_path, argv[1]);
    long fileSize;
    char* p_fileData = load_file(file_path);
    fileSize = strlen(p_fileData);
    if (p_fileData) {
        printf("File size %ld bytes\n", fileSize);
        printf("File contents:\n%.*s\n", (int)fileSize, p_fileData);
    }

<<<<<<< HEAD
    // Initialize b_text_file_data with loaded data
    if (p_fileData) {
        b_text_file_data = malloc(fileSize + 1);
        if (b_text_file_data) {
            memcpy(b_text_file_data, p_fileData, fileSize);
            b_text_file_data[fileSize] = '\0';
        }
        free(p_fileData);
    } else {
        b_text_file_data = malloc(1);
        b_text_file_data[0] = '\0';
=======
    // Initialize str with loaded data
    char *str;
    if (p_fileData) {
        str = malloc(fileSize + 1);
        if (str) {
            memcpy(str, p_fileData, fileSize);
            str[fileSize] = '\0';
        }
        free(p_fileData);
    } else {
        str = malloc(1);
        str[0] = '\0';
>>>>>>> parent of b78e7dd (Modularizes editor logic and improves buffer handling)
    }

    enable_raw_mode();

    char c;
    char *seq[3];  // for escape sequences
    
    // Main loop for editor
    while (running)
    {   
        clear_screen();
<<<<<<< HEAD
        int numLines = count_lines(b_text_file_data);
        gotoxy(0,0);
        print_text(b_text_file_data);
=======
        int numLines = countLines(str);
        printLineNumbers(numLines);
        gotoxy(0,0);
        printText(str);
>>>>>>> parent of b78e7dd (Modularizes editor logic and improves buffer handling)
        printf("\r\n");  // Ensure we end with proper line ending
        // Print editor message
        if (confirm_exit_val > 0) print_editor_message("Are you sure you want to exit?\n\rFile will not be saved\n");
        if (file_saved_val == 1) print_editor_message("File Saved");
        
        // Position cursor at text area (after line numbers)
<<<<<<< HEAD
        constrain_cursor(b_text_file_data);
        gotoxy(*cy + 1, *cx + 1);  // +1 because terminal coordinates start at 1,1
=======
        constrainCursor(str);
        gotoxy(cy + 1, cx + 1);  // +1 because terminal coordinates start at 1,1
>>>>>>> parent of b78e7dd (Modularizes editor logic and improves buffer handling)
        
        // Read input
        ssize_t nread = read(STDIN_FILENO, &c, 1);
        if (nread == -1) continue;
        
<<<<<<< HEAD
        handle_inputs(&c, seq);
=======
        if (c == '\x1b') {  // Escape sequence start
            if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;
            
            if (seq[0] == '[') {
                switch (seq[1]) {
                    case 'A':  // Up arrow
                        cy--;
                        break;
                    case 'B':  // Down arrow
                        cy++;
                        break;
                    case 'C':  // Right arrow
                        cx++;
                        break;
                    case 'D':  // Left arrow
                        cx--;
                        break;
                }
            }
        } else if (c == 19) { // Ctrl+S Save 
            saveFile(filePath, str);
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
            int index = getIndexFromCursor(str, cy, cx);
            size_t len = strlen(str);
            char *temp = realloc(str, len + 2);  // +1 for \n, +1 for null terminator
            if (temp) {
                str = temp;
                // Shift characters after cursor to make room for \n
                memmove(&str[index + 1], &str[index], len - index + 1);
                str[index] = '\n';
                // Move cursor to start of new line
                cy++;
                cx = 0;
            }
        } else if (c == 127) {  // Backspace
            int index = getIndexFromCursor(str, cy, cx);
            // Move cursor before editing str
            if (cx > 0 && (cx - 1) > 0) {
                cx--;
            } else if (cy > 0) {
                cy--;
                cx = getLineLength(str, cy);
            }
            size_t len = strlen(str);
            if (index > 0) {
                // Shift characters left to overwrite the character before cursor
                memmove(&str[index - 1], &str[index], len - index + 1);
                // Shrink memory since we removed a character
                char *temp = realloc(str, len);  // len is now the new length after memmove
                if (temp) {
                    str = temp;
                }
            }
        } else {
            if (confirmExit > 0) confirmExit = 0;
            if (fileSaved) fileSaved = 0;
            int index = getIndexFromCursor(str, cy, cx);
            size_t len = strlen(str);
            char *temp = realloc(str, len + 2);  // +1 for new char, +1 for null terminator
            if (temp) {
                str = temp;
                // Shift characters after cursor to make room for new character
                memmove(&str[index + 1], &str[index], len - index + 1);
                str[index] = c;
                cx++;  // Move cursor right after insertion
            }
        }
>>>>>>> parent of b78e7dd (Modularizes editor logic and improves buffer handling)
    }
    clear_screen();
    printf("\rProgram Exited\0");
    return EXIT_SUCCESS;
}
