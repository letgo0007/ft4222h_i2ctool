#Copyright @2018 Nick Yang

###Compiler
CC = gcc

###C source file
CSOURCE= \
fti2c.c\
cli.c

###C include path
CINCLUDE = -I.

###C flags
CFLAG = -Wall -Wno-int-to-void-pointer-cast

###Lib search path
LIBPATH = -Wl,-rpath,/usr/local/lib

###Lib flags, make sure libft4222.dylib is in /usr/local/lib
LIBFLAG = -L. -lft4222

###TARGET
TARGET = fti2c

all:
	$(CC) $(CSOURCE) $(CINCLUDE) $(CFLAG) $(LIBPATH) $(LIBFLAG) -o$(TARGET)

debug: all
	chmod +x ./test.sh
	./test.sh

clean: 
	rm -f $(TARGET)
