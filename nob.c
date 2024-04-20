#define NOB_IMPLEMENTATION
#include "include/nob.h"

#include <string.h>

int main(int argc, char **argv) {
	NOB_GO_REBUILD_URSELF(argc, argv);
	nob_log(NOB_INFO, "Hello, World!");

	const char* program = nob_shift_args(&argc, &argv);

	// TODO: Implement this :- clang -DSTB_C_LEXER_IMPLEMENTATION -x c -c stb_c_lexer.h
	// I don't need to do this here, but it's interesting so I'll just leave it
	// in here.

	Nob_Cmd cmd = {0};
	nob_cmd_append(&cmd, "gcc");
	nob_cmd_append(&cmd, "./src/coogle-main.c");
	nob_cmd_append(&cmd, "-I", "./");
	nob_cmd_append(&cmd, "lib/libclang-14.so");
	nob_cmd_append(&cmd, "-o", "coogle");
	nob_cmd_append(&cmd, "-Wall", "-Wextra", "-ggdb", "-pedantic");


	if (argc > 0) {
		const char* subcmd = nob_shift_args(&argc, &argv);
		if  (strcmp(subcmd, "run") == 0) {
			if (!nob_cmd_run_sync(cmd)) return 1;
			cmd.count = 0;
			nob_cmd_append(&cmd, "./coogle");
			nob_da_append_many(&cmd, argv, argc);
			if (!nob_cmd_run_sync(cmd)) return 1;
		} else if (strcmp(subcmd, "clean") == 0) {
			cmd.count = 0;
			nob_cmd_append(&cmd, "rm", "coogle", "nob", "nob.old");
			if (!nob_cmd_run_sync(cmd)) return 1;
			return 0;
		} else {
			fprintf(stderr, "Unknown subcommand: %s\n", subcmd);
			return 1;
		}
	}

	if (!nob_cmd_run_sync(cmd)) return 1;
	return 0;
	
}
