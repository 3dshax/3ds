OBJS = fs.o crypto.o wearlevel.o helper.o fuse_glue.o main.o
LIBS = -lssl -lfuse
CFLAGS = -Wall -ggdb -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=22 
OUTPUT = 3dsfuse
main: $(OBJS)
	gcc -o $(OUTPUT) $(LIBS) $(OBJS)
clean:
	rm -f $(OUTPUT) $(OBJS)
