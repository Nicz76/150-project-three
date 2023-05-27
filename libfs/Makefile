# Target library
lib := libfs.a
objs := \
	disk.o \
	fs.o

all: $(lib)

# objs := *.o *.d *.x

# Don't print the commands unless explicitly requested with `make V=1`
ifneq ($(V),1)
Q = @
V = 0
endif
## Debug flag
ifneq ($(D),1)
CFLAGS	+= -O2
else
CFLAGS	+= -fprofile-arcs -ftest-coverage -fPIC -O0
endif

# Current directory
CUR_PWD := $(shell pwd)

CC := gcc
CFLAGS := -Wall -Wextra -Werror

CFLAGS	+= -MMD

# Include dependencies
deps := $(patsubst %.o,%.d,$(objs))
-include $(deps)

## TODO: Phase 1
$(lib): $(objs)
	@echo "MAKE	$@"
	$(Q)ar rcs $@ $^

%.o: %.c
	@echo "CC	$@"
	$(Q)$(CC) $(CFLAGS) -c $<

clean:
	@echo "CLEAN	$(CUR_PWD)"
	$(Q)rm $(objs) $(deps) $(lib)

# libfs.a: fs.o disk.o
#     ar rcs libfs.a fs.o disk.o
# fs.o: fs.c fs.h
# 	gcc -Wall -Wextra -Werror -c -o fs.o fs.c
# disk.o: disk.c fisk.h
# 	gcc -Wall -Wextra -Werror -c -o disk.o disk.c

# clean:
#     rm -f $(lib)

## TODO: Phase 1
