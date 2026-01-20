#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

#include "fileio.h"

extern int *cx;
extern int *cy;
extern int *confirm_exit;
extern int *file_saved;
extern int *running;
extern char *file_path;
extern char *b_text_file_data;

// Editor flags
int editor_mode = 0;



// Macros
#define gotoxy(l, c) printf("\033[%d;%dH", (l), (c)) // move terminal cursor

struct termios original_termios;

void disable_raw_mode()
{
    // Restore the original terminal attributes
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
}

void enable_raw_mode()
{
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

// Function to get terminal size
void get_terminal_size(int *rows, int *cols)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
    {
        // Fallback values if ioctl fails
        *rows = 24;
        *cols = 80;
        return;
    }
    *rows = ws.ws_row;
    *cols = ws.ws_col;
    // Ensure we have valid values
    if (*rows <= 0)
        *rows = 24;
    if (*cols <= 0)
        *cols = 80;
}

// Function to count lines in text
int count_lines(const char *text)
{
    if (!text || !*text)
        return 1;
    int lines = 1;
    for (const char *p = text; *p; p++)
    {
        if (*p == '\n')
            lines++;
    }
    return lines;
}

// Function to get line length at specific row
int get_line_length(const char *text, int row)
{
    if (!text)
        return 0;

    int currentRow = 0;
    int lineStart = 0;
    int i = 0;

    while (text[i] && currentRow <= row)
    {
        if (text[i] == '\n' || text[i] == '\0')
        {
            if (currentRow == row)
            {
                return i - lineStart;
            }
            currentRow++;
            lineStart = i + 1;
        }
        i++;
    }

    // If we reach end and it's the target row
    if (currentRow == row)
    {
        return i - lineStart;
    }

    return 0;
}

// Function to print text from editor at the bottom of screen
void print_editor_message(const char *text)
{
    if (!text)
        return;
    int rows, cols;
    get_terminal_size(&rows, &cols);

    // Move cursor to bottom row at column 1 and print message
    printf("\x1b[%d;1H", rows);
    printf("\x1b[K");
    printf("%s", text);

    // Move cursor back to text area
    gotoxy(*cy + 1, *cx + 1);
}

// Function to print text with proper line endings for terminal display
void print_text(const char *text)
{
    if (!text)
        return;
    for (const char *p = text; *p; p++)
    {
        if (*p == '\n')
        {
            printf("\r\n"); // Use \r\n for terminal display
        }
        else
        {
            putchar(*p);
        }
    }
}

// Function to constrain cursor to text bounds
void constrain_cursor(const char *text)
{
    if (!text)
    {
        *cx = 0;
        *cy = 0;
        return;
    }

    int totalLines = count_lines(text);

    // Constrain row
    if (*cy >= totalLines)
        *cy = totalLines - 1;
    if (*cy < 0)
        *cy = 0;

    // Constrain column
    int lineLen = get_line_length(text, *cy);
    if (*cx >= lineLen)
        *cx = lineLen;
    if (*cx < 0)
        *cx = 0;
}

// Function to get index in text from cursor position
int get_index_from_cursor(const char *text, int row, int col)
{
    if (!text)
        return 0;
    int currentRow = 0;
    int index = 0;
    while (text[index] && currentRow < row)
    {
        if (text[index] == '\n')
        {
            currentRow++;
        }
        index++;
    }
    index += col;
    return index;
}

void alter_char_buffer(char **b_textFileData, int index, char newData[])
{
    size_t len = strlen(*b_textFileData);

    if (index < 0)
    {
        return; // Safety check
    }
    if (index > (int)len)
    {
        index = len; // Clamp index to buffer length
    }

    size_t newDataLen = strlen(newData);

    if (newData[0] == '\b')
    { // Backspace detection
        if (index > 0)
        {
            (*cx)--; // Move cursor left
            if (*cx < 0)
            {
                (*cy)--;
                *cx = get_line_length(*b_textFileData, *cy);
            }
            char *temp = realloc(*b_textFileData, len - 1); // shrink by 2
            if (temp)
            {
                *b_textFileData = temp;
                memmove(&(*b_textFileData)[index - 1], &(*b_textFileData)[index], len - index + 1); // +1 to include null terminator
            }
        }
        constrain_cursor(*b_textFileData);
        return;
    }
    else if (newData[0] == 127)
    { // Delete detection
        if (index < (int)len)
        {
            char *temp = realloc(*b_textFileData, len); // shrink by 1
            if (temp)
            {
                *b_textFileData = temp;
                memmove(&(*b_textFileData)[index], &(*b_textFileData)[index + 1], len - index); // includes null terminator
            }
        }
        constrain_cursor(*b_textFileData);
        return;
    }
    else if (newData[0] == '\n')
    {                                                   // Handle newline character
        char *temp = realloc(*b_textFileData, len + 2); // Grow buffer for \n and null terminator (\0)
        if (temp)
        {
            *b_textFileData = temp;
            memmove(&(*b_textFileData)[index + 1], &(*b_textFileData)[index], len - index + 1); // Shift data to the right
            (*b_textFileData)[index] = '\n';
            (*cx) = 0; // Reset column to 0
            (*cy)++;   // Move to the next line
        }
        constrain_cursor(*b_textFileData);
        return;
    }
    else if (newDataLen > 0)
    {
        newDataLen--;
        char *temp = realloc(*b_textFileData, len + newDataLen + 1); // grow for new data + null
        if (temp)
        {
            *b_textFileData = temp;
            memmove(&(*b_textFileData)[index + newDataLen], &(*b_textFileData)[index], len - index + 1); // +1 for null terminator
            memcpy(&(*b_textFileData)[index], newData, newDataLen);
            (*cx) += newDataLen; // Move cursor right
        }
        constrain_cursor(*b_textFileData);
        return;
    }
}

void handle_inputs(char *input, char *seq[])
{
    if (*input == '\x1b')
    { // Escape sequence start
        if (read(STDIN_FILENO, seq[0], 1) != 1) return;
        if (read(STDIN_FILENO, seq[1], 1) != 1) return;

        if (seq[0] == '[')
        {
            switch (*seq[1])
            {
            case 'A': // Up arrow
                (*cy)--;
                break;
            case 'B': // Down arrow
                (*cy)++;
                break;
            case '*input': // Right arrow
                (*cx)++;
                break;
            case 'D': // Left arrow
                (*cx)--;
                break;
            }
        }
    }
    else if (*input == 19)
    { // Ctrl+S Save
        save_file(file_path, b_text_file_data);
        *file_saved = 1;
    }
    else if (*input == 24)
    { // Ctrl+X Exit
        if (*file_saved == 1)
        {
            *running = 0;
        }
        else if (*confirm_exit < 1)
        {
            *confirm_exit++;
        }
        else
        {
            *running = 0;
        }
    }
    else if (*input == '\r')
    { // Enter key
        char temp[2] = {'\n', '\0'};
        int index = get_index_from_cursor(b_text_file_data, *cy, *cx);
        alter_char_buffer(&b_text_file_data, index, temp);
    }
    else if (*input == 127)
    { // Backspace
        int index = get_index_from_cursor(b_text_file_data, *cy, *cx);
        size_t len = strlen(b_text_file_data);
        if (index > 0)
        {
            alter_char_buffer(&b_text_file_data, index, "\b");
        }
    }
    else
    {
        if (*confirm_exit > 0)
            *confirm_exit = 0;
        if (*file_saved)
            *file_saved = 0;
        int index = get_index_from_cursor(b_text_file_data, *cy, *cx);
        alter_char_buffer(&b_text_file_data, index, input);
    }
}