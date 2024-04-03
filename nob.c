#define NOB_IMPLEMENTATION
#include "nob.h"

#include <string.h>

int main(int argc, char **argv) {
	NOB_GO_REBUILD_URSELF(argc, argv);

	const char* program = nob_shift_args(&argc, &argv);

	Nob_Cmd cmd = {0};
	nob_cmd_append(&cmd, "gcc");
	nob_cmd_append(&cmd, "main.c");
	nob_cmd_append(&cmd, "-I", "/usr/lib/llvm-14/include/");
	nob_cmd_append(&cmd, "/usr/lib/llvm-14/lib/libclang-14.so");
	nob_cmd_append(&cmd, "-o", "main");
	nob_cmd_append(&cmd, "-Wextra", "-ggdb", "-pedantic");

	if (!nob_cmd_run_sync(cmd)) return 1;

	if (argc > 0) {
		const char* subcmd = nob_shift_args(&argc, &argv);
		
		if (strcmp(subcmd, "build") == 0) {
			return 0;
		} else if  (strcmp(subcmd, "run") == 0) {
			cmd.count = 0;
			nob_cmd_append(&cmd, "./main");
			nob_da_append_many(&cmd, argv, argc);
			if (!nob_cmd_run_sync(cmd)) return 1;
		} else if (strcmp(subcmd, "clean") == 0) {
			cmd.count = 0;
			nob_cmd_append(&cmd, "rm", "main", "nob.old");
			if (!nob_cmd_run_sync(cmd)) return 1;
		} else {
			fprintf(stderr, "Unknown subcommand: %s\n", subcmd);
			return 1;
		}
	}

	return 0;
	
}
