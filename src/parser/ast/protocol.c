#include "protocol.h"
int big_endian_decode(uint8_t const *buffer, int size){
    int value = 0;
    for (int i = 0; i < size; i++) {
        value |= buffer[i] << (8 * (size - i - 1));
    }
    return value;
}

void big_endian_encode(int value, uint8_t *buffer, int size) {
    for (int i = 0; i < sizeof(int); i++) {
        buffer[i] = (value >> (8 * (sizeof(int) - i - 1))) & 0xFF;
    }
}

float decode_float(uint8_t *buffer) {
	int value = big_endian_decode(buffer, TYPST_INT_SIZE);
	if (value == 0) {
		return 0.0f;
	}
	union FloatBuffer {
		float f;
		int i;
	} float_buffer;
	float_buffer.i = value;
	return float_buffer.f;
}

void encode_float(float value, uint8_t *buffer) {
	if (value == 0.0f) {
		big_endian_encode(0, buffer, TYPST_INT_SIZE);
	} else {
		union FloatBuffer {
			float f;
			int i;
		} float_buffer;
		float_buffer.f = value;
		big_endian_encode(float_buffer.i, buffer, TYPST_INT_SIZE);
	}
}

size_t list_size(void *list, size_t size, size_t (*sf)(const void*), size_t element_size) {
    size_t result = 0;
    for (int i = 0; i < size; i++) {
        result += sf(list + i * element_size);
    }
    return result;
}

size_t int_size(const void* elem) {
    return TYPST_INT_SIZE;
}
size_t float_size(const void *elem) {
    return TYPST_INT_SIZE;
}
size_t bool_size(const void *elem) {
    return TYPST_INT_SIZE;
}
size_t char_size(const void *elem) {
    return 1;
}
size_t string_size(const void *elem) {
    if (!elem || !((char *)elem)[0]) {
        return 1;
    }
    return strlen((char *)elem) + 1;
}
size_t string_list_size(char **list, size_t size) {
	size_t result = 0;
	for (size_t i = 0; i < size; i++) {
		result += string_size(list[i]);
	}
	return result;
}

void free_ASTElement(ASTElement *s) {
    if (s->value) {
        free(s->value);
    }
    for (size_t i = 0; i < s->children_len; i++) {
    free_ASTElement(&s->children[i]);
    }
    free(s->children);
}
size_t ASTElement_size(const void *s){
	return TYPST_INT_SIZE + string_size(((ASTElement*)s)->value) + TYPST_INT_SIZE + list_size(((ASTElement*)s)->children, ((ASTElement*)s)->children_len, ASTElement_size, sizeof(*((ASTElement*)s)->children));
}
int encode_ASTElement(const ASTElement *s, uint8_t *__input_buffer, size_t *buffer_len, size_t *buffer_offset) {
    size_t __buffer_offset = 0;    size_t s_size = ASTElement_size(s);
    if (s_size > *buffer_len) {
        return 2;
    }
    int err;
	(void)err;
    INT_PACK(s->type)
    STR_PACK(s->value)
    INT_PACK(s->children_len)
    for (size_t i = 0; i < s->children_len; i++) {
        if ((err = encode_ASTElement(&s->children[i], __input_buffer + __buffer_offset, buffer_len, &__buffer_offset))) {
            return err;
        }
    }

    *buffer_offset += __buffer_offset;
    return 0;
}
void free_parse(parse *s) {
    if (s->smiles) {
        free(s->smiles);
    }
}
int decode_parse(size_t buffer_len, parse *out) {
    INIT_BUFFER_UNPACK(buffer_len)
    int err;
    (void)err;
    NEXT_STR(out->smiles)
    FREE_BUFFER()
    return 0;
}
void free_result(result *s) {
    free_ASTElement(&s->result);
}
size_t result_size(const void *s){
	return ASTElement_size((void*)&((result*)s)->result);
}
int encode_result(const result *s) {
    size_t buffer_len = result_size(s);
    INIT_BUFFER_PACK(buffer_len)
    int err;
	(void)err;
        if ((err = encode_ASTElement(&s->result, __input_buffer + __buffer_offset, &buffer_len, &__buffer_offset))) {
            return err;
        }

    wasm_minimal_protocol_send_result_to_host(__input_buffer, buffer_len);
    return 0;
}
