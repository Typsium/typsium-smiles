#include "parser/parser.h"

parser_ctx init_ctx(char *buffer, size_t len) {
    parser_ctx ctx = {
        .buffer = buffer, .buffer_len = len, .buffer_pos = 0, .errored = false, .error = NULL};
    return ctx;
}

void save_pos(parser_ctx *ctx) {
    ctx->saved_pos = ctx->buffer_pos;
}

void restore_pos(parser_ctx *ctx) {
    ctx->buffer_pos = ctx->saved_pos;
}

bool is_eof(const parser_ctx *ctx) {
    return ctx->buffer_pos >= ctx->buffer_len || ctx->errored;
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

#define INVALID_ELEMENT (ASTElement){.type = -1}

#define EXPECT_CHAR(char_, ctx, elem)                                                               \
    if (next(ctx) != char_) {                                                                       \
        free_ASTElement(&elem);                                                                    \
        ctx->errored = true;                                                                       \
        ctx->error = malloc(sizeof(char) * 11);                                                    \
        ctx->error = strcpy(ctx->error, "Expected ");                                              \
        ctx->error[9] = char_;                                                                      \
        ctx->error[10] = '\0';                                                                     \
        return INVALID_ELEMENT;                                                                    \
    }

#define CHECK_CTX(ctx, elem)                                                                       \
    if (ctx->errored) {                                                                            \
        free_ASTElement(&elem);                                                                    \
        return INVALID_ELEMENT;                                                                    \
    }

bool start_by(parser_ctx *ctx, const char *str) {
	size_t i = 0;
    while (!is_eof(ctx) && str[i] != '\0') {
        if (next(ctx) != str[i]) {
            restore_pos(ctx);
            return false;
        }
        i++;
    }
    return true;
}

bool is_digit(char c) {
	return c >= '0' && c <= '9';
}

ASTElement stringElement(parser_ctx *ctx, ASTElementType type, char strings[][6], size_t len) {
	ASTElement elem = {
		.type = type,
    };
    for (int i = 0; i < len; i++) {
		save_pos(ctx);
        if (start_by(ctx, strings[i])) {
            elem.value = malloc(sizeof(char) * strlen(strings[i]));
            elem.value = strcpy(elem.value, strings[i]);
            return elem;
        }
		restore_pos(ctx);
    }
    ctx->errored = true;
    ctx->error = malloc(sizeof(char) * (len * 10 + 25 + ctx->buffer_len));
    ctx->error = strcpy(ctx->error, "Expected one of ");
    for (int i = 0; i < len; i++) {
        ctx->error = strcat(ctx->error, strings[i]);
        ctx->error = strcat(ctx->error, ", ");
    }
    ctx->error = strcat(ctx->error, "got ");
    ctx->error = strcat(ctx->error, ctx->buffer + ctx->buffer_pos);
    return INVALID_ELEMENT;
}

ASTElement single_char(parser_ctx *ctx, ASTElementType type, char c) {
    if (peek(ctx) == c) {
		next(ctx);
        ASTElement elem = {
            .type = type,
        };
        elem.value = malloc(sizeof(char) * 2);
        elem.value[0] = c;
        elem.value[1] = '\0';
        return elem;
    }
    ctx->errored = true;
    ctx->error = malloc(sizeof(char) * 25);
    ctx->error = strcpy(ctx->error, "Expected ");
    ctx->error = strcat(ctx->error, &c);
    ctx->error = strcat(ctx->error, ", got ");
    ctx->error = strcat(ctx->error, ctx->buffer + ctx->buffer_pos);
    return INVALID_ELEMENT;
}

ASTElement number(parser_ctx *ctx) {
    ASTElement elem = {
        .type = NUMBER,
    };
    char *value = malloc(sizeof(char) * 10);
    size_t i = 0;
    while (!is_eof(ctx) && is_digit(peek(ctx))) {
        value[i++] = next(ctx);
    }
    if (i == 0) {
        ctx->errored = true;
        ctx->error = malloc(sizeof(char) * 25);
        strcpy(ctx->error, "Expected a number, got ");
        ctx->error[23] = peek(ctx);
        ctx->error[24] = '\0';
        elem.value = NULL;
        return INVALID_ELEMENT;
    }
    value[i] = '\0';
    elem.value = value;
    return elem;
}

ASTElement digit(parser_ctx *ctx) {
    ASTElement elem = {
        .type = NUMBER,
    };
    if (is_digit(peek(ctx))) {
        elem.value = malloc(sizeof(char) * 2);
        elem.value[0] = next(ctx);
        elem.value[1] = '\0';
        return elem;
    }
    ctx->errored = true;
    ctx->error = malloc(sizeof(char) * 25);
    strcpy(ctx->error, "Expected a digit, got ");
    ctx->error[23] = peek(ctx);
    ctx->error[24] = '\0';
    elem.value = NULL;
    return INVALID_ELEMENT;
}

ASTElement option(parser_ctx *ctx, ASTElementParser parser) {
    ASTElement elem = parser(ctx);
    if (ctx->errored) {
        ctx->errored = false;
        free(ctx->error);
        return INVALID_ELEMENT;
    }
    return elem;
}

#define STRING(name, type, ...)                                                                    \
    char name##_strings[][6] = {__VA_ARGS__};                                                      \
    ASTElement name(parser_ctx *ctx) {                                                             \
        return stringElement(ctx, type, name##_strings, sizeof(name##_strings) / sizeof(char *));  \
    }

#define SINGLE_CHAR(name, type, c)                                                                 \
    ASTElement name(parser_ctx *ctx) {                                                             \
        return single_char(ctx, type, c);                                                          \
    }

STRING(aliphatic_organic, ALIPHATIC_ORGANIC, "B", "C", "N", "O", "P", "S", "F", "Cl", "Br", "I")
STRING(aromatic_organic, AROMATIC_ORGANIC, "b", "c", "n", "o", "s", "p")
STRING(element_symbols, ELEMENT_SYMBOL, "H", "He", "Li", "Be", "B", "C", "N", "O", "F", "Ne", "Na",
       "Mg", "Al", "Si", "P", "S", "Cl", "Ar", "K", "Ca", "Sc", "Ti", "V", "Cr", "Mn", "Fe", "Co",
       "Ni", "Cu", "Zn", "Ga", "Ge", "As", "Se", "Br", "Kr", "Rb", "Sr", "Y", "Zr", "Nb", "Mo",
       "Tc", "Ru", "Rh", "Pd", "Ag", "Cd", "In", "Sn", "Sb", "Te", "I", "Xe", "Cs", "Ba", "Hf",
       "Ta", "W", "Re", "Os", "Ir", "Pt", "Au", "Hg", "Tl", "Pb", "Bi", "Po", "At", "Rn", "Fr",
       "Ra", "Rf", "Db", "Sg", "Bh", "Hs", "Mt", "Ds", "Rg", "Cn", "Fl", "Lv", "La", "Ce", "Pr",
       "Nd", "Pm", "Sm", "Eu", "Gd", "Tb", "Dy", "Ho", "Er", "Tm", "Yb", "Lu", "Ac", "Th", "Pa",
       "U", "Np", "Pu", "Am", "Cm", "Bk", "Cf", "Es", "Fm", "Md", "No", "Lr")
STRING(aromatic_symbols, AROMATIC_SYMBOL, "b", "c", "n", "o", "p", "s", "se", "as")
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
    if (elem.type == -1) {
        elem = option(ctx, star);
    }
    if (elem.type == -1) {
        return element_symbols(ctx);
    }
    return elem;
}

ASTElement chiral(parser_ctx *ctx) {
    ASTElement elem = chiral_(ctx);
	CHECK_CTX(ctx, elem);
    if (strcmp(elem.value, "@OH") == 0 || strcmp(elem.value, "@TB") == 0) {
        char d1 = next(ctx);
        char d2 = peek(ctx);
        if (d1 == 0 || !is_digit(d1)) {
            ctx->errored = true;
            ctx->error = malloc(sizeof(char) * 25);
            strcpy(ctx->error, "Expected a digit, got ");
            ctx->error[23] = d1;
            ctx->error[24] = '\0';
        }
        elem.value[3] = d1;
        if (is_digit(d2)) {
            next(ctx);
            elem.value[4] = d2;
        }
        return elem;
    }
    return elem;
}

ASTElement hcount(parser_ctx *ctx) {
    ASTElement elem = hydrogen(ctx);
    CHECK_CTX(ctx, elem);
    if (is_digit(peek(ctx))) {
        elem.value = realloc(elem.value, sizeof(char) * 3);
        elem.value[1] = next(ctx);
        elem.value[2] = '\0';
    }
    return elem;
}

ASTElement charge(parser_ctx *ctx) {
    ASTElement sign = option(ctx, minus);
    if (sign.type == -1) {
        sign = option(ctx, plus);
    }
    if (sign.type == -1) {
        ctx->errored = true;
        ctx->error = malloc(sizeof(char) * 25);
        strcpy(ctx->error, "Expected a sign, got ");
        ctx->error[23] = peek(ctx);
        ctx->error[24] = '\0';
        return INVALID_ELEMENT;
    }
    ASTElement d1 = digit(ctx);
    CHECK_CTX(ctx, d1);
    ASTElement d2 = option(ctx, digit);
    if (d2.type == -1) {
        d2 = INVALID_ELEMENT;
    }
    ASTElement elem = {
        .type = CHARGE, .children = malloc(sizeof(ASTElement) * 3), .children_len = 3};
    elem.children[0] = sign;
    elem.children[1] = d1;
    elem.children[2] = d2;
    return elem;
}

ASTElement class(parser_ctx *ctx) {
    ASTElement col = colon(ctx);
    CHECK_CTX(ctx, col);
    ASTElement num = number(ctx);
    CHECK_CTX(ctx, col);
    ASTElement elem = {
        .type = CLASS, .children = malloc(sizeof(ASTElement) * 2), .children_len = 2};
    elem.children[0] = col;
    elem.children[1] = num;
    return elem;
}

ASTElement bracket_atom(parser_ctx *ctx) {
    ASTElement elem = {.type = BRACKET_ATOM, .children = malloc(sizeof(ASTElement) * 6)};
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
    return elem;
}

ASTElement atom(parser_ctx *ctx) {
    ASTElement elem = option(ctx, aliphatic_organic);
    if (elem.type == -1) {
        elem = option(ctx, aromatic_organic);
    }
    if (elem.type == -1) {
        elem = option(ctx, star);
    }
    if (elem.type == -1) {
        return bracket_atom(ctx);
    }
    return elem;
}