VERSION := $(shell grep '^version' typst.toml | awk -F ' = ' '{print $$2}' | tr -d '"')
PACKAGE_NAME := $(shell grep '^name' typst.toml | awk -F ' = ' '{print $$2}' | tr -d '"')
TARGET_DIR=./$(PACKAGE_NAME)/$(VERSION)


check:
	typst compile ./lib.typ
	rm ./lib.pdf

wasm:
	$(MAKE) -C ./src/parser/Makefile

module:
	mkdir -p $(TARGET_DIR)
	mkdir -p $(TARGET_DIR)/src
	mkdir -p $(TARGET_DIR)/src/parser
	cp ./typst.toml $(TARGET_DIR)/typst.toml
	cp ./LICENSE $(TARGET_DIR)/
	cp ./src/lib.typ $(TARGET_DIR)/src/lib.typ
	cp ./src/parser/protocol.typ $(TARGET_DIR)/src/parser/protocol.typ
	cp ./src/parser/smiles.wasm $(TARGET_DIR)/src/parser/smiles.wasm
	awk '{gsub("https://typst.app/universe/package/$(PACKAGE_NAME)", "https://github.com/Typsium/$(PACKAGE_NAME)");print}' ./README.md > $(TARGET_DIR)/README.md

all: wasm module
