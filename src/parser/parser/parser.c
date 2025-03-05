#include "parser/parser.h"

parser_ctx init_ctx(char *buffer, size_t len) {
    parser_ctx ctx = {
        .buffer = buffer, .buffer_len = len, .buffer_pos = 0, .errored = false, .error = NULL};
    return ctx;
}

size_t save_pos(parser_ctx *ctx) {
    return ctx->buffer_pos;
}

void restore_pos(parser_ctx *ctx, size_t pos) {
    ctx->buffer_pos = pos;
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

ASTElement new_ASTElement(int type, size_t nb_children) {
	return (ASTElement) {
		.type = type,
		.value = NULL,
		.children = nb_children == 0 ? NULL : malloc(sizeof(ASTElement) * nb_children),
		.children_len = 0
	};
}

bool is_invalid(const ASTElement *element) {
	return element->type == -1;
}

#define INVALID_ELEMENT new_ASTElement(-1, 0);

#define EXPECT_CHAR(char_, ctx, elem)                                                               \
    if (peek(ctx) != char_) {                                                                       \
        free_ASTElement(&elem);                                                                    \
        ctx->errored = true;                                                                       \
        ctx->error = malloc(sizeof(char) * 11);                                                    \
        ctx->error = strcpy(ctx->error, "Expected ");                                              \
        ctx->error[9] = char_;                                                                      \
        ctx->error[10] = '\0';                                                                     \
        return INVALID_ELEMENT;                                                                    \
    } \
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

ASTElement stringElement(parser_ctx *ctx, ASTElementType type, const char strings[][5], size_t len) {
	ASTElement elem = {
		.type = type,
    };
    for (int i = 0; i < len; i++) {
		size_t pos = save_pos(ctx);
        if (start_by(ctx, strings[i])) {
            elem.value = malloc(sizeof(char) * strlen(strings[i]) + 1);
            elem.value = strcpy(elem.value, strings[i]);
            return elem;
        }
		restore_pos(ctx, pos);
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
        ASTElement elem = new_ASTElement(type, 0);
        elem.value = malloc(sizeof(char) * 2);
        elem.value[0] = c;
        elem.value[1] = '\0';
        return elem;
    }
    ctx->errored = true;
    ctx->error = malloc(sizeof(char) * 28);
    ctx->error = strcpy(ctx->error, "Expected ");
	char expected[] = {c, '\0'};
    ctx->error = strcat(ctx->error, expected);
    ctx->error = strcat(ctx->error, ", got ");
	expected[0] = peek(ctx);
    ctx->error = strcat(ctx->error, expected);
    return INVALID_ELEMENT;
}

ASTElement number(parser_ctx *ctx) {
    ASTElement elem = new_ASTElement(NUMBER, 0);
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
    ASTElement elem = new_ASTElement(NUMBER, 0);
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
	if (is_eof(ctx)) {
		return INVALID_ELEMENT;
	}
	size_t pos = save_pos(ctx);
    ASTElement elem = parser(ctx);
    if (ctx->errored) {
		restore_pos(ctx, pos);
        ctx->errored = false;
        free(ctx->error);
        return INVALID_ELEMENT;
    }
    return elem;
}

#define STRING(name, type, ...)                                                                    \
	const char name##_strings[][5] = {__VA_ARGS__};                                                \
	ASTElement name(parser_ctx *ctx) {                                                             \
        return stringElement(ctx, type, name##_strings, sizeof(name##_strings) / sizeof(name##_strings[0]));  \
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
    return elem;
}

ASTElement charge(parser_ctx *ctx) {
    ASTElement sign = option(ctx, minus);
    if (is_invalid(&sign)) {
        sign = option(ctx, plus);
    }
    if (is_invalid(&sign)) {
        ctx->errored = true;
        ctx->error = malloc(sizeof(char) * 25);
        strcpy(ctx->error, "Expected a sign, got ");
        ctx->error[23] = peek(ctx);
        ctx->error[24] = '\0';
        return INVALID_ELEMENT;
    }

    ASTElement elem = new_ASTElement(CHARGE, 3);
	elem.children_len = 3;
    elem.children[0] = sign;
    elem.children[1] = option(ctx, digit);
    elem.children[2] = option(ctx, digit);
    return elem;
}

ASTElement class(parser_ctx *ctx) {
    ASTElement elem = new_ASTElement(CLASS, 1);
    EXPECT_CHAR(':', ctx, elem);
    elem.children[0] = number(ctx);
	elem.children_len++;
    CHECK_CTX(ctx, elem);
    return elem;
}

ASTElement bracket_atom(parser_ctx *ctx) {
    ASTElement elem = new_ASTElement(BRACKET_ATOM, 6);
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
    if (is_invalid(&elem)) {
        elem = option(ctx, aromatic_organic);
    }
    if (is_invalid(&elem)) {
        elem = option(ctx, star);
    }
    if (is_invalid(&elem)) {
        return bracket_atom(ctx);
    }
    return elem;
}

ASTElement ringbond(parser_ctx *ctx) {
	ASTElement elem = new_ASTElement(RINGBOND, 4);
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
	return elem;
}

ASTElement chain(parser_ctx *ctx);
ASTElement branch(parser_ctx *ctx);

ASTElement branched_atom(parser_ctx *ctx) {
	ASTElement element = new_ASTElement(BRANCHED_ATOM,  1);
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
	ASTElement elem = new_ASTElement(BRANCH, 2);
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
	return elem;
}

ASTElement chain_(parser_ctx *ctx, ASTElement *chain, size_t cap) {
	if (chain->children_len + 2 >= cap) {
		cap *= 2;
		chain->children = realloc(chain->children, sizeof(ASTElement) * cap);
	}

	chain->children_len++;
	chain->children[chain->children_len - 1] = option(ctx, bond);
	if (is_invalid(&chain->children[chain->children_len - 1])) {
		chain->children[chain->children_len - 1] = option(ctx, dot);
	}
	if (is_invalid(&chain->children[chain->children_len - 1])) {
		chain->children[chain->children_len - 1] = option(ctx, branched_atom);
		if (is_invalid(&chain->children[chain->children_len - 1])) {
			chain->children_len--;
			return *chain;
		}
	} else {
		chain->children_len++;
		chain->children[chain->children_len - 1] = branched_atom(ctx);
		CHECK_CTX(ctx, *chain);
	}

	return chain_(ctx, chain, cap);
}

ASTElement chain(parser_ctx *ctx) {
	ASTElement elem = new_ASTElement(CHAIN, 1);
	elem.children[0] = branched_atom(ctx);
	elem.children_len++;
	CHECK_CTX(ctx, elem);
	return chain_(ctx, &elem, 1);
}

bool is_terminator(parser_ctx *ctx){return is_eof(ctx) || peek(ctx) == ' ' || peek(ctx) == '\r' ||
                                          peek(ctx) == '\n' || peek(ctx) == '\t' ||
                                          peek(ctx) == '\0';
										}

ASTElement terminator(parser_ctx *ctx) {
    if (is_terminator(ctx)) {
		next(ctx);
		return new_ASTElement(TERMINATOR, 0);
	}
	ctx->errored = true;
	ctx->error = malloc(sizeof(char) * 28);
	strcpy(ctx->error, "Expected end of expression");
	return INVALID_ELEMENT;
}

ASTElement smile(parser_ctx *ctx) {
	ASTElement elem = new_ASTElement(SMILES, 2);
	elem.children[0] = chain(ctx);
	elem.children_len++;
	if (ctx->errored) {
		ctx->buffer_pos = 0;
		if (!is_terminator(ctx)) {
			return INVALID_ELEMENT;
		}
		ctx->errored = false;
		free(ctx->error);
		elem.children_len--;
	}
	elem.children[elem.children_len] = terminator(ctx);
	elem.children_len++;
	CHECK_CTX(ctx, elem);
	return elem;
}
