#define NOB_IMPLEMENTATION
#include "include/nob.h"

#include <string.h>

int main(int argc, char **argv) {
	NOB_GO_REBUILD_URSELF(argc, argv);

	const char* program = nob_shift_args(&argc, &argv);
	const char* src_file = "src/coogle-main.c";
	const char* out_file = "coogle";

	if (nob_file_exists(src_file) < 0){
		nob_log(NOB_ERROR, "%s Not found", src_file);
		return 1;
	}

	// TODO: Implement this :- clang -DSTB_C_LEXER_IMPLEMENTATION -x c -c stb_c_lexer.h
	// I don't need to do this here, but it's interesting so I'll just leave it
	// in here.

	Nob_Cmd cmd = {0};
	nob_cmd_append(&cmd, "gcc");
	nob_cmd_append(&cmd, src_file);
	nob_cmd_append(&cmd, "-I", "./");
	nob_cmd_append(&cmd, "lib/libclang-14.so");
	nob_cmd_append(&cmd, "-o", out_file);
	nob_cmd_append(&cmd, "-Wall", "-Wextra", "-ggdb", "-pedantic");

	if (argc == 0) {
		nob_log(NOB_INFO, "Building %s\n", out_file);
		if (!nob_cmd_run_sync(cmd)) return 1;
	} else if (argc > 0) {
		const char* subcmd = nob_shift_args(&argc, &argv);
		if  (strcmp(subcmd, "run") == 0) {
			if (!nob_cmd_run_sync(cmd)) return 1;
			nob_log(NOB_INFO, "Running %s\n", out_file);
			cmd.count = 0;
			Nob_String_Builder run_cmd = { 0 };
			nob_sb_append_cstr(&run_cmd, "./");
			nob_sb_append_cstr(&run_cmd, out_file);
			nob_cmd_append(&cmd, run_cmd.items);
			nob_da_append_many(&cmd, argv, argc);
			if (!nob_cmd_run_sync(cmd)) return 1;
			nob_sb_free(run_cmd);
		} else if (strcmp(subcmd, "clean") == 0) {
			cmd.count = 0;
			nob_cmd_append(&cmd, "rm", "coogle", "nob", "nob.old");
			if (!nob_cmd_run_sync(cmd)) return 1;
			return 0;
		} else if (strcmp(subcmd, "test") == 0) {
			if (!nob_cmd_run_sync(cmd)) return 1;
			cmd.count = 0;
			Nob_String_Builder run_cmd = { 0 };
			nob_sb_append_cstr(&run_cmd, "./");
			nob_sb_append_cstr(&run_cmd, out_file);
			nob_cmd_append(&cmd, run_cmd.items);
			nob_cmd_append(&cmd, "clang-c/Index.h", "CXCursorKind(CXCursor)");
			if (!nob_cmd_run_sync(cmd)) {
				nob_sb_free(run_cmd);
				return 1;
			}
			nob_sb_free(run_cmd);

			return 0;
		} else {
			nob_log(NOB_ERROR, "Unknown subcommand: %s\n", subcmd);
			return 1;
		}
	}

	return 0;

}
