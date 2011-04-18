CC=gcc
INCLUDES=include
CFLAGS=-c -Wall -O2 -std=c99
LDFLAGS=-lcrypto
SOURCES=src/decrypt.c src/utils.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=decrypt

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) -I$(INCLUDES) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS) $(EXECUTABLE)
