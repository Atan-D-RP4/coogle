#include "include/nob.h"
#include <stdio.h>
#define NOB_IMPLEMENTATION
#define STB_C_LEXER_IMPLEMENTATION

#include "coogle.h"

int functions = 0;

CFunctions funcs_global = { 0, 0, NULL };

int compare_functions_r(const void *a, const void *b, void *query) {
	CFunction function_a = *(CFunction *) a;
	CFunction function_b = *(CFunction *) b;

	char *query_str = (char *) query;
	size_t rank = levenstein_distance(query_str, function_a.name.items) -
		levenstein_distance(query_str, function_b.name.items);

	return rank;
}

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

/**
 * Normalizes a given query string by tokenizing it and returning a string builder containing the normalized tokens.
 *
 * This function uses the `stb_c_lexer` library to tokenize the input query string. It iterates through the tokens and appends them to a `Nob_String_Builder` object, handling different token types appropriately (e.g. identifiers, unsupported tokens).
 *
 * @param query The input query string to be normalized.
 * @return A `Nob_String_Builder` containing the normalized tokens.
 */
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
			sb.items[sb.count] = '\0';
			free(token);
		} else if (lex.token == CLEX_id) {
			nob_sb_append_cstr(&sb, lex.string);
			sb.items[sb.count] = '\0';
		} else {
			nob_log(NOB_ERROR, "\rUnsupported Token: %d", lex.token);
		}
	}

	nob_sb_free(buf);
	return sb;
}

/**
 * Callback function for the Clang AST traversal that collects function declarations.
 *
 * This function is called for each node in the Clang AST. It checks if the current node is a function declaration, and if so, it extracts information about the function (name, return type, arguments) and adds it to the `CFunctions` data structure passed in the `client_data` parameter.
 *
 * @param cursor The current Clang AST node being visited.
 * @param parent The parent Clang AST node of the current node.
 * @param client_data A pointer to a `struct ClientData_t` containing the necessary data for the function matching logic.
 * @return `CXChildVisit_Continue` to continue the AST traversal, or `CXChildVisit_Break` to stop the traversal.
 */
enum CXChildVisitResult get_funcs(CXCursor cursor, CXCursor parent, CXClientData client_data) {
	if (clang_Location_isFromMainFile(clang_getCursorLocation(cursor)) == 0) {
		return CXChildVisit_Continue;
	}

	if (cursor.kind == CXCursor_FunctionDecl) {
		struct ClientData_t {
				char *regexp_ret;
				char **regexp_args;
				int argc;
				CFunctions funcs;
			} ClientData_t;

		ClientData_t = *(struct ClientData_t *) client_data;
		char *regexp_ret = ClientData_t.regexp_ret;
		char **regexp_args = ClientData_t.regexp_args;
		int argc = ClientData_t.argc;
		CFunctions funcs = ClientData_t.funcs;

//	nob_log(NOB_INFO, "Ret: %s", regexp_ret);
//	for (int j = 0; j < argc; j++) {
//		nob_log(NOB_INFO, "Arg: %s", regexp_args[j]);
//	}
		CXString name = clang_getCursorSpelling(cursor);
		CFunction *func = getFuncFromCursor(cursor);
		printf("CFunction: %s :: ", func->name.items);
		printf("%s", func->arguments.items);
		printf(" :: %s\n", func->return_type.items);

		char* func_args[10];
		char *token = strtok(NULL, ",()");
		int i = 0;

		printf("Ret: %s ", regexp_ret);
		while (token != NULL) {
			func_args[i++] = token;
			token = strtok(NULL, ",()");
			printf("%s ", func_args[i - 1]);
		}
		printf("\n");


		bool matched = match_expr(regexp_ret, func->return_type.items);
		for (int j = 0; j < i; j++) {
			matched = matched && match_expr(regexp_args[j], func->arguments.items);
		}
		if (matched) {
			nob_da_append(&funcs, *func);
			clang_disposeString(name);
		}
	}

