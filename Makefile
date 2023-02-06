#TODO
CC = gcc
CFLAGS = -std=c99 -Wall -pedantic
CPPFLAGS = -pthread

SOURCE = src
HEADERS = src/src_headers
BUILD_PATH = build

TEMP_FILES = tmp
TEST_FOLDER = test

SOURCE_FILES := $(wildcard $(SOURCE)/*.c)
#TODO Fix this (v)
OBJECTS := $(SOURCE_FILES:.c=.o)

all : prepareTest farm

#Pre-made test generation -> sets up test folder
#TODO add arguments or something to the execution!
prepareTest : generafile
	./generafile.exe 

#Shouldn't be needed given generafile's a single-source program, but just for future use keeping track of the object should be better
generafile : $(BUILD_PATH)/generafile.o
	$(CC) $< -o $@

$(BUILD_PATH)/generafile.o : generafile.c
	$(CC) $(CFLAGS) $< -c $@

#Main program
farm : $(OBJECTS)


#TODO hierarchy compilation

#TODO test and utilities
# .PHONY: clean cleanTmp cleanBuild cleanTest test1
