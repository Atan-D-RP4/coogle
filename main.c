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
	int argc;
	Nob_String_View name;
	Nob_String_View args;
	Nob_String_View return_type;
	Nob_String_View location;
	unsigned line;
	unsigned column;
} Function;

typedef struct {
	size_t count;
	size_t capacity;
	Function *items;
} da_Function;

da_Function coogle_funcs = { 0, 0, NULL };

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

int functions = 0;

void getFunctions(Children children, Function *coogle_funcs, const char* source_file) {

	for (size_t count = 0; count < children.count; count++) {

		Function curr = coogle_funcs[count];

		CXSourceLocation loc = clang_getCursorLocation(children.items[count]);
		clang_getSpellingLocation(loc, NULL, &curr.line,
				&curr.column, NULL);
		curr.location = nob_sv_from_cstr(source_file);

		CXType crsr_type = clang_getCursorType(children.items[count]);
		CXType f_type = clang_getResultType(crsr_type);
		CXString type_kind = clang_getTypeSpelling(f_type);
		curr.argc = clang_Cursor_getNumArguments(children.items[count]);

		Nob_String_Builder args = { 0 };

		for (int i = 0; i < curr.argc; i++) {
			CXString arg = clang_getTypeSpelling(clang_getArgType(crsr_type, i));

			nob_sb_append_cstr(&args, clang_getCString(arg));
			nob_sb_append_cstr(&args, ", ");

			clang_disposeString(arg);
		}

		if (curr.argc == 0) {
			curr.args = nob_sv_from_cstr("void");
		} else {
			curr.args.count = args.count;
			curr.args.data = args.items;
		}

		nob_sb_append_cstr(&args, clang_getCString(clang_getCursorSpelling(children.items[count])));
		curr.name.count = args.count;
		curr.name.data = args.items;

		nob_sb_append_cstr(&args, clang_getCString(type_kind));
		curr.return_type.count = args.count;
		curr.return_type.data = args.items;

		//nob_log(NOB_INFO, "\r%s: %d:%d: %s :: %s :: %s", curr.location.data, curr.line, curr.column, curr.return_type.data, curr.name.data, curr.args.data);
		clang_disposeString(type_kind);
		
	}
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

	CXFile file = clang_getFile(tu, source_file);
	CXString filename = clang_getFileName(file);

	Function coogle_funcs[children.count];

	getFunctions(children, coogle_funcs, clang_getCString(filename));

//	for (size_t count = 0; count < children.count; count++) {
//
//		Function *curr = &coogle_funcs[count];
//
//		CXSourceLocation loc = clang_getCursorLocation(children.items[count]);
//		clang_getSpellingLocation(loc, NULL, &curr.line,
//				&curr.column, NULL);
//		curr.location = nob_sv_from_cstr(clang_getCString(filename));
//
//		CXType crsr_type = clang_getCursorType(children.items[count]);
//		CXType f_type = clang_getResultType(crsr_type);
//		CXString type_kind = clang_getTypeSpelling(f_type);
//		curr.argc = clang_Cursor_getNumArguments(children.items[count]);
//		Nob_String_Builder args = { 0 };
//
//		// nob_log(NOB_INFO, "\rSignature - %s", clang_getCString(sig));
//
//		for (int i = 0; i < curr.argc; i++) {
//			CXString arg = clang_getTypeSpelling(clang_getArgType(crsr_type, i));
//
//			nob_sb_append_cstr(&args, clang_getCString(arg));
//			nob_sb_append_cstr(&args, ", ");
//
//			clang_disposeString(arg);
//		}
//
//		if (curr.argc == 0) {
//			curr.args = nob_sv_from_cstr("Void");
//		} else {
//			curr.args.count = args.count;
//			curr.args.data = args.items;
//		}
//
//		curr.name = nob_sv_from_cstr(clang_getCString(clang_getCursorSpelling(children.items[count])));
//		curr.return_type = nob_sv_from_cstr(clang_getCString(type_kind));
//		
//		nob_log(NOB_INFO, "\r%s: %d:%d", curr.location.data, curr.line, curr.column);
//		nob_log(NOB_INFO, "\r%s :: %s :: "SV_Fmt, curr.name.data, curr.return_type.data, SV_Arg(curr.args));
//		clang_disposeString(type_kind);
//	}


	clang_disposeString(filename);
	clang_disposeTranslationUnit(tu);
	clang_disposeIndex(index);
	nob_da_free(children);

	nob_log(NOB_INFO, "Exit.");

	return 0;
}
