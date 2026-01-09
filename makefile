TARGET = main.c
OUTPUT = editor
SRC = ./src/
BUILD = ./build/
HEADERS = $(SRC)fileio.c $(SRC)editor.c
CC = gcc 


$(TARGET): 
	$(CC) $(SRC)$(TARGET) $(HEADERS) -o $(BUILD)$(OUTPUT)

clean:
	rm -f $(BUILD)$(TARGET)

dev:
	rm -f $(BUILD)$(TARGET)
	$(CC) $(SRC)$(TARGET) $(HEADERS) -o $(BUILD)$(OUTPUT)
