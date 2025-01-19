#include "coogle.h"

int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <filename> '<query>'\n", argv[0]);
		return 1;
	}

	const char *source_file = argv[1];
	Children children = { 0, 0, NULL };

	Nob_String_Builder normalized_query_sb = normalize_string(argv[2]);
	const char *normalized_query = normalized_query_sb.items;

	nob_log(NOB_INFO, "\rNormalized Query: %s", normalized_query);

	ParsedFile parsed_file = parse_file(source_file);
	CXIndex index = parsed_file.index;
	CXTranslationUnit tu = parsed_file.tu;
	children = parsed_file.children;

	coogle(5, &children, normalized_query, source_file);

/*
	ParsedFile parsed_file = parse_file_new(source_file, normalized_query);
	CXIndex index = parsed_file.index;
	CXTranslationUnit tu = parsed_file.tu;
	Functions functions = parsed_file.functions;

	coogle_new(&functions, normalized_query, source_file);
*/

	printTU_Usage(tu);
	nob_da_free(children);
	nob_sb_free(normalized_query_sb);
	clang_disposeTranslationUnit(tu);
	clang_disposeIndex(index);

	nob_log(NOB_INFO, "Exit.");

	return 0;
}
