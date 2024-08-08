#include "clang-c/CXString.h"
#include <clang-c/Index.h>
// Resolved with -I /usr/lib/llvm-14/include/
// + /usr/lib/llvm-14/lib/libclang-14.so
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define NOB_IMPLEMENTATION
#include "include/nob.h"

#define STB_C_LEXER_IMPLEMENTATION
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

typedef struct Function {
	Nob_String_Builder name;
	Nob_String_Builder return_type;
	Nob_String_Builder arguments;
	unsigned int line;
	unsigned int column;
} Function;

typedef struct {
	CXIndex index;
	CXTranslationUnit tu;
	Children children;
} ParsedFile;

int functions = 0;

int match_expr(char *regexp, char *text);
int match_here(char *regexp, char *text);
int match_star(int c, char *regexp, char *text);

void qsort_r(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *, void *), void *arg);
int compare_cursors_r(const void *a, const void *b, void *query);
int levenstein_distance(const char *a, const char *b);

enum CXChildVisitResult get_children(CXCursor cursor, CXCursor parent, CXClientData client_data);
Nob_String_Builder normalize_string(const char *query);
ParsedFile parse_file(const char *source_file);
void printTU_Usage(CXTranslationUnit tu);
Function* getFuncFromCursor(CXCursor cursor);

int compare_cursors_r(const void *a, const void *b, void *query) {
	CXCursor cursor_a = *(CXCursor *) a;
	CXCursor cursor_b = *(CXCursor *) b;

	CXString sig_a = clang_getTypeSpelling(clang_getCursorType(cursor_a));
	CXString sig_b = clang_getTypeSpelling(clang_getCursorType(cursor_b));

	size_t rank = levenstein_distance((const char *) query, clang_getCString(sig_a)) -
		levenstein_distance((const char *) query, clang_getCString(sig_b));

	clang_disposeString(sig_a);
	clang_disposeString(sig_b);

	return rank;
}


enum CXChildVisitResult get_children(CXCursor cursor, CXCursor parent, CXClientData client_data) {

	if (clang_Location_isFromMainFile(clang_getCursorLocation(cursor)) == 0) {
		return CXChildVisit_Continue;
	}

	if (cursor.kind == CXCursor_FunctionDecl) {
		functions++;
		Children *children = (Children *) client_data;
		nob_da_append(children, cursor);
	}

	return CXChildVisit_Recurse;
}

Nob_String_Builder normalize_string(const char *query) {
	Nob_String_Builder buf = { NULL, 0, 0 };
	buf.capacity = NOB_DA_INIT_CAP;
	buf.items = (char *)malloc(buf.capacity);

	stb_lexer lex;
	stb_c_lexer_init(&lex, query, (query + strlen(query)), buf.items, buf.capacity);

	Nob_String_Builder sb = { NULL, 0, 0 };
	while (stb_c_lexer_get_token(&lex)) {
		if (lex.token < CLEX_eof) {
			char *token = (char *)malloc(2);
			token[0] = lex.token;
			token[1] = '\0';
			nob_sb_append_cstr(&sb, token);
			nob_sb_append_cstr(&sb, " ");
			sb.items[sb.count] = '\0';
			free(token);
		} else if (lex.token == CLEX_id) {
			nob_sb_append_cstr(&sb, lex.string);
			nob_sb_append_cstr(&sb, " ");
			sb.items[sb.count] = '\0';
		} else {
			nob_log(NOB_ERROR, "\rUnsupported Token: %d", lex.token);
		}
	}

	nob_sb_free(buf);
	return sb;
}

int levenstein_distance(const char *a, const char *b) {
	int len1 = strlen(a);
	int len2 = strlen(b);

	// Create a matrix to store the distances
	int matrix[len1 + 1][len2 + 1];

	// Initialize the matrix
	for (int i = 0; i <= len1; ++i)
		matrix[i][0] = i;

	for (int i = 0; i <= len2; ++i)
		matrix[0][i] = i;

	// Fill in the matrix
	for (int i = 1; i <= len1; i++) {
		for (int j = 1; j <= len2; j++) {
			if (a[i - 1] == b[j - 1])
				matrix[i][j] = matrix[i - 1][j - 1];
			else
				matrix[i][j] = 1 + min(matrix[i - 1][j], matrix[i][j - 1], matrix[i - 1][j - 1]);
		}
	}

	// The bottom-right cell of the matrix contains the Levenshtein distance
	return matrix[len1][len2];
}

