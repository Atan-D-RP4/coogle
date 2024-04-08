#include "clang-c/CXString.h"
#include <clang-c/Index.h> 
// Resolved with -I /usr/lib/llvm-14/include/
// + /usr/lib/llvm-14/lib/libclang-14.so 
#include <stdio.h>

#define NOB_IMPLEMENTATION
#define STB_C_LEXER_IMPLEMENTATION 
#include "include/nob.h"
#include "include/stb_c_lexer.h"

// TODO: Fix the copy_and_dispose function. Somehow

typedef struct {
	size_t count;
	size_t capacity;
	CXCursor *items;
} Children;

typedef struct {
	unsigned line;
	unsigned column;
	int argc;
	Nob_String_Builder name;
	Nob_String_Builder args;
	Nob_String_Builder return_type;
	Nob_String_Builder file_name;
	Nob_String_Builder signature;
} Function;

typedef struct {
	Function *items;
	size_t count;
	size_t capacity;
} da_Function;

da_Function coogle_funcs = { NULL, 0, 0 };

int visisted = 0;
int functions = 0;
Nob_String_Builder normalized;

Nob_String_Builder normalize_string(const char *query);
enum CXChildVisitResult get_functions(CXCursor cursor, CXCursor parent, CXClientData client_data);
void parse_file(const char* source_file, da_Function *coogle_funcs);
const char *copy_and_dispose(CXString str);
void dispose_funcs();
void print_lexer(stb_lexer lex);
Nob_String_Builder normalize_string(const char *query);
int levenstein_distance(Nob_String_Builder a, Nob_String_Builder b);

Nob_String_Builder sb_copy_and_dispose(CXString str) {
	Nob_String_Builder sb_copy = { NULL, 0, 0 };
	const char *cstr = clang_getCString(str);
	size_t len = strlen(cstr);
	char* copy = (char*)malloc(len + 1);
	if (copy) {
		memcpy(copy, cstr, len);
		copy[len] = '\0';
	}
	clang_disposeString(str);
	
	return sb_copy;
}

const char *copy_and_dispose(CXString str) {
	const char *cstr = clang_getCString(str);
	size_t len = strlen(cstr);
    char* copy = (char*)malloc(len + 1);
    if (copy) {
        memcpy(copy, cstr, len);
        copy[len] = '\0';
    }
	clang_disposeString(str);
	return copy;
}

enum CXChildVisitResult get_functions(CXCursor cursor, CXCursor parent, CXClientData client_data) {

	visisted++;
	if (clang_Location_isFromMainFile(clang_getCursorLocation(cursor)) == 0) {
		return CXChildVisit_Continue;
	}

	if (cursor.kind == CXCursor_FunctionDecl) {

		Function curr = { 0, 0, 0, { NULL, 0, 0 }, { NULL, 0, 0 }, { NULL, 0, 0 }, { NULL, 0, 0 }, {NULL, 0 , 0 }};

		CXFile file;
		

		clang_getSpellingLocation(clang_getCursorLocation(cursor), &file, &curr.line,
				&curr.column, NULL);

		// Get the name of the file function is in and append it 
		nob_sb_append_cstr(&curr.file_name, copy_and_dispose(clang_getFileName(file)));

		// Ref: copy_and_dispose(clang_getTypeSpelling(clang_getCursorResultType(cursor)));
		// Get the return type of the function and append it
		nob_sb_append_cstr(&curr.return_type,
				copy_and_dispose(clang_getTypeSpelling(clang_getResultType(clang_getCursorType(cursor)))));

		// Get the name of the function and append it
		nob_sb_append_cstr(&curr.name, copy_and_dispose(clang_getCursorSpelling(cursor)));

		// Get the arguments of the function and append them
		curr.argc = clang_Cursor_getNumArguments(cursor);
		if (curr.argc == -1) {
			nob_log(NOB_ERROR, "Unable to get the number of arguments for %s", curr.name.items);
		} else if (curr.argc == 0) {
			// Set the arguments to 'void' if there are none
			nob_sb_append_cstr(&curr.args, "void");
		} else {
			for (int i = 0; i < curr.argc; i++) {
				// Get the i'th argument of the function and append it
				nob_sb_append_cstr(&curr.args,
						copy_and_dispose(clang_getTypeSpelling(clang_getArgType(clang_getCursorType(cursor), i))));
				nob_sb_append_cstr(&curr.args, ", ");
			}
		}

		// Get the full signature of the Function
		nob_sb_append_cstr(&curr.signature ,
				normalize_string(copy_and_dispose(clang_getTypeSpelling(clang_getCursorType(cursor)))).items);

		// Append the constructed function to the dynamic array
		nob_da_append((da_Function *)client_data, curr);
	}
	
