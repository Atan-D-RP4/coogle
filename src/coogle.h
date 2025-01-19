#ifndef COOGLE_H_
#define COOGLE_H_

#include "clang-c/CXString.h"
#include "clang-c/Index.h"
// Resolved with -I /usr/lib/llvm-14/include/
// + /usr/lib/llvm-14/lib/libclang-14.so
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "include/nob.h"
#include "include/stb_c_lexer.h"

#define min(a, ...) ({ 													\
		typeof(a) args[] = {a, __VA_ARGS__}; 							\
		typeof(a) min = args[0]; 										\
		for (size_t i = 1; i < sizeof(args) / sizeof(args[0]); ++i) { 	\
			if (args[i] < min) { 										\
				min = args[i]; 											\
			} 															\
		} 																\
		min; 															\
	})

typedef struct {
	size_t count;
	size_t capacity;
	CXCursor *items;
} Children;

typedef struct CFunction {
	Nob_String_Builder name;
	Nob_String_Builder return_type;
	Nob_String_Builder arguments;
	unsigned int line;
	unsigned int column;
} CFunction;

typedef struct {
	size_t count;
	size_t capacity;
	CFunction *items;
} CFunctions;

typedef struct {
	CXIndex index;
	CXTranslationUnit tu;
	Children children;
	CFunctions functions;
} ParsedFile;

int match_expr(char *regexp, char *text);
int match_here(char *regexp, char *text);
int match_star(int c, char *regexp, char *text);

int compare_cursors_r(const void *a, const void *b, void *query);
int compare_functions_r(const void *a, const void *b, void *query);

void qsort_r(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *, void *), void *arg);
int levenstein_distance(const char *a, const char *b);

ParsedFile parse_file(const char *source_file);
enum CXChildVisitResult get_children(CXCursor cursor, CXCursor parent, CXClientData client_data);
Nob_String_Builder normalize_string(const char *query);
void printTU_Usage(CXTranslationUnit tu);
CFunction* getFuncFromCursor(CXCursor cursor);

void coogle(size_t limit, Children *cursors, const char *normalized_query, const char *source_file);

ParsedFile parse_file_new(const char* source_file, const char* query);
void coogle_new(CFunctions* funcs, const char* query, const char* src_file);

#endif // COOGLE_H_
