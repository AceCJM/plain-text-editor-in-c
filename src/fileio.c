#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* loadFile(const char* filePath) {
    FILE* p_file; // File Pointer
    p_file = fopen(filePath, "rb"); // Open file in read only binary mode

    // Check if Pointer is NULL
    if (p_file == NULL) {
        perror("Error opening file!");
        return NULL;
    }

    // fseek end of file for to get size
    fseek(p_file, 0L, SEEK_END);
    long fileSize = ftell(p_file);
    rewind(p_file); // go back to beginning of file

    // check if fileSize read was correct
    if (fileSize == -1) {
        perror("Error getting file size");
        fclose(p_file);
        return NULL;
    }

    char* b_file = (char*)malloc(fileSize + 1); // Creates buffer for file contents of fileSize + a null terminator

    // Check if buffer was allocated
    if (!b_file) {
        fclose(p_file);
        fputs("Memory allocation failed", stderr);
        return NULL;
    }

    size_t bytes_read = fread(b_file, 1, fileSize, p_file); // Reads entire file into the buffer

    // check if correct amount of bytes were read
    if (bytes_read != fileSize) {
        fputs("Error reading file", stderr);
        free(b_file);
        fclose(p_file);
        return NULL;
    }

    b_file[fileSize] = '\0'; // Null terminate the buffer

    fclose(p_file); // Close file if all was successful

    return b_file; // return the buffer
}

int saveFile(const char* filePath, char* p_fileData) {
    FILE* p_file; // File Pointer
    p_file = fopen(filePath, "wb"); // Open file in write binary mode
    const char* str = p_fileData;
    // Check if Pointer is NULL
    if (p_file == NULL) {
        fclose(p_file);
        perror("Error opening file!");
        return -1;
    }
    
    // null terminator to end of file
    size_t len = strlen(str);
    char* new_str = (char*)malloc(len + 1);
    if (!new_str) {
        fclose(p_file);
        perror("Error saving file");
        return -1;
    }
    strcpy(new_str, str);
    new_str[len] = '\0'; // Remove the extra newline addition
    if (new_str[len + 1] != '\n') new_str[len + 1] = '\n';
    // Write bytes to file
    size_t bytes_written = fwrite(new_str, sizeof(char), len+1, p_file); // get number of bytes written to the file
    fclose(p_file);
    printf("\nNumber of bytes written: %ld\n", bytes_written);
    return 0;
}