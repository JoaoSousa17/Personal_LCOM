# Nome do executável
TARGET = fightlist

# Compilador
CC = gcc

# Flags
CFLAGS = -Wall -Wextra -std=c99 -I./terminalVersionArchives -I./utils -I./gameLogic -I./leaderboard -I./storedData

# Ficheiros fonte
SRC = main.c \
      gameLogic/jogo.c \
      terminalVersionArchives/terminalGestion.c \
      terminalVersionArchives/utils.c \
      leaderboard/leaderboard.c \

# Comando de build
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

# Limpeza
clean:
	rm -f $(TARGET)
	rm -f leaderboard.txt
