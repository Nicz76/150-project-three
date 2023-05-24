# Target library
lib := libfs.a

all: $(lib)

objs := *.o *.d *.x

CC := gcc
CFLAGS := -Wall -Wextra -Werror

## TODO: Phase 1
libfs.a: fs.o disk.o
    ar rcs libfs.a fs.o disk.o
fs.o: fs.c fs.h
	gcc -Wall -Wextra -Werror -c -o fs.o fs.c
disk.o: disk.c fisk.h
	gcc -Wall -Wextra -Werror -c -o disk.o disk.c

clean:
    rm -f $(lib)

## TODO: Phase 1
