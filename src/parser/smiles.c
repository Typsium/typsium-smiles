#include "ast/protocol.h"
#include "parser/parser.h"
#include <stdio.h>

#define DEBUG(fmt, ...)                                                                            \
    char buf[1000];                                                                                \
    sprintf(buf, fmt, __VA_ARGS__);                                                                \
    wasm_minimal_protocol_send_result_to_host((uint8_t *)buf, strlen(buf));                        \
    return 1;

EMSCRIPTEN_KEEPALIVE
int parse_smiles(size_t buffer_len) {
    parse p;
    if (decode_parse(buffer_len, &p)) {
        char *error = "Failed to decode parse";
        wasm_minimal_protocol_send_result_to_host((uint8_t *)error, strlen(error));
        return 1;
    }
    parser_ctx ctx = init_ctx(p.smiles, strlen(p.smiles));
    ASTElement elem = smile(&ctx);
    free_parse(&p);
    if (ctx.errored) {
        if (ctx.error) {
            char *error = malloc(strlen(ctx.error) + strlen(p.smiles) * 2 + 20);
            sprintf(error, "Failed to parse: %s\n%s\n", ctx.error, p.smiles);
            size_t len = strlen(error);
            for (int i = 0; i < ctx.buffer_pos; i++) {
                error[len++] = ' ';
            }
            error[len++] = '^';
            error[len] = '\0';

            wasm_minimal_protocol_send_result_to_host((uint8_t *)error, len);
            free(ctx.error);
            free(error);
        } else {
            char *error = "Failed to parse";
            wasm_minimal_protocol_send_result_to_host((uint8_t *)error, strlen(error));
        }
        return 1;
    }

    result r = {.result = elem};
    if (encode_result(&r)) {
        char *error = "Failed to encode result";
        wasm_minimal_protocol_send_result_to_host((uint8_t *)error, strlen(error));
        return 1;
    }
    free_result(&r);
    return 0;
}
