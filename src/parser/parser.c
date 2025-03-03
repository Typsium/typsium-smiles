#include "ast/protocol.h"
#include "parser/parser.h"

int parse_smiles(size_t buffer_len) {
    parse p;
    if (decode_parse(buffer_len, &p)) {
        char *error = "Failed to decode parse";
        wasm_minimal_protocol_send_result_to_host((uint8_t *)error, strlen(error));
        return 1;
    }
    parser_ctx ctx = init_ctx(p.smiles, buffer_len);
    ASTElement elem = atom(&ctx);
    free_parse(&p);
    if (ctx.errored) {
        if (ctx.error) {
            wasm_minimal_protocol_send_result_to_host((uint8_t *)ctx.error, strlen(ctx.error));
            free(ctx.error);
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
