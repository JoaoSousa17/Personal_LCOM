# name of the program (Minix service)
PROG=proj

# source code files to be compiled
SRCS = keyboard.c videocard.c proj.c font.c mouse.c utils.c leaderboard.c game.c timer.c letter_rain.c sprite.c

# additional compilation flags
# "-Wall -Wextra -Werror -I . -std=c11 -Wno-unused-parameter" are already set
CFLAGS += -pedantic -DPROJ

# list of library dependencies (for Lab 2, only LCF library)
DPADD += ${LIBLCF}
LDADD += -llcf

# include LCOM's makefile that does all the "heavy lifting"
.include <minix.lcom.mk>
