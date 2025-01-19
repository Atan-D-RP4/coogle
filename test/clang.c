#include "clang-c/CXString.h"
#include <clang-c/Index.h>

#define NOB_IMPLEMENTATION
#include "../include/nob.h"

int main() {

    const char *source_file = "./lev.c";
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
	unsigned num_diagnostics = clang_getNumDiagnostics(tu);
			CXString string = clang_formatDiagnostic(diag,
					clang_defaultDiagnosticDisplayOptions());
			nob_log(NOB_WARNING, "%s", clang_getCString(string));
			clang_disposeString(string);
		}

	} else {
		nob_log(NOB_INFO, "No diagnostics.");
	}

	clang_disposeTranslationUnit(tu);
	clang_disposeIndex(index);

	nob_log(NOB_INFO, "Exit.");

    return 0;
}
