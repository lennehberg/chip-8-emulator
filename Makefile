CC=gcc
CFLAGS= -Wall -Wextra -g


DISASSMBLERDIR = src/Disassmbler
DISASSMBLERSRC = $(DISASSMBLERDIR)/disassmbler.c

DISASSMBLER= disassmbler

SHELL_DIR = src/EmulatorShell
SHELLSRC = $(SHELL_DIR)/emulatorShell.c
SHELLINC = $(SHELL_DIR)/emulatorShell.h
SHELLOBJ = $(SHELLSRC:.c=.o)

TARGETS= $(DISASSMBLER)

TEST_SHELL = test_shell
TEST_DIR = tests
TEST_SHELL_SRC = $(TEST_DIR)/shell_tests.c
TEST_SHELL_OBJ = $(SHELLOBJ)

TEST_TARGETS = $(TEST_SHELL)

all: $(TARGETS)


$(DISASSMBLER) : $(DISASSMBLERSRC)
	$(CC) $(CFLAGS) -o $@ $^

test: $(TEST_TARGETS)

$(TEST_SHELL) : $(TEST_SHELL_OBJ) $(TEST_SHELL_SRC)
	$(CC) $(CCFLAGS) -o $@ $^

%.o : %.c
	$(CC) $(CCFLAGS) -c -o $@ $<

clean:
	rm $(TARGETSOBJ) $(TARGETS)