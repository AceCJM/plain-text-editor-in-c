#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "fileio.h"
#include "editor.h"

// Macros
#define gotoxy(l, c) printf("\033[%d;%dH", (l), (c)) // move terminal cursor
#define clear_screen() printf("\033[2J\033[H")  // clear screen

char str_val;
char *str = &str_val;

// Forward declarations
void get_terminal_size(int* rows, int* cols);

extern int *cx;
extern int *cy;

// Editor message flags
int confirm_exit = 0;
int file_saved = 0; 

struct termios original_termios;

void disable_raw_mode() {
    // Restore the original terminal attributes
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
}

void enable_raw_mode() {
    // Get the current terminal attributes
    tcgetattr(STDIN_FILENO, &original_termios);
    // Ensure disableRawMode is called on exit
    atexit(disable_raw_mode);

    struct termios raw = original_termios;
    raw.c_lflag &= ~(ECHO | ICANON);

    // Use the cfmakeraw function to set all necessary raw mode flags
    cfmakeraw(&raw);

    // Apply the new settings immediately (TCSANOW)
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void print_line_numbers(int numLines) {
    int i = 0;
    while (i < numLines) {
        printf("%c\n", i);
        i++;
    }
}

// Function to count lines in text
int count_lines(const char* text) {
    if (!text || !*text) return 1;
    int lines = 1;
    for (const char* p = text; *p; p++) {
        if (*p == '\n') lines++;
    }
    return lines;
}

// Function to get line length at specific row
int get_line_length(const char* text, int row) {
    if (!text) return 0;
    
    int current_row = 0;
    int line_start = 0;
    int i = 0;
    
    while (text[i] && current_row <= row) {
        if (text[i] == '\n' || text[i] == '\0') {
            if (current_row == row) {
                return i - line_start;
            }
            current_row++;
            line_start = i + 1;
        }
        i++;
    }
    
    // If we reach end and it's the target row
    if (current_row == row) {
        return i - line_start;
    }
    
    return 0;
}

// Function to constrain cursor to text bounds
void constrain_cursor(const char* text) {
    int total_lines = count_lines(text);
    
    // Constrain row
    if (*cy >= total_lines) *cy = total_lines - 1;
    if (*cy < 0) *cy = 0;
    
    // Constrain column
    int line_len = get_line_length(text, *cy);
    if (*cx >= line_len) *cx = line_len > 0 ? line_len : 0;
    if (*cx < 0) *cx = 0;
}

// Function to print text with proper line endings for terminal display
void print_text(const char* text) {
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
void print_editor_message(const char* text) {
    if (!text) return;
    int rows, cols;
    get_terminal_size(&rows, &cols);

    // Move cursor to bottom row at column 1 and print message
    printf("\x1b[%d;1H", rows);
    printf("\x1b[K");
    printf("%s", text);

    // Move cursor back to text area
    gotoxy(*cy + 1, *cx + 1);
}

// Function to get index in text from cursor position
int get_index_from_cursor(const char* text, int row, int col) {
    if (!text) return 0;
    int current_row = 0;
    int index = 0;
    while (text[index] && current_row < row) {
        if (text[index] == '\n') {
            current_row++;
        }
        index++;
    }
    index += col;
    return index;
}

// Function to get terminal size
void get_terminal_size(int* rows, int* cols) {
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
    long file_size;
    char *b_text_file_data = load_file(file_path);
    file_size = strlen(b_text_file_data);
    if (b_text_file_data) {
        printf("File size %ld bytes\n", file_size);
        printf("File contents:\n%.*s\n", (int)file_size, b_text_file_data);
    }
    // Initialize str with loaded data
    if (b_text_file_data) {
        str = malloc(file_size + 1);
        if (str) {
            memcpy(str, b_text_file_data, file_size);
            str[file_size] = '\0';
        }
        free(b_text_file_data);
    } else {
        str = malloc(1);
        str[0] = '\0';
    }

    char c;
    char seq[3];  // for escape sequences
    char typedChar;
    
    enable_raw_mode();
    
    // Main loop for editor
    while (running)
    {   
        clear_screen();
        int num_lines = count_lines(str);
        gotoxy(0,0);
        // print_line_numbers(num_lines);
        print_text(str);
        printf("\r\n");  // Ensure we end with proper line ending
        // Print editor message
        if (confirm_exit > 0) print_editor_message("Are you sure you want to exit?\n\rFile will not be saved\n");
        if (file_saved == 1) print_editor_message("File Saved");
        
        // Position cursor at text area (after line numbers)
        constrain_cursor(str);
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
            save_file(file_path, str);
            file_saved = 1;
        } else if (c == 24) {  // Ctrl+X Exit
            if (file_saved == 1) {
                running = 0;
            }
            else if (confirm_exit < 1) {
                confirm_exit++;
            } else {
                running = 0;
            }
        } else if (c == 127) { // Delete
            alterCharBuffer("\b");
        } else if (c == 8) { // Backspace
            alterCharBuffer("\b");
        } else {
            typedChar = (char)c;
            alterCharBuffer(&typedChar);
        }
    }
    clear_screen();
    printf("\rProgram Exited\0");
    return EXIT_SUCCESS;
}
