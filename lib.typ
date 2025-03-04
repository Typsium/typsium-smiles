#import "src/parser/protocol.typ": encode-parse, decode-ASTElement

#let parser = plugin("src/parser/parser.wasm")

#let parse(smile) = {
	decode-ASTElement(parser.parse_smiles(encode-parse((
		"smiles": smile
	))))
}

