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
    }

    enable_raw_mode();

    char c;
    char *seq[3];  // for escape sequences
    
    // Main loop for editor
    while (running)
    {   
        clear_screen();
        int numLines = count_lines(b_text_file_data);
        gotoxy(0,0);
        print_text(b_text_file_data);
        printf("\r\n");  // Ensure we end with proper line ending
        // Print editor message
        if (confirm_exit_val > 0) print_editor_message("Are you sure you want to exit?\n\rFile will not be saved\n");
        if (file_saved_val == 1) print_editor_message("File Saved");
        
        // Position cursor at text area (after line numbers)
        constrain_cursor(b_text_file_data);
        gotoxy(*cy + 1, *cx + 1);  // +1 because terminal coordinates start at 1,1
        
        // Read input
        ssize_t nread = read(STDIN_FILENO, &c, 1);
        if (nread == -1) continue;
        
        handle_inputs(&c, seq);
    }
    clear_screen();
    printf("\rProgram Exited\0");
    return EXIT_SUCCESS;
}
