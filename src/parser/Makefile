CC=emcc
CONFIGURE=emconfigure
EMMAKE=emmake
SOURCES=$(wildcard *.c)
CFLAGS = -O2 --no-entry -sERROR_ON_UNDEFINED_SYMBOLS=0 -sFILESYSTEM=0 -sASSERTIONS=0 -sEXPORT_KEEPALIVE=1 -Wall -Wno-logical-op-parentheses
INCLUDE_FLAGS = -I"src"

all: parser.wasm

parser.wasm: $(SOURCES) ast
	$(CC) $(CFLAGS) $(SOURCES) -o parser.wasm $(INCLUDE_FLAGS)
	wasi-stub --stub-function env:__syscall_unlinkat,env:__syscall_faccessat ./parser.wasm.wasm
	mv ./"parser.wasm - stubbed.wasm" ./parser.wasm.wasm

ast ast.c ast.h: ast.prot
	wasmpg ast.prot -t . -c .

format:
	clang-format -i -style=file src/*.c src/*/*.c src/*/*.h

clean:
	rm -f *.wasm \
		  ast.c \
		  ast.h