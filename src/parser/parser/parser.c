#include "parser/parser.h"
#include <stdarg.h>
#include <stdio.h>

parser_ctx init_ctx(char *buffer, size_t len) {
    parser_ctx ctx = {.buffer = buffer,
                      .buffer_len = len,
                      .buffer_pos = 0,
                      .errored = false,
                      .error = NULL,
                      .no_error_message = 0};
    return ctx;
}

size_t save_pos(parser_ctx *ctx) {
    return ctx->buffer_pos;
}

void restore_pos(parser_ctx *ctx, size_t pos) {
    ctx->buffer_pos = pos;
}

bool is_eof(const parser_ctx *ctx) {
    return ctx->buffer_pos >= ctx->buffer_len;
}

char next(parser_ctx *ctx) {
    if (is_eof(ctx)) {
        ctx->errored = true;
        return '\0';
    }
    return ctx->buffer[ctx->buffer_pos++];
}

char peek(parser_ctx *ctx) {
    if (is_eof(ctx)) {
        ctx->errored = true;
        return '\0';
    }
    return ctx->buffer[ctx->buffer_pos];
}

ASTElement new_ASTElement(int type, size_t nb_children, int from) {
    return (ASTElement){.type = type,
                        .from = from,
                        .to = from + 1,
                        .value = NULL,
                        .children =
                            nb_children == 0 ? NULL : malloc(sizeof(ASTElement) * nb_children),
                        .children_len = 0};
}

void error(parser_ctx *ctx, char *fmt, ...) {
    ctx->errored = true;
    if (ctx->error != NULL) {
        free(ctx->error);
    }
    if (ctx->no_error_message > 0) {
        ctx->error = NULL;
        return;
    }
    va_list args;
    va_start(args, fmt);
    int size = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (size < 0) {
        ctx->error = NULL;
    }
    ctx->error = malloc(sizeof(char) * size + 1);
    va_start(args, fmt);
    if (vsprintf(ctx->error, fmt, args) < 0) {
        free(ctx->error);
        ctx->error = NULL;
    }
    va_end(args);
}

bool is_invalid(const ASTElement *element) {
    return element->type == -1;
}

#define INVALID_ELEMENT new_ASTElement(-1, 0, 0);

#define RETURN_ELEMENT(elem, ctx)                                                                  \
    elem.to = ctx->buffer_pos - 1;                                                                 \
    return elem;

#define EXPECT_CHAR(char_, ctx, elem)                                                              \
    if (peek(ctx) != char_) {                                                                      \
        free_ASTElement(&elem);                                                                    \
        error(ctx, "Expected %c, got %c", char_, peek(ctx));                                       \
        return INVALID_ELEMENT;                                                                    \
    }                                                                                              \
    next(ctx);

#define CHECK_CTX(ctx, elem)                                                                       \
    if (ctx->errored) {                                                                            \
        free_ASTElement(&elem);                                                                    \
        return INVALID_ELEMENT;                                                                    \
    }

bool start_by(parser_ctx *ctx, const char *str) {
    size_t i = 0;
    size_t pos = save_pos(ctx);
    while (!is_eof(ctx) && str[i] != '\0') {
        if (next(ctx) != str[i]) {
            restore_pos(ctx, pos);
            return false;
        }
        i++;
    }
    return true;
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

ASTElement stringElement(parser_ctx *ctx, ASTElementType type, const char strings[][5],
                         size_t len) {
    ASTElement elem = new_ASTElement(type, 0, ctx->buffer_pos);
    for (int i = 0; i < len; i++) {
        size_t pos = save_pos(ctx);
        if (start_by(ctx, strings[i])) {
            size_t len = strlen(strings[i]);
            elem.value = malloc(sizeof(char) * len + 1);
            elem.value = strcpy(elem.value, strings[i]);
            RETURN_ELEMENT(elem, ctx);
        }
        restore_pos(ctx, pos);
    }
    char *concat = malloc(sizeof(char) * len * 7 + 1);
    size_t clen = 0;
    for (int i = 0; i < len; i++) {
        strcpy(concat + clen, strings[i]);
        clen += strlen(strings[i]);
        if (i != len - 1) {
            concat[clen++] = ',';
            concat[clen++] = ' ';
        }
    }
    concat[clen] = '\0';

    error(ctx, "Expected one of %s, got %s", concat, ctx->buffer + ctx->buffer_pos);
    free(concat);
    return INVALID_ELEMENT;
}

ASTElement single_char(parser_ctx *ctx, ASTElementType type, char c) {
    if (peek(ctx) == c) {
        next(ctx);
        ASTElement elem = new_ASTElement(type, 0, ctx->buffer_pos);
        elem.value = malloc(sizeof(char) * 2);
        elem.value[0] = c;
        elem.value[1] = '\0';
        RETURN_ELEMENT(elem, ctx);
    }
    error(ctx, "Expected %c, got %c", c, peek(ctx));
    return INVALID_ELEMENT;
}

ASTElement number(parser_ctx *ctx) {
    ASTElement elem = new_ASTElement(NUMBER, 0, ctx->buffer_pos);
    char *value = malloc(sizeof(char) * 5);
    int cap = 4;
    size_t i = 0;
    while (!is_eof(ctx) && is_digit(peek(ctx))) {
        if (i == cap) {
            cap *= 2;
            value = realloc(value, cap);
        }
        value[i++] = next(ctx);
    }
    if (i == 0) {
        error(ctx, "Expected a number, got %c", peek(ctx));
        free(value);
        return INVALID_ELEMENT;
    }
    value[i] = '\0';
    elem.value = value;
    RETURN_ELEMENT(elem, ctx);
}

ASTElement digit(parser_ctx *ctx) {
    ASTElement elem = new_ASTElement(NUMBER, 0, ctx->buffer_pos);
    if (is_digit(peek(ctx))) {
        elem.value = malloc(sizeof(char) * 2);
        elem.value[0] = next(ctx);
        elem.value[1] = '\0';
        RETURN_ELEMENT(elem, ctx);
    }
    error(ctx, "Expected a digit, got %c", peek(ctx));
    return INVALID_ELEMENT;
}

ASTElement option(parser_ctx *ctx, ASTElementParser parser) {
    if (is_eof(ctx)) {
        return INVALID_ELEMENT;
    }
    size_t pos = save_pos(ctx);
    ctx->no_error_message++;
    ASTElement elem = parser(ctx);
    ctx->no_error_message--;
    if (ctx->errored) {
        restore_pos(ctx, pos);
        ctx->errored = false;
        free(ctx->error);
        free_ASTElement(&elem);
        return INVALID_ELEMENT;
    }
    RETURN_ELEMENT(elem, ctx);
}

#define STRING(name, type, ...)                                                                    \
    const char name##_strings[][5] = {__VA_ARGS__};                                                \
    ASTElement name(parser_ctx *ctx) {                                                             \
        return stringElement(ctx, type, name##_strings,                                            \
                             sizeof(name##_strings) / sizeof(name##_strings[0]));                  \
    }

#define SINGLE_CHAR(name, type, c)                                                                 \
    ASTElement name(parser_ctx *ctx) {                                                             \
        return single_char(ctx, type, c);                                                          \
    }

STRING(aliphatic_organic, ALIPHATIC_ORGANIC, "Br", "Cl", "N", "O", "P", "S", "F", "C", "B", "I")
STRING(aromatic_organic, AROMATIC_ORGANIC, "b", "c", "n", "o", "s", "p")
STRING(element_symbols, ELEMENT_SYMBOL, "He", "Li", "Be", "Bi", "Ne", "Na", "Mg", "Al", "Si", "Cl",
       "Ar", "Ca", "Sc", "Ti", "Cr", "Mn", "Fe", "Co", "Ni", "Cu", "Zn", "Ga", "Ge", "As", "Se",
       "Br", "Kr", "Rb", "Sr", "Zr", "Nb", "Mo", "Tc", "Ru", "Rh", "Pd", "Ag", "Cd", "In", "Sn",
       "Sb", "Te", "Xe", "Cs", "Ba", "Hf", "Ta", "Re", "Os", "Ir", "Pt", "Au", "Hg", "Tl", "Pb",
       "Po", "At", "Rn", "Fr", "Ra", "Rf", "Db", "Sg", "Bh", "Hs", "Mt", "Ds", "Rg", "Cn", "Fl",
       "Lv", "La", "Ce", "Pr", "Nd", "Pm", "Sm", "Eu", "Gd", "Tb", "Dy", "Ho", "Er", "Tm", "Yb",
       "Lu", "Ac", "Th", "Pa", "Np", "Pu", "Am", "Cm", "Bk", "Cf", "Es", "Fm", "Md", "No", "Lr",
       "H", "C", "N", "O", "F", "P", "S", "K", "V", "Y", "I", "W", "B", "U")
STRING(aromatic_symbols, AROMATIC_SYMBOL, "se", "as", "b", "c", "n", "o", "p", "s")
STRING(bond, BOND, "-", "=", "#", "$", ":", "/", "\\")
STRING(chiral_, CHIRAL, "@", "@@", "@TH1", "@TH2", "@AL1", "@AL2", "@SP1", "@SP2", "@SP3", "@TB",
       "@OH")
SINGLE_CHAR(star, CHAR, '*')
SINGLE_CHAR(minus, CHAR, '-')
SINGLE_CHAR(plus, CHAR, '+')
SINGLE_CHAR(dot, CHAR, '.')
SINGLE_CHAR(mod, CHAR, '%')
SINGLE_CHAR(colon, CHAR, ':')
SINGLE_CHAR(hydrogen, CHAR, 'H')
SINGLE_CHAR(open_paren, CHAR, '(')
SINGLE_CHAR(close_paren, CHAR, ')')

ASTElement symbol(parser_ctx *ctx) {
    ASTElement elem = option(ctx, aromatic_symbols);
    if (is_invalid(&elem)) {
        elem = option(ctx, star);
    }
    if (is_invalid(&elem)) {
        return element_symbols(ctx);
    }
    RETURN_ELEMENT(elem, ctx);
}

ASTElement chiral(parser_ctx *ctx) {
    ASTElement elem = chiral_(ctx);
    CHECK_CTX(ctx, elem);
    if (strcmp(elem.value, "@OH") == 0 || strcmp(elem.value, "@TB") == 0) {
        char d1 = next(ctx);
        char d2 = peek(ctx);
        if (d1 == 0 || !is_digit(d1)) {
            return digit(ctx);
        }
        elem.value[3] = d1;
        if (is_digit(d2)) {
            next(ctx);
            elem.value[4] = d2;
        }
        RETURN_ELEMENT(elem, ctx);
    }
    RETURN_ELEMENT(elem, ctx);
}

ASTElement hcount(parser_ctx *ctx) {
    ASTElement H = hydrogen(ctx);
    CHECK_CTX(ctx, H);
    ASTElement elem = {
        .type = HCOUNT,
        .children = malloc(sizeof(ASTElement) * 2),
        .children_len = 1,
    };
    elem.children[0] = H;
    if (is_digit(peek(ctx))) {
        elem.children[1] = digit(ctx);
        CHECK_CTX(ctx, elem);
        elem.children_len++;
    }
    RETURN_ELEMENT(elem, ctx);
}

ASTElement charge(parser_ctx *ctx) {
    ASTElement sign = option(ctx, minus);
    if (is_invalid(&sign)) {
        sign = option(ctx, plus);
    }
    if (is_invalid(&sign)) {
        error(ctx, "Expected a sign (- or +), got %c", peek(ctx));
        free_ASTElement(&sign);
        return INVALID_ELEMENT;
    }

    ASTElement elem = new_ASTElement(CHARGE, 3, ctx->buffer_pos);
    elem.children_len = 3;
    elem.children[0] = sign;
    elem.children[1] = option(ctx, digit);
    elem.children[2] = option(ctx, digit);
    RETURN_ELEMENT(elem, ctx);
}

ASTElement class(parser_ctx *ctx) {
    ASTElement elem = new_ASTElement(CLASS, 1, ctx->buffer_pos);
    EXPECT_CHAR(':', ctx, elem);
    elem.children[0] = number(ctx);
    elem.children_len++;
    CHECK_CTX(ctx, elem);
    RETURN_ELEMENT(elem, ctx);
}

ASTElement bracket_atom(parser_ctx *ctx) {
    ASTElement elem = new_ASTElement(BRACKET_ATOM, 6, ctx->buffer_pos);
    EXPECT_CHAR('[', ctx, elem);
    elem.children[0] = option(ctx, number);
    elem.children_len++;
    elem.children[1] = symbol(ctx);
    elem.children_len++;
    CHECK_CTX(ctx, elem);
    elem.children[2] = option(ctx, chiral);
    elem.children_len++;
    elem.children[3] = option(ctx, hcount);
    elem.children_len++;
    elem.children[4] = option(ctx, charge);
    elem.children_len++;
    elem.children[5] = option(ctx, class);
    elem.children_len++;
    EXPECT_CHAR(']', ctx, elem);
    RETURN_ELEMENT(elem, ctx);
}

ASTElement atom(parser_ctx *ctx) {
    ASTElement elem = option(ctx, aliphatic_organic);
    if (is_invalid(&elem)) {
        elem = option(ctx, aromatic_organic);
    }
    if (is_invalid(&elem)) {
        elem = option(ctx, star);
    }
    if (is_invalid(&elem)) {
        elem = option(ctx, bracket_atom);
    }
    if (is_invalid(&elem)) {
        error(ctx, "Expected an atom");
        return INVALID_ELEMENT;
    }
    RETURN_ELEMENT(elem, ctx);
}

ASTElement ringbond(parser_ctx *ctx) {
    ASTElement elem = new_ASTElement(RINGBOND, 4, ctx->buffer_pos);
    elem.children_len = 2;
    elem.children[0] = option(ctx, bond);
    if (peek(ctx) == '%') {
        elem.children[1] = mod(ctx);
        CHECK_CTX(ctx, elem);
        elem.children[2] = digit(ctx);
        elem.children_len++;
        CHECK_CTX(ctx, elem);
        elem.children[3] = digit(ctx);
        elem.children_len++;
        CHECK_CTX(ctx, elem);
    } else {
        elem.children[1] = digit(ctx);
        CHECK_CTX(ctx, elem);
    }
    RETURN_ELEMENT(elem, ctx);
}

ASTElement chain(parser_ctx *ctx);
ASTElement branch(parser_ctx *ctx);

ASTElement branched_atom(parser_ctx *ctx) {
    ASTElement element = new_ASTElement(BRANCHED_ATOM, 1, ctx->buffer_pos);
    element.children[0] = atom(ctx);
    element.children_len++;
    CHECK_CTX(ctx, element);
    int capacity = 1;
    bool check_ringbound = true;
    while (!is_invalid(&element.children[element.children_len - 1])) {
        if (element.children_len == capacity) {
            capacity *= 2;
            element.children = realloc(element.children, sizeof(ASTElement) * capacity);
        }
        if (check_ringbound) {
            element.children[element.children_len] = option(ctx, ringbond);
        } else {
            element.children[element.children_len] = option(ctx, branch);
        }
        element.children_len++;
        if (check_ringbound && is_invalid(&element.children[element.children_len - 1])) {
            check_ringbound = false;
            element.children_len--;
        }
    }
    element.children_len--;
    return element;
}

ASTElement branch(parser_ctx *ctx) {
    ASTElement elem = new_ASTElement(BRANCH, 2, ctx->buffer_pos);
    EXPECT_CHAR('(', ctx, elem);
    elem.children[0] = option(ctx, bond);
    elem.children_len++;
    if (is_invalid(&elem.children[0])) {
        elem.children[0] = option(ctx, dot);
    }
    if (is_invalid(&elem.children[0])) {
        elem.children[0] = chain(ctx);
    } else {
        elem.children[1] = chain(ctx);
    }
    CHECK_CTX(ctx, elem);
    EXPECT_CHAR(')', ctx, elem);
    RETURN_ELEMENT(elem, ctx);
}

ASTElement chain_(parser_ctx *ctx, ASTElement *chain, size_t cap) {
    if (chain->children_len + 2 >= cap) {
        cap *= 2;
        if (cap <= 2) {
            cap = 3;
        }
        chain->children = realloc(chain->children, sizeof(ASTElement) * cap);
    }

    chain->children[chain->children_len] = option(ctx, bond);
    if (is_invalid(&chain->children[chain->children_len])) {
        chain->children[chain->children_len] = option(ctx, dot);
    }
    if (is_invalid(&chain->children[chain->children_len])) {
        chain->children[chain->children_len] = option(ctx, branched_atom);
        if (is_invalid(&chain->children[chain->children_len])) {
            return *chain;
        }
        chain->children_len++;
        return chain_(ctx, chain, cap);
    }

    chain->children_len++;
    chain->children[chain->children_len] = branched_atom(ctx);
    chain->children_len++;
    CHECK_CTX(ctx, *chain);

    return chain_(ctx, chain, cap);
}

ASTElement chain(parser_ctx *ctx) {
    ASTElement elem = new_ASTElement(CHAIN, 1, ctx->buffer_pos);
    elem.children[0] = branched_atom(ctx);
    elem.children_len++;
    CHECK_CTX(ctx, elem);
    return chain_(ctx, &elem, 1);
}

bool is_terminator(parser_ctx *ctx) {
    return is_eof(ctx) || peek(ctx) == ' ' || peek(ctx) == '\r' || peek(ctx) == '\n' ||
           peek(ctx) == '\t' || peek(ctx) == '\0';
}

ASTElement terminator(parser_ctx *ctx) {
    if (is_terminator(ctx)) {
        return new_ASTElement(TERMINATOR, 0, ctx->buffer_pos);
    }
    error(ctx, "Expected end of expression");
    return INVALID_ELEMENT;
}

ASTElement smile(parser_ctx *ctx) {
    ASTElement elem = new_ASTElement(SMILES, 2, ctx->buffer_pos);
    elem.children[0] = chain(ctx);
    elem.children_len++;
    if (ctx->errored) {
        size_t pos = save_pos(ctx);
        ctx->buffer_pos = 0;
        if (!is_terminator(ctx)) {
            restore_pos(ctx, pos);
            return INVALID_ELEMENT;
        }
        free(ctx->error);
        ctx->errored = false;
        elem.children_len--;
    }
    elem.children[elem.children_len] = terminator(ctx);
    elem.children_len++;
    CHECK_CTX(ctx, elem);
    RETURN_ELEMENT(elem, ctx);
}
