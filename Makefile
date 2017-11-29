CC=gcc -c
LIB_DIR=$(realpath ./lib)
INC_DIR=$(realpath ./include)
BIN_DIR=$(realpath ./bin)
SRC_DIR=$(realpath ./src)

SRC=$(wildcard $(SRC_DIR)/*.c)
BIN=$(addprefix $(BIN_DIR)/, $(notdir $(SRC:.c=.o)))
LIB=$(LIB_DIR)/libt2fs.a
CFLAGS=-Wall -lmath -I$(INC_DIR) -std=gnu99 -m32 -O2

all: $(BIN)
	ar -cvq $(LIB) $^

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $< $(CFLAGS)

.PHONY: install
.PHONY: debug
.PHONY: clean

install: $(LIB) $(INC_DIR)/t2fs.h
	@install -t /usr/lib $(LIB)
	@install -t /usr/include $(INC_DIR)/*

debug:
	@echo $(SRC)
	@echo $(BIN)
	@echo $(LIB_DIR)
	@echo $(INC_DIR)
	@echo $(BIN_DIR)
	@echo $(SRC_DIR)

clean:
	rm -rf $(LIB_DIR)/*.a $(BIN_DIR)/*.o $(SRC_DIR)/*~ $(INC_DIR)/*~ *~
