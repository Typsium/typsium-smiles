CC=emcc
CONFIGURE=emconfigure
EMMAKE=emmake
SOURCES=$(wildcard */*.c)
CFLAGS = -O2 --no-entry -sERROR_ON_UNDEFINED_SYMBOLS=0 -sFILESYSTEM=0 -sASSERTIONS=0 -sEXPORT_KEEPALIVE=1 -Wall -Wno-logical-op-parentheses
INCLUDE_FLAGS = -I"."


parser.wasm: smiles.c $(SOURCES) ast/protocol.c ast/protocol.h
	$(CC) $(CFLAGS) smiles.c $(SOURCES) -o smiles.wasm $(INCLUDE_FLAGS)
	wasi-stub --stub-function env:__syscall_unlinkat,env:__syscall_faccessat ./smiles.wasm
	mv ./"smiles - stubbed.wasm" ./smiles.wasm

all: parser.wasm test

ast ast/protocol.c ast/protocol.h: ast/ast.prot
	wasmpg ast/ast.prot -c ast -t .

test: $(SOURCES) ast
	gcc -g -Wall test.c $(SOURCES) -o test_parser $(INCLUDE_FLAGS) -I"./test/" -DTEST

format:
	clang-format -i -style=file *.c */*.h */*.c

clean:
	rm -f *.wasm \
		  ast/protocol.c \
		  ast/protocol.h \
		  protocol.typ
