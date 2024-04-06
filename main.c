#include "clang-c/CXString.h"
#include <clang-c/Index.h> 
// Resolved with -I /usr/lib/llvm-14/include/
// + /usr/lib/llvm-14/lib/libclang-14.so 
#include <stdio.h>
#include <unistd.h>

#define NOB_IMPLEMENTATION
#include "nob.h"

// TODO: Convert everything in Function from String_View to String_Builder
// TODO: Check for memory leaks, other than LLVM's

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

int visisted = 0;


enum CXChildVisitResult get_children(CXCursor cursor, CXCursor parent, CXClientData client_data) {

	visisted++;
	if (clang_Location_isFromMainFile(clang_getCursorLocation(cursor)) == 0) {
		return CXChildVisit_Continue;
	}

	if (cursor.kind == CXCursor_FunctionDecl) {

		Function curr = { 0, 0, 0, { NULL, 0, 0 }, { NULL, 0, 0 }, { NULL, 0, 0 }, { NULL, 0, 0 }, {NULL, 0 , 0 }};

		CXFile file;
		
		CXSourceLocation loc = clang_getCursorLocation(cursor);

		clang_getSpellingLocation(loc, &file, &curr.line,
				&curr.column, NULL);

		// Get the name of the file function is in and append it 
		// TODO: Fucks up somewhere. Must debug
		nob_sb_append_cstr(&curr.file_name, copy_and_dispose(clang_getFileName(file)));

		// Get the return type of the function and append it
		CXType crsr_type = clang_getCursorType(cursor);
		// copy_and_dispose(clang_getTypeSpelling(clang_getCursorResultType(cursor)));
		nob_sb_append_cstr(&curr.return_type, copy_and_dispose(clang_getTypeSpelling(clang_getResultType(crsr_type))));

		// Get the name of the function and append it
		nob_sb_append_cstr(&curr.name, copy_and_dispose(clang_getCursorSpelling(cursor)));

		// Get the arguments of the function and append them
		curr.argc = clang_Cursor_getNumArguments(cursor);
		if (curr.argc == 0) {
			// Set the arguments to 'void' if there are none
			nob_sb_append_cstr(&curr.args, "void");
		} else {
			for (int i = 0; i < curr.argc; i++) {
				// Get the i'th argument of the function and append it
				nob_sb_append_cstr(&curr.args, copy_and_dispose(clang_getTypeSpelling(clang_getArgType(crsr_type, i))));
				nob_sb_append_cstr(&curr.args, ", ");
			}
		}

		// Get the full signature of the Function
		nob_sb_append_cstr(&curr.signature, copy_and_dispose(clang_getTypeSpelling(crsr_type)));

		// Append the constructed function to the dynamic array
		nob_da_append(&coogle_funcs, curr);
	}
	
	return CXChildVisit_Recurse;
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
		return 1;
	}

	const char *source_file = argv[1];

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
		return 1;
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

	clang_visitChildren(cursor, get_children, NULL);

	printf("\n-----------------------------------\n\n");

	for (size_t count = 0; count < coogle_funcs.count; ++count) {

		Function *curr = &coogle_funcs.items[count];

		printf("%s", curr->file_name.items);
		printf(":%2d:%2d: ", curr->line, curr->column);
		printf("%2s :: ", curr->return_type.items);
		printf("%2s :: ", curr->name.items);
		printf("%2s :: ", curr->args.items);
		printf("Sig - %s\n", curr->signature.items);

//		nob_log(NOB_INFO, "\r%s: %d:%d: %s :: %s :: %s",
//				curr->file_name.items, curr->line, curr->column,
//				curr->return_type.items, curr->name.items, curr->args.items);

		nob_sb_free(curr->file_name);
		nob_sb_free(curr->name);
		nob_sb_free(curr->args);
		nob_sb_free(curr->return_type);
	
	}

	printf("\n-----------------------------------\n\n");

	printf("Visited: %d\n", visisted);
	printf("Functions: %ld\n", coogle_funcs.count);

	nob_da_free(coogle_funcs);
//	nob_da_free(children);
	clang_disposeTranslationUnit(tu);
	clang_disposeIndex(index);

	nob_log(NOB_INFO, "Exit.");

	return 0;
}
