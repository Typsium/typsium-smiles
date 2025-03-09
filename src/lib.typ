#import "parser/protocol.typ": encode-parse, decode-ASTElement

#let parser = plugin("parser/smiles.wasm")

#let parse(smile) = {
	decode-ASTElement(parser.parse_smiles(encode-parse((
		"smiles": smile
	))))
}

