SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

EXE := $(BIN_DIR)/prodcons
UTESTS := $(BIN_DIR)/conqueue $(BIN_DIR)/conlfqueue $(BIN_DIR)/conbst
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

CPPFLAGS := -Iinclude -MMD -MP
#CPPFLAGS += -D_VERBOSE
#CPPFLAGS += -D_LOCK_FREE_QUEUE

CFLAGS := -Wall -pthread
#CFLAGS += -g

LDFLAGS := -Llib

LDLIBS :=

.PHONY: all clean

all: $(EXE) $(UTESTS)

$(EXE): $(OBJ) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

$(BIN_DIR)/conqueue: $(OBJ_DIR)/t_conqueue.o | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/t_conqueue.o: $(SRC_DIR)/conqueue.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) -D_UTEST $(CFLAGS) -c $< -o $@

$(BIN_DIR)/conlfqueue: $(OBJ_DIR)/t_conlfqueue.o | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/t_conlfqueue.o: $(SRC_DIR)/conlfqueue.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) -D_UTEST $(CFLAGS) -c $< -o $@

$(BIN_DIR)/conbst: $(OBJ_DIR)/t_conbst.o | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/t_conbst.o: $(SRC_DIR)/conbst.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) -D_UTEST $(CFLAGS) -c $< -o $@

clean:
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR)

-include $(OBJ:.o=.d)
