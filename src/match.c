#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int match_expr(char *regexp, char *text);
int match_here(char *regexp, char *text);
int match_star(int c, char *regexp, char *text);

/* match: search for regexp anywhere in text */
int match_expr(char *regexp, char *text)
{
	if (regexp[0] == '^')
		return match_here(regexp+1, text);
	do {    /* must look even if string is empty */
		if (match_here(regexp, text))
			return 1;
	} while (*text++ != '\0');
	return 0;
}

/* matchhere: search for regexp at beginning of text */
int match_here(char *regexp, char *text)
{
	if (regexp[0] == '\0')
		return 1;
	if (regexp[1] == '*')
		return match_star(regexp[0], regexp+2, text);
	if (regexp[0] == '$' && regexp[1] == '\0')
		return *text == '\0';
	if (*text!='\0' && (regexp[0]=='.' || regexp[0]==*text))
		return match_here(regexp+1, text+1);
	return 0;
}

/* matchstar: search for c*regexp at beginning of text */
int match_star(int c, char *regexp, char *text)
{
	char *t;

	for (t = text; *t != '\0' && (*t == c || c == '.'); t++);

	do {    /* * matches zero or more */
		if (match_here(regexp, t))
			return 1;
	} while (t-- > text);

	return 0;
}

int grep(char *regexp, FILE *f, char *name) {
	int n, nmatch;

	char buf [BUFSIZ] ;

	nmatch = 0;

	while (fgets(buf, sizeof buf, f) != NULL) {
		n = strlen(buf);

		if (n > 0 && buf [n-1] == '\n')
			buf[n-1] = '\0' ;

		if (match_expr(regexp, buf)) {
			nmatch++;

			if (name != NULL) printf("%s : ", name);

			printf("%s\n", buf) ;
		}
	}

		return nmatch;
}

//int main(int argc, char *argv[]) {
//	if (argc < 2) {
//		fprintf(stderr, "Usage: %s regexp [file ...]\n", argv[0]);
//		exit(1);
//	}
//	int nmatch = 0;
//	if (argc == 2) {
//		if (grep(argv[1], stdin, NULL) > 0)
//			nmatch++;
//	} else {
//		for (int i = 2; i < argc; i++) {
//			FILE *f = fopen(argv[i], "r");
//			if (f == NULL) {
//				perror(argv[i]);
//				continue;
//			}
//			if (grep(argv[1], f, argc > 3 ? argv[i] : NULL) > 0)
//				nmatch++;
//			fclose(f);
//		}
//	}
//
//	return 0;
//}
