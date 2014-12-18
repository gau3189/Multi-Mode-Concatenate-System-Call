/*
 * FileName     : xhw1.h
 * Subject      : Homework Assignment #1, Gautham
 * Date         : Mar-09-2014
*/

#define __user

/* Parameters struture for the XCONCAT system call
 * outfile	: Name of the outputfile
 * infiles	: Array of Input Files to be copied
 * infile_count : Number of fiels to be copied
 * oflags	: Flags for the output file
 * mode		: Permissions for the output file creation
 * flags	: Atomic behaviour or the return value
 *		: of the system call
*/
struct inputParameters {
	__user const char *outfile;
	__user const char **infiles;
	unsigned int infile_count;
	int oflags;
	mode_t mode;
	unsigned int flags;
};
