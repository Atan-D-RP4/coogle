#include <tree_sitter/api.h>
#include <stdio.h>

#define NOB_IMPLEMENTATION
#include "include/nob.h"

TSLanguage *tree_sitter_c();

void print_children(TSNode node, int depth, int max_depth) {
    // Thank you Copilot
    if (depth >= max_depth) return;

    for (size_t i = 0; i < ts_node_child_count(node); i++) {
        TSNode child = ts_node_child(node, i);
        for (int j = 0; j < depth; j++) {
            printf("  ");
        }

            printf("%s - (%d)\n", ts_node_type(child), ts_node_child_count(child));
        print_children(child, depth + 1, max_depth);
    }
}

int main(int argc, char **argv) {
	const char *program = nob_shift_args(&argc, &argv);
	if (argc <= 0) {
		nob_log(NOB_ERROR, "No input provided\n", program);
		nob_log(NOB_ERROR, "Usage: %s <file_path>\n", program);
		return 1;
	}

    //
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_c());

    printf("Hello, Tree-sitter!\n");

	const char *source_file = nob_shift_args(&argc, &argv);
	Nob_String_Builder buf = {0};

	if (!nob_read_entire_file(source_file, &buf)) return 1;

    TSTree *tree = ts_parser_parse_string(
        parser,
        NULL,
        buf.items,
        buf.count
    );

    TSNode root = ts_tree_root_node(tree);
    nob_log(NOB_INFO, "Root node is: %s\n", ts_node_type(root));

    print_children(root, 0, 50);

//   for (size_t i = 0; i < ts_node_child_count(root); i++) {
//      nob_log(NOB_INFO, "Child %2zu is: %s", i, ts_node_type(ts_node_child(root, i)));
//   }

    ts_tree_delete(tree);
    ts_parser_delete(parser);
    //
    return 0;
}
