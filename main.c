#include <clang-c/Index.h> 
// Resolved with -I /usr/lib/llvm-14/include/
// + /usr/lib/llvm-14/lib/libclang-14.so 
#include <stdio.h>

int main(int argc, char *argv[]) {
	printf("Hello, World!\n");
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
		return 1;
	}

	const char *source_file = argv[1];
	CXIndex index = clang_createIndex(0, 0);

	CXTranslationUnit tu = clang_parseTranslationUnit(index, source_file, NULL, 0, NULL, 0, CXTranslationUnit_None);

	if (!tu) {
		fprintf(stderr, "Unable to parse translation unit. Quitting.\n");
		return 1;
	}

	clang_disposeTranslationUnit(tu);
	clang_disposeIndex(index);

	return 0;
}
