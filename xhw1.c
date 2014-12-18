/*
 * FileName	: xhw1.c
 * Subject	: Homework Assignment #1, Gautham
 * Date		: Mar-09-2014
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include "xhw1.h"

#define __NR_xconcat    349     /* our private syscall number */

/*
 * Description : Take the parameters from user and fills the inputParameter
 *		 structure and passes it to the system call xconcat.
 *
 * Default Values : When no flags are passed by user the default flags for
 *			oflags - O_RDWR | O_APPEND
 *			flags - 0
 *			mode - 744
 * Return Value	 : Number of bytes copied, or files copied or,
		 : %age of content copied based on flags
*/
int main(int argc, char *argv[])
{
	struct inputParameters inp;
	int rc, i = 0, j = 0;
	int flags, opt, special_flag;
	char *str_mode = (char *)malloc(sizeof(char));
	inp.mode = strtol("0744", NULL, 8);	/*Default permission*/
	special_flag = 0;
	flags = 0;	/*Default flags*/

	if (argc < 3)
		fprintf(stderr, "Usage: %s[flags(-a,-c,-e,-t,-A,-N,-P,-m)]"
			"outfile infile1 infile2 ...\n", argv[0]);

	while ((opt = getopt(argc, argv, "acteANPm:h")) != -1) {
		switch (opt) {
		case 'a':
				flags |= O_APPEND;
			break;
		case 'c':
				flags |= O_CREAT;
			break;
		case 't':
				flags |= O_TRUNC;
			break;
		case 'e':
				flags |= O_EXCL;
				break;
		case 'A':
				special_flag += 4;
			break;
		case 'P':
			special_flag += 2;
			break;
		case 'N':
			special_flag += 1;
			break;
		case 'm':
			if (strlen(optarg) > 4) {
				printf("Error: Invalid mode\n");
				exit(EXIT_FAILURE);
			}
			inp.mode = strtol(optarg, NULL, 8);
			sprintf(str_mode, "%o", inp.mode);
			if (strcmp(str_mode, optarg) != 0) {
				printf("Error: Invalid mode\n");
				free(str_mode);
				exit(0);
			}
			break;
		case '?':
		default:
			goto Syntax; /* 'h' help */
		}
	}
	if (special_flag == 0)
		inp.flags = 0;
	else
		inp.flags = special_flag;

	if (flags == 0)
		inp.oflags = O_RDWR|O_APPEND;
	else
		inp.oflags = flags|O_RDWR;
	if (optind >= argc) {
		fprintf(stderr, "Expected argument after options\n");
		exit(EXIT_FAILURE);
	}

	inp.outfile = (char *)malloc(sizeof(char));
	inp.outfile = argv[optind];
	inp.infile_count = argc-optind-1;
	inp.infiles = malloc((argc-optind-1) * sizeof(char));

	/*Getting all the input files*/
	for (i = (optind + 1), j = 0; i < argc; i++, j++) {
		inp.infiles[j] = (char *)
				malloc(sizeof(argv[i]) * sizeof(char *));
		inp.infiles[j] = argv[i];
	}

	void *dummy = (void *) &inp;
	printf("%d",sizeof(struct inputParameters));
	rc = syscall(__NR_xconcat, dummy, sizeof(struct inputParameters));

	if (rc >= 0) {
		switch (inp.flags) {
		case 1:
		case 5:
			printf("successful : %d files\n", rc);
			break;
		case 2:
		case 6:
			printf("successful : %d percentage\n", rc);
			break;
		default:
			printf("successful : %d bytes\n", rc);
			break;
		}
	} else
		printf("Failed  errno = %d (%s)\n",
			 errno, strerror(errno));
	exit(rc);

Syntax: fprintf(stderr, "Usage: %s[flags(-a,-c,-e,-t,-m,-A,-N,-P)]"
			"outfile infile1 infile2 ...\n", argv[0]);
	exit(EXIT_FAILURE);
}
