#ifndef WASM_H
#define WASM_H
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>


uint8_t buffer[] = {0x00};

void wasm_minimal_protocol_write_args_to_buffer(uint8_t *ptr) {
    memcpy(ptr, buffer, sizeof(buffer));
}
void wasm_minimal_protocol_send_result_to_host(const uint8_t *ptr, size_t len) {
    printf("Buffer length: %zd\n", len);
    for (size_t i = 0; i < len; i++) {
        printf("%02x ", ptr[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    free(ptr);
}
#endif // WASM_H
