#include "clang-c/CXString.h"
#include <clang-c/Index.h> 
// Resolved with -I /usr/lib/llvm-14/include/
// + /usr/lib/llvm-14/lib/libclang-14.so 
#include <stdio.h>
#include <unistd.h>

#define NOB_IMPLEMENTATION
#include "nob.h"

typedef struct {
	size_t count;
	size_t capacity;
	CXCursor *items;
} Children;

typedef struct {
	int argc;
	Nob_String_View name;
	Nob_String_Builder args;
	Nob_String_View return_type;
} Function;

enum CXChildVisitResult get_children(CXCursor cursor, CXCursor parent, CXClientData client_data) {

	if (clang_Location_isFromMainFile(clang_getCursorLocation(cursor)) == 0) {
		return CXChildVisit_Continue;
	}

	if (cursor.kind == CXCursor_FunctionDecl) {
		Children *children = (Children *) client_data;
		nob_da_append(children, cursor);
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

	Children children = { 0, 0, NULL };

	CXCursor cursor = clang_getTranslationUnitCursor(tu);
	clang_visitChildren(cursor, get_children, &children);

	for (size_t count = 0; count < children.count && count < 10; count++) {

		CXFile file;
		unsigned line, column;
		CXSourceLocation loc = clang_getCursorLocation(children.items[count]);
		CXString filename = clang_getFileName(file);
		clang_getSpellingLocation(loc, &file, &line, &column, NULL);
		const char *func_loc = clang_getCString(filename);

		nob_log(NOB_INFO, "\r%s: %d:%d", func_loc, line, column);


		// CXPrintingPolicy policy = clang_getCursorPrintingPolicy(cursor);	
		// CXString sig = clang_getCursorPrettyPrinted(children.items[count], policy);

		// CXType clang_getArgType(CXType T, unsigned i);

		Function curr = { 0 };

		// Cursor type is that of a Function
		CXType crsr_type = clang_getCursorType(children.items[count]);
		CXType f_type = clang_getResultType(crsr_type);
		CXString type_kind = clang_getTypeKindSpelling(f_type.kind);
		int f_argc = clang_Cursor_getNumArguments(children.items[count]);

		// nob_log(NOB_INFO, "\rSignature - %s", clang_getCString(sig));
		nob_log(NOB_INFO, "\rReturns - %s", clang_getCString(type_kind));

		clang_disposeString(type_kind);
		// clang_disposeString(sig);
	}

	clang_disposeTranslationUnit(tu);
	clang_disposeIndex(index);
	nob_da_free(children);

	nob_log(NOB_INFO, "Exit.");

	return 0;
}
