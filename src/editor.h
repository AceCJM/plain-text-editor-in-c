#include <stdio.h>

void getTerminalSize(int* rows, int* cols);
int countLines(const char* text);
int getLineLength(const char* text, int row);
void printEditorMessage(const char* text);
void printText(const char* text);
void constrainCursor(const char* text);
int getIndexFromCursor(const char* text, int row, int col);
void alterCharBuffer(char** b_textFileData, int index, char newData[]);
