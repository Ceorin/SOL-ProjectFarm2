#TODO
CC = gcc
CFLAGS = -std=c99 -Wall -pedantic
CPPFLAGS = -pthread

PROJECT_DIR = .
SOURCE = $(PROJECT_DIR)/src
HEADERS = $(PROJECT_DIR)/src/src_headers
BUILD_PATH = $(PROJECT_DIR)/build

TEMP_FILES = $(PROJECT_DIR)/tmp
TEST_FOLDER = $(PROJECT_DIR)/test

SOURCE_FILES := $(wildcard $(SOURCE)/*.c)
OBJECTS := $(patsubst $(SOURCE)/%.c, $(BUILD_PATH)/%.o, $(SOURCE_FILES))

.PHONY: all prepareTest clean cleanTmp cleanBuild cleanTest test1

all : prepareTest farm


#Main program
farm : $(OBJECTS) collector
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

collector : $(BUILD_PATH)/collector.o
	$(CC) $(CFLAGS) $^ -o $@

#Header dependencies (will join the general object dependency rule below)
#collector needs worker's interface
$(BUILD_PATH)/collector.o : $(HEADERS)/collector.h $(HEADERS)/workers.h 
#main needs every header
$(BUILD_PATH)/main.o : $(wildcard $(HEADERS)/*.h)
#master needs workers' interface
$(BUILD_PATH)/master.o : $(HEADERS)/master.h $(HEADERS)/workers.h
#workers needs the function as well
$(BUILD_PATH)/workers.o : $(HEADERS)/workers.h $(HEADERS)/fun_task.h

$(BUILD_PATH)/fun_task.o : $(HEADERS)/fun_task.h

#general rule
$(OBJECTS): $(BUILD_PATH)/%.o : $(SOURCE)/%.c $(HEADERS)/utils.h
	$(CC) $(CFLAGS) $< -c -I $(HEADERS) -o $@ 

#Pre-made test generation -> sets up test folder
#Right now the test is managed by test.sh
prepareTest : generafile

#Shouldn't be needed given generafile's a single-source program, but just for future use keeping track of the object should be better
generafile : $(TEST_FOLDER)/generafile.o
	$(CC) $< -o $@

$(TEST_FOLDER)/generafile.o : $(TEST_FOLDER)/generafile.c
	$(CC) $(CFLAGS) $< -c -o $@



#TODO hierarchy compilation

#TODO test and utilities
# .PHONY: clean cleanTmp cleanBuild cleanTest test1

#TODO everything
clean: cleanTmp cleanBuild cleanTest
	-rm farm
	-rm collector

cleanTmp : 
	-rm $(TEMP_FILES)/*

cleanBuild : 
	-rm $(BUILD_PATH)/*.o

cleanTest :
	-rm $(TEST_FOLDER)/*.o
	-rm -f generafile

test1 :
	test/test.sh

debugtest :
	echo $(SOURCE_FILES)
	echo $(OBJECTS)