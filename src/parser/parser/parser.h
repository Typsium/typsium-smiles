#ifndef PARSER_H
#define PARSER_H

#include "ast/protocol.h"

typedef struct parser_ctx {
    size_t buffer_len;
    size_t buffer_pos;
    char *buffer;
    bool errored;
    char *error;
    int no_error_message;
} parser_ctx;

typedef enum ASTElementType {
    ALIPHATIC_ORGANIC,
    AROMATIC_ORGANIC,
    ELEMENT_SYMBOL,
    AROMATIC_SYMBOL,
    BOND,
    NUMBER,
    CHAR,
    BRACKET_ATOM,
    CHARGE,
    CHIRAL,
    CLASS,
    HCOUNT,
    RINGBOND,
    BRANCHED_ATOM,
    BRANCH,
    CHAIN,
    TERMINATOR,
    SMILES
} ASTElementType;

typedef ASTElement (*ASTElementParser)(parser_ctx *ctx);

parser_ctx init_ctx(char *buffer, size_t buffer_len);
ASTElement smile(parser_ctx *ctx);

#endif // PARSER_H
