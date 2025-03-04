#include <stdio.h>
#include "test/wasm.h"
#include "parser/parser.h"

void print_astType(ASTElementType t) {
	switch (t) {
		case ALIPHATIC_ORGANIC:
			printf("ALIPHATIC_ORGANIC");
			break;
		case AROMATIC_ORGANIC:
			printf("AROMATIC_ORGANIC");
			break;
		case ELEMENT_SYMBOL:
			printf("ELEMENT_SYMBOL");
			break;
		case AROMATIC_SYMBOL:
			printf("AROMATIC_SYMBOL");
			break;
		case BOND:
			printf("BOND");
			break;
		case NUMBER:
			printf("NUMBER");
			break;
		case CHAR:
			printf("CHAR");
			break;
		case BRACKET_ATOM:
			printf("BRACKET_ATOM");
			break;
		case CHARGE:
			printf("CHARGE");
			break;
		case CHIRAL:
			printf("CHIRAL");
			break;
		case CLASS:
			printf("CLASS");
			break;
		case HCOUNT:
			printf("HCOUNT");
			break;
		case RINGBOND:
			printf("RINGBOND");
			break;
		case BRANCHED_ATOM:
			printf("BRANCHED_ATOM");
			break;
		case BRANCH:
			printf("BRANCH");
			break;
		case CHAIN:
			printf("CHAIN");
			break;
		case TERMINATOR:
			printf("TERMINATOR");
			break;
		case SMILES:
			printf("SMILES");
			break;
		default:
			printf("INVALID");
			break;
	}
}

void print_ast(const ASTElement *e, char *indent) {
	printf("%sASTElement {\n", indent);
	printf("%s    type: ", indent);
	print_astType(e->type);
	printf(",\n");
	if (e->value != NULL) {
		printf("%s    value: %s,\n", indent, e->value);
	}
	if (e->children_len > 0) {
		printf("%s    children: [\n", indent);
		for (size_t i = 0; i < e->children_len; i++) {
			char new_indent[100];
			sprintf(new_indent, "%s        ", indent);
			print_ast(&e->children[i], new_indent);
			if (i < e->children_len - 1) {
				printf(",\n");
			} else {
				printf("\n");
			}
		}
		printf("%s    ]\n", indent);
	}
	printf("%s}", indent);
}

int main() {
    char test_string[] = "Oc1c(*)cccc1";
    parser_ctx ctx = init_ctx(test_string, sizeof(test_string));
	ASTElement ast = smile(&ctx);
	if (ctx.errored) {
		printf("Error: %s\n", ctx.error);
		printf("%s\n", test_string);
		for (size_t i = 0; i < ctx.buffer_pos; i++) {
			printf(" ");
		}
		printf("^\n");
		return 1;
	}
	print_ast(&ast, "");
	printf("\n");
	free_ASTElement(&ast);
	return 0;
}