	return CXChildVisit_Recurse;
}


void parse_file(const char* source_file, da_Function *coogle_funcs) {
	
	CXIndex index = clang_createIndex(0, 0);

	CXTranslationUnit tu; 

	struct CXUnsavedFile temp_files[] = { NULL };
	
	enum CXTranslationUnit_Flags flags = CXTranslationUnit_DetailedPreprocessingRecord;

	enum CXErrorCode err = clang_parseTranslationUnit2(index, source_file, NULL, 0, 
			temp_files, temp_files->Length, flags, &tu);

	if (err != CXError_Success) {
		clang_disposeIndex(index);
		clang_disposeTranslationUnit(tu);
		nob_log(NOB_ERROR, "Unable to parse translation unit. Quitting.");
		return;
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
	clang_visitChildren(cursor, get_functions, coogle_funcs);

	clang_disposeTranslationUnit(tu);
	clang_disposeIndex(index);

}


void print_funcs(da_Function *coogle_funcs, const char *query, size_t limit) {
	
	printf("\n-----------------------------------\n\n");

	for (size_t count = 0; count < coogle_funcs->count && count < limit; ++count) {
		Function *curr = &coogle_funcs->items[count];

		nob_log(NOB_INFO, "\r%s: %d:%d: %s :: %s",
				curr->file_name.items, curr->line, curr->column,
				curr->name.items, curr->signature.items);
		}
//		printf("Sig - %s\n", curr->signature.items);
//		nob_log(NOB_INFO, "\r%s: %d:%d: %s :: %s :: %s",
//				curr->file_name.items, curr->line, curr->column,
//				curr->return_type.items, curr->name.items, curr->args.items);

	printf("\n-----------------------------------\n\n");
}


void dispose_funcs() {
	for (size_t count = 0; count < coogle_funcs.count; ++count) {
		Function *curr = &coogle_funcs.items[count];
		nob_sb_free(curr->file_name);
		nob_sb_free(curr->name);
		nob_sb_free(curr->args);
		nob_sb_free(curr->return_type);
		nob_sb_free(curr->signature);
	}
	nob_da_free(coogle_funcs);
}


void print_lexer(stb_lexer lex) {
	printf("Lexer: [%p, %p, %p, %s, %d, %p, %p, %ld, %lf, %ld, %p, %d]\n",
			lex.input_stream, lex.eof, lex.parse_point,
			lex.string_storage, lex.string_storage_len, lex.where_firstchar,
			lex.where_lastchar, lex.token, lex.real_number, lex.int_number,
			lex.string, lex.string_len);
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

int min(int a, int b, int c) {
	return (a < b) ? ((a < c) ? a : c) : ((b < c) ? b : c);
}

int levenstein_distance(Nob_String_Builder a, Nob_String_Builder b) {
	int len1 = a.count;
    int len2 = b.count;

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
            if (a.items[i - 1] == b.items[j - 1])
                matrix[i][j] = matrix[i - 1][j - 1];
            else
                matrix[i][j] = 1 + min(matrix[i - 1][j], matrix[i][j - 1], matrix[i - 1][j - 1]);
        }
    }

    // The bottom-right cell of the matrix contains the Levenshtein distance
    return matrix[len1][len2];
}

int compare_functions(const void *a, const void *b) {
	Function *fa = (Function *)a;
	Function *fb = (Function *)b;

	return levenstein_distance(fa->signature, normalized) - levenstein_distance(fb->signature, normalized);
}

int main(int argc, char *argv[]) {

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <filename> <query>\n", argv[0]);
		return 1;
	}

	const char *source_file = argv[1];

	da_Function coogle_funcs = { NULL, 0, 0 };
	parse_file(source_file, &coogle_funcs);


	normalized = normalize_string(argv[2]);
	nob_log(NOB_INFO, "Normalized Query: %s", normalized.items);

	//qsort(coogle_funcs.items, coogle_funcs.count, sizeof(Function), compare_functions);

	print_funcs(&coogle_funcs, normalized.items, coogle_funcs.count);

	dispose_funcs();
	nob_sb_free(normalized);
	nob_log(NOB_INFO, "Exit.");

	return 0;
}
