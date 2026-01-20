#include <stdio.h>
void disable_raw_mode();
void enable_raw_mode();
void get_terminal_size(int* rows, int* cols);
int count_lines(const char* text);
int get_line_length(const char* text, int row);
void print_editor_message(const char* text);
void print_text(const char* text);
void constrain_cursor(const char* text);
int get_index_from_cursor(const char* text, int row, int col);
void alter_char_buffer(char** b_textFileData, int index, char newData[]);
void handle_inputs(char *input, char *seq[]);