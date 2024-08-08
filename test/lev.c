#define NOB_IMPLEMENTATION
#include "../include/nob.h"
#include <stdio.h>

int min(int a, int b, int c) {
	return (a < b) ? ((a < c) ? a : c) : ((b < c) ? b : c);
}

int levenstein_distance(Nob_String_View a, Nob_String_View b) {
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
            if (a.data[i - 1] == b.data[j - 1])
                matrix[i][j] = matrix[i - 1][j - 1];
            else
                matrix[i][j] = 1 + min(matrix[i - 1][j], matrix[i][j - 1], matrix[i - 1][j - 1]);
        }
    }

	for (int i = 0; i <= len1; i++) {
		for (int j = 0; j <= len2; j++) {
			printf("%d ", matrix[i][j]);
		}
		printf("\n");
	}
	printf("\n");

    // The bottom-right cell of the matrix contains the Levenshtein distance
    return matrix[len1][len2];
}

int main() {
	Nob_String_View a = nob_sv_from_cstr("add");
	Nob_String_View b = nob_sv_from_cstr("daddy");
	levenstein_distance(a, b);
	return 0;
}