	return CXChildVisit_Recurse;
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

/**
 * Parses the specified source file and returns a ParsedFile struct containing the parsed translation unit, its diagnostics, and the list of function declarations found in the file.
 *
 * @param source_file The path to the source file to be parsed.
 * @return A ParsedFile struct containing the parsed translation unit, its diagnostics, and the list of function declarations.
 */
ParsedFile parse_file(const char *source_file) {
	nob_log(NOB_INFO, "Parsing file: %s", source_file);

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

	Children children = { 0, 0, NULL };
	CXCursor cursor = clang_getTranslationUnitCursor(tu);
	nob_log(NOB_INFO, "Getting children...");
	clang_visitChildren(cursor, get_children, &children);

	return (ParsedFile) { index, tu, children, { 0, 0, NULL } };
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

CFunction* getFuncFromCursor(CXCursor cursor) {
	CFunction *func = (CFunction *) calloc(1, sizeof(CFunction));
	memset(func, 0, sizeof(CFunction));

	CXSourceLocation loc = clang_getCursorLocation(cursor);
	clang_getSpellingLocation(loc, NULL, &func->line, &func->column, NULL);

	CXString name = clang_getCursorSpelling(cursor);
	nob_sb_append_cstr(&func->name, clang_getCString(name));

	CXString signature = clang_getTypeSpelling(clang_getCursorType(cursor));
	char *sig_t = (char *) clang_getCString(signature);
	char sig[strlen(sig_t) + 1];
	strcpy(sig, sig_t);

	char *ret = strtok(sig, ",()");
	nob_sb_append_cstr(&func->return_type, ret);

	int argc = clang_Cursor_getNumArguments(cursor);
	while (ret != NULL) {
		ret = strtok(NULL, " ,()");
		if (ret != NULL) {
			nob_sb_append_cstr(&func->arguments, ret);
			if (argc-- > 1)
				nob_sb_append_cstr(&func->arguments, ", ");
		}
	}


	clang_disposeString(name);
	clang_disposeString(signature);

	return func;
}

/**
 * Prints information about the top matching functions from the given cursors, up to the specified limit.
 *
 * @param limit The maximum number of functions to print.
 * @param cursors The list of CXCursor objects representing the matching functions.
 * @param normalized_query The normalized query string used to sort the cursors.
 * @param source_file The path to the source file containing the matching functions.
 */
void coogle(size_t limit, Children *cursors, const char *normalized_query, const char *source_file) {
	Children children = *cursors;

	printf("\n");
	qsort_r(children.items, children.count, sizeof(CXCursor), compare_cursors_r, (void *) normalized_query);
	for (size_t count = 0; count < children.count /* && count < limit*/; count++) {

		CFunction *func = getFuncFromCursor(children.items[count]);

		fprintf(stdout, "%s:%4d:%2d - %s :: %s(%s)",
				source_file, func->line, func->column,
				func->return_type.items, func->name.items, func->arguments.items);

		nob_sb_free(func->name);
		nob_sb_free(func->return_type);
		nob_sb_free(func->arguments);
		free(func);
		fprintf(stdout, "\n");
	}
	fprintf(stdout, "\n");
}




ParsedFile parse_file_new(const char* source_file, const char* query) {
	nob_log(NOB_INFO, "Parsing and matching in file: %s", source_file);

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

	char *regexp_ret = strtok((char *) query, ",()");
	char *regexp_args[10];
	char *token = strtok(NULL, ",()");
	int i = 0;

	nob_log(NOB_INFO, "Ret: %s", regexp_ret);
	while (token != NULL) {
		regexp_args[i++] = token;
		token = strtok(NULL, ",()");
		nob_log(NOB_INFO, "Arg: %s", regexp_args[i - 1]);
	}

	struct ClientData_t {
		char* regexp_ret;
		char** regexp_args;
		int argc;
		CFunctions funcs;
	} ClientData_t = { regexp_ret, regexp_args, i, { 0, 0, NULL } };
	CXCursor cursor = clang_getTranslationUnitCursor(tu);
	clang_visitChildren(cursor, get_funcs, &ClientData_t);
	nob_log(NOB_INFO, "CFunctions Got!");

	return (ParsedFile) { index, tu, { 0, 0, NULL }, ClientData_t.funcs };
}

void coogle_new(CFunctions* funcs, const char* query, const char* src_file) {
	CFunctions functions = *funcs;

	nob_log(NOB_INFO, "Printing functions...");
//	qsort_r(functions.items, functions.count, sizeof(CXCursor), compare_functions_r, (void *) query);
//	nob_log(NOB_INFO, "Sorted functions");
	for (size_t count = 0; count < functions.count; count++) {

		CFunction function = functions.items[count];
		nob_log(NOB_INFO, "%2zu - %s:%4d:%2d - %s :: %s(%s)",
				count + 1, src_file, function.line, function.column,
				function.return_type.items, function.name.items, function.arguments.items);

		nob_sb_free(function.name);
		nob_sb_free(function.return_type);
		nob_sb_free(function.arguments);
		free(&functions.items[count]);
	}
	nob_da_free(functions);
}
