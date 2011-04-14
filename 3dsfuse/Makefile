OBJS = fs.o crypto.o wearlevel.o helper.o fuse_glue.o main.o
PACKAGES = openssl fuse
CFLAGS = $(shell pkg-config --cflags $(PACKAGES))  -D__DARWIN_64_BIT_INO_T=0 -Wall -ggdb -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=25
LDFLAGS = $(shell pkg-config --libs $(PACKAGES)) 
OUTPUT = 3dsfuse
main: $(OBJS)
	gcc $(CFLAGS) -o $(OUTPUT) $(LIBS) $(OBJS) $(LDFLAGS)
clean:
	rm -f $(OUTPUT) $(OBJS)
