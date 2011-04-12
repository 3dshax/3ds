CC=gcc
INCLUDES=include
CFLAGS=-c -Wall -O2 -std=c99
LDFLAGS=
SOURCES=src/decrypt.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=decrypt

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) -I$(INCLUDES) $(CFLAGS) $< -o $@
