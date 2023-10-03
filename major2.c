#include "major2.h"

int main(int argc, char *argv[])
{
	char prompt[100];

	if (argc == 1) {
		printf("Enter wanted prompt: "); //customize prompt
		scanf("%s", prompt);
		fflush(stdin);
		interactive(prompt);
	}
	else if (argc==2)
		batch(argv[1]);
	else
		fprintf(stderr, "too many arguments\n");

	return 1;
}