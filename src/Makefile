# Makefile for the LCOM Project

# name of the program (Minix service)
PROG=proj

# source code files to be compiled
SRCS = main.c videocard.c keyboard.c

# additional compilation flags
# "-Wall -Wextra -Werror -I . -std=c11 -Wno-unused-parameter" are already set
CFLAGS += -pedantic

# additional include directories
CPPFLAGS += -I./src

# list of library dependencies
DPADD += ${LIBLCF}
LDADD += -llcf

# include LCOM's makefile that does all the "heavy lifting"
.include <minix.lcom.mk> 