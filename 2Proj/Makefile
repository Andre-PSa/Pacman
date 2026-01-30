# Compiler variables
CC = gcc
CFLAGS = -g -Wall -Wextra -std=c17 -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lncurses

# Directory variables
CLIENT_DIR = client
SERVER_DIR = server

C_SRC_DIR = $(CLIENT_DIR)/src/client
S_SRC_DIR = $(SERVER_DIR)/src

C_OBJ_DIR = $(CLIENT_DIR)/obj
S_OBJ_DIR = $(SERVER_DIR)/obj

C_BIN_DIR = $(CLIENT_DIR)/bin
S_BIN_DIR = $(SERVER_DIR)/bin

C_INCLUDE_DIR = $(CLIENT_DIR)/include
S_INCLUDE_DIR = $(SERVER_DIR)/include

# Executables
CLIENT = client
TARGET = Pacmanist

# Client objects
C_SRCS = client_main.c debug.c api.c display.c
OBJS_CLIENT = $(addprefix $(C_OBJ_DIR)/,$(C_SRCS:.c=.o))

# Server objects
S_SRCS = game.c display.c board.c parser.c
OBJS_SERVER = $(addprefix $(S_OBJ_DIR)/,$(S_SRCS:.c=.o))

# Make targets
all: client server

client: $(C_BIN_DIR)/$(CLIENT)

server: $(S_BIN_DIR)/$(TARGET)

$(C_BIN_DIR)/$(CLIENT): $(OBJS_CLIENT) | folders
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(S_BIN_DIR)/$(TARGET): $(OBJS_SERVER) | folders
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# dont include LDFLAGS in the end, to allow compilation on macos
$(C_OBJ_DIR)/%.o: $(C_SRC_DIR)/%.c | folders
	$(CC) -I$(C_INCLUDE_DIR) $(CFLAGS) -c $< -o $@

$(S_OBJ_DIR)/%.o: $(S_SRC_DIR)/%.c | folders
	$(CC) -I$(S_INCLUDE_DIR) $(CFLAGS) -c $< -o $@

# Create folders
folders:
	mkdir -p $(C_OBJ_DIR) $(S_OBJ_DIR) $(C_BIN_DIR) $(S_BIN_DIR)

# Clean object files and executable
clean:
	rm -rf $(C_OBJ_DIR)/*.o $(S_OBJ_DIR)/*.o
	rm -rf $(C_BIN_DIR)/* $(S_BIN_DIR)/*

# indentify targets that do not create files
.PHONY: all clean folders client server
