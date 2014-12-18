#Subject	: Homework Assignment #1
#Name		: Gautham Reddy Kunta
#Date   	: Mar-09-2014


Command Line Arguments
======================
------------------------------------------------------
Example:  ./xhw1 [flags] outfile infile1 infile2 ....
-------------------------------------------------------
[flags]	- optional
-a: append mode (O_APPEND)
-c: O_CREATE
-t: O_TRUNC
-e: O_EXCL
-A: Atomic concat mode (extra credit only)
-N: return num-files instead of num-bytes written
-P: return percentage of data written out
-m ARG: set default mode to ARG (e.g., octal 755, see chmod(2) and umask(1))
-h: print short usage string

Default Values of flags (Assumptions):
------------------------------------- 
When no flags are given the default flags for output file are
	
	# O_RDWR|O_APPEND -	If the file is present then we  open it in read
				write mode and in append mode instead of 
				corrupting the file.
	# 744 mode value -	in case of O_CREAT mode is set to 744

The return value is number of bytes successfully copied.

The input files (infile1 infile2 ..... ) can be provided maximum upto 10 files.

------------
Description 
------------

Input Validations in system call:
	# whether all the input files and output file (if present) can be opened
	   else returns error.
	# whether input and output file name are same if same returns an error.
	# special flag validation
		allowed combintions are -N, -P, -A, -N -A, -P -A
	# validating the data passed from user space.

Normal Behaviour:
	# Validates all the inputs and creates the inputPrameter structure.
	# Checks for the append flag if present opens or creates the output
	   file with the given flags andsets the offset to end of the file.
	# Loops through all the files and copies data to the output file.
	# Incase of read or write fails(given bytes<0) error is returned 
	   to the user
	# If successfully or partial written to output file, it returns
	   appropriate value (no of bytes or no of files or %age of bytes)
	   successfully read and written based on special flag (-N -P or default).

Atomic Behaviour:
	Code for handling atomic behaviour is ifdef under EXTRA_CREDIT macro
	and is executed only when -A option is specified.
	
	# In this behaviour if a file is already present a temp file is created with
	   O_CREAT|O_TRUNC|O_RDWR flags and mode value of the existing output file 
    	   and a copy the content of the output file to temp file if O_TRUNC ( -t ) flag is not set.
	# If file does not exist temp file is not created instead the output file with 
	   user given flags is created.
	# If Failure occurs i.e partial write or failure during reading or writing
	    the temp file is deleted and already existing file is preserved.
	    if new file was created it is deleted.
	# Incase of success the output file already existing is deleted and the temp file is renamed 
	   with the given output file name.

Unhandled :

	# Validation of mode since if invalid mode is given a sticky bit is set by filp_open.
	# Normal octal validation is done in user program.


Functions used:
	# vfs_write()
	# vfs_read()
	# filp_open()
	# filp_close()
	# copy_from_user()
	# vfs_llseek() 
	# vfs_rename()
	# vfs_unlink()

Looked at links under class resoources "http://www.cs.sunysb.edu/~ezk/cse506-s14/" for help