ParsedFile parse_file(const char *source_file) {

	Children children = { 0, 0, NULL };
	CXIndex index = clang_createIndex(0, 0);
	CXTranslationUnit tu;
	struct CXUnsavedFile temp_files[] = { NULL };
	enum CXTranslationUnit_Flags flags = CXTranslationUnit_DetailedPreprocessingRecord;

	enum CXErrorCode err = clang_parseTranslationUnit2(index, source_file, NULL, 0,
			temp_files, temp_files->Length, flags, &tu);

	if (err != CXError_Success) {
		nob_log(NOB_ERROR, "Unable to parse translation unit. Quitting.");
		exit(1);
	}

	nob_log(NOB_INFO, "Parsed file: %s", source_file);

	unsigned num_diagnostics = clang_getNumDiagnostics(tu);
	if (num_diagnostics > 0) {
		nob_log(NOB_WARNING, "There were %d diagnostics:", num_diagnostics);
		for (unsigned i = 0; i < num_diagnostics; i++) {
			CXDiagnostic diag = clang_getDiagnostic(tu, i);
			CXString string = clang_formatDiagnostic(diag,
					clang_defaultDiagnosticDisplayOptions());
			nob_log(NOB_WARNING, "%s", clang_getCString(string));
			clang_disposeString(string);
		}
	} else {
		nob_log(NOB_INFO, "No diagnostics.");
	}

	CXCursor cursor = clang_getTranslationUnitCursor(tu);
	clang_visitChildren(cursor, get_children, &children);

	return (ParsedFile) { index, tu, children };
}
void printTU_Usage(CXTranslationUnit tu) {

	CXTUResourceUsage usage = clang_getCXTUResourceUsage(tu);

	nob_log(NOB_INFO, "Memory Usage: %zu", usage.numEntries);
	for (unsigned i = 0; i < usage.numEntries; i++) {
		CXTUResourceUsageEntry entry = usage.entries[i];
		const char* name = clang_getTUResourceUsageName(entry.kind);
		nob_log(NOB_INFO, "%s: %zu", name, entry.amount);
	}

	clang_disposeCXTUResourceUsage(usage);
}

Function* getFuncFromCursor(CXCursor cursor) {
	Function *func = (Function *) calloc(1, sizeof(Function));
	memset(func, 0, sizeof(Function));

	CXString name = clang_getCursorSpelling(cursor);
	CXString signature = clang_getTypeSpelling(clang_getCursorType(cursor));
	CXSourceLocation loc = clang_getCursorLocation(cursor);

	clang_getSpellingLocation(loc, NULL, &func->line, &func->column, NULL);

	nob_sb_append_cstr(&func->name, clang_getCString(name));
	nob_sb_append_cstr(&func->return_type, clang_getCString(signature));

	int argc = clang_Cursor_getNumArguments(cursor);
	for (size_t i = 0; i < (size_t) argc; i++) {
		CXCursor arg = clang_Cursor_getArgument(cursor, i);
		CXString arg_name = clang_getCursorSpelling(arg);
		CXString arg_type = clang_getTypeSpelling(clang_getCursorType(arg));

		nob_sb_append_cstr(&func->arguments, clang_getCString(arg_type));
		nob_sb_append_cstr(&func->arguments, " ");
		nob_sb_append_cstr(&func->arguments, clang_getCString(arg_name));
		nob_sb_append_cstr(&func->arguments, ", ");

		clang_disposeString(arg_name);
		clang_disposeString(arg_type);
	}

	clang_disposeString(name);
	clang_disposeString(signature);

	return func;
}

void coogle(size_t limit, Children *cursors, const char *normalized_query, const char *source_file) {
	Children children = *cursors;

	qsort_r(children.items, children.count, sizeof(CXCursor), compare_cursors_r, (void *) normalized_query);
	for (size_t count = 0; count < children.count && count < limit; count++) {

		Function *func = getFuncFromCursor(children.items[count]);

		nob_log(NOB_INFO, "%2zu - %s:%4d:%2d - %s :: %s(%s)",
				count + 1, source_file, func->line, func->column,
				func->return_type.items, func->name.items, func->arguments.items);
		free(func);
	}
}

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

	coogle(60, &children, normalized_query, source_file);

	printTU_Usage(tu);
	nob_da_free(children);
	nob_sb_free(normalized_query_sb);
	clang_disposeTranslationUnit(tu);
	clang_disposeIndex(index);

	nob_log(NOB_INFO, "Exit.");

	return 0;
}

