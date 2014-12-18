/*
 * FileName     : sys_xconcat.c
 * Subject      : Homework Assignment #1, Gautham
 * Date         : Mar-09-2014
*/
#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/syscalls.h>
#include <linux/slab.h>

#include "xhw1.h"

/* Description
 * SystemCall Name	: xconcat
 * Input		: arg of type struct inputParameters
 * Return Value		: Number of bytes copied (default)
 *			: Number of files copied for -N option
 *			: Percentage of bytes copied for -P option
 *
 * The EXTRA_CREDIT macro in the module contains the code for ATOMIC behaviour
 * of the system call.
 */

asmlinkage extern long (*sysptr)(void *arg, int argslen);

static inline int validateInputs(struct inputParameters *);

asmlinkage long xconcat(void *arg, int argslen)
{
	char  *buf = NULL;
	int files_successful = 0;
	int i = 0, len = 0, result = 0;
	int bytes = 0, bytes_written = 0, total_bytes_written = 0;
	bool isAtomic = false, isAppend = false;

	mm_segment_t oldfs;
	int file_size = 0;

	struct file *fr = NULL, *fw = NULL;
	struct inputParameters *user_input = NULL, *temp = NULL;

#ifdef EXTRA_CREDIT
	struct inode *temp_inode  = NULL;
	struct dentry *temp_dentry = NULL;
#endif
	printk(KERN_INFO "Entered System Call\n");

	if (!arg || !argslen) {
		result = -EBADF;
		goto CleanUp;
	}
	if (argslen != sizeof(struct inputParameters)) {
		result = -EBADF;
		goto CleanUp;
	}

	/* ##############################################################
	 * Validating and Copying the data from user space to kernel space
	 * #############################################################*/
	temp = (struct inputParameters *)arg;

	user_input = kmalloc(argslen, GFP_KERNEL);
	if (!user_input) {
		result = -ENOMEM;
		goto CleanUp;
	}
	result = copy_from_user(user_input, (struct inputParameters *)arg,
				argslen);
	if (result) {
		result = -EFAULT;
		goto CleanUp;
	}

	user_input->outfile = getname(temp->outfile);
	user_input->infiles = kmalloc((temp->infile_count) * sizeof(char *),
					 GFP_KERNEL);

	if (!user_input->infiles) {
		result = -ENOMEM;
		goto CleanUp;
	}

	for (i = 0; i < temp->infile_count; i++) {
		user_input->infiles[i] = getname(temp->infiles[i]);
		if (IS_ERR(user_input->infiles[i])) {
			result = PTR_ERR(user_input->infiles[i]);
			goto CleanUp;
		}
	}

	printk(KERN_INFO "Done copying data from user space to kernelspace\n");
	/* #################################################################
	 * Validating and Copying the data from user space to kernel space
	 * ################################################################*/


	/*############################################################
	 * Calling validateInputs()to validate the input parameters
	 *##########################################################*/

	printk(KERN_DEBUG " Validating the input parameters\n");

	result = validateInputs(user_input);
	if (result < 0)
		goto CleanUp;

	file_size = result;
	printk(KERN_INFO "Validation Successful\n");

/*
 * Setting the isAppend = true if O_APPEND flag has been set
*/
	if (user_input->oflags & O_APPEND) {
		fr = filp_open(user_input->outfile, user_input->oflags, 0);
		if (!fr || IS_ERR(fr)) {
			if (user_input->oflags&O_CREAT)
				isAppend = false;
		} else
			isAppend = true;

		printk(KERN_DEBUG "%d File opened\n", isAppend);
		if (fr && !IS_ERR(fr))
			filp_close(fr, NULL);
	}
printk(KERN_DEBUG "Append Done %d\n", isAppend);

/*
 * Setting the isAtomic = true incase of atomic behaviour.
*/
#ifdef EXTRA_CREDIT
	switch (user_input->flags) {
	case 4:	/* #### -A  ####*/
	case 5:	/* #### -A -N ####*/
	case 6:	/* #### -A -P ####*/
		isAtomic = true;
		break;
	default:
		isAtomic = false;
		break;
	}
#else
	isAtomic = false;
#endif

/*
 * Creating the outputfile with proper flags and mode given by user.
 * Incase of ATOMIC behaviour creates a temp00077766 file
*/

	if (isAtomic) {

#ifdef EXTRA_CREDIT
		if (isAppend) {
			fr = filp_open(user_input->outfile, O_RDWR, 0);
			if (!fr || IS_ERR(fr)) {
				printk(KERN_ERR "wrapfs_read_file error\n");
				result = (int) PTR_ERR(fr);
				goto CleanUp;
			}

			fw = filp_open("temp000777666",
					O_RDWR|O_CREAT|O_TRUNC,
					fr->f_dentry->d_inode->i_mode);

			if (!fw || IS_ERR(fw)) {
				printk(KERN_ERR "error while creating file\n");
				result = (int) PTR_ERR(fr);
				goto CleanUp;
			}
			filp_close(fr, NULL);
		} else {
			fw = filp_open(user_input->outfile,
				user_input->oflags,
				user_input->mode);
			if (!fw || IS_ERR(fw)) {
				printk(KERN_ERR "file not found err\n");
				result = (int)PTR_ERR(fw);
				goto CleanUp;
			}
		}
#endif
	} else {
		printk(KERN_DEBUG "\n Open the output file\n");
		fw = filp_open(user_input->outfile,
				user_input->oflags,
				user_input->mode);

		if (!fw || IS_ERR(fw)) {
			printk(KERN_ERR "file not found err\n");
			result = (int)PTR_ERR(fw);
			goto CleanUp;
		}
	}

	printk(KERN_DEBUG"Opened\n");
	/* ########################################################
	 * Parsing each input file and writing into the output file
	 ##########################################################*/

	len = PAGE_SIZE;
	fw->f_pos = 0;
	buf = kmalloc(len, GFP_KERNEL);
	if (!buf) {
		result = -ENOMEM;
		goto CleanUp;
	}

/*
 * Validation for O_APPEND flag
 * Normal Behaviour :   IF O_APPEND flag is set places the offset at end of line
 *
 * Atomic Behaviour :   Creating the temp000777666 file and copying the content
 *                      in case of O_APPEND is set
*/
	if (isAppend && !(user_input->oflags&O_TRUNC)) {
		if (isAtomic) {

#ifdef EXTRA_CREDIT
			fr = filp_open(user_input->outfile, O_RDONLY, 0);
			if (!fr || IS_ERR(fr)) {
				printk(KERN_ERR"wrapfs_read_file err %d\n",
					(int) PTR_ERR(fr));
				result = (int) PTR_ERR(fr);
				goto CleanUp;
			}
			oldfs = get_fs();
			set_fs(KERNEL_DS);
			fr->f_pos = 0;
			do {
				bytes = vfs_read(fr, buf, len, &fr->f_pos);
				if (bytes == 0)
					break;
				bytes_written = vfs_write(fw, buf,
							bytes, &fw->f_pos);
				if (bytes_written < 0) {
					result = bytes;
					goto CleanUp;
				}
			} while (bytes >= len);
			set_fs(oldfs);
			filp_close(fr, NULL);
#endif
		} else
			fw->f_pos = vfs_llseek(fw, (loff_t)0, SEEK_END);
	}

/*
 *	Input Files :	user_input->infiles
 *	Output File :	user_input->outputfile
 *			temp000777666(if outfile already exists) for atomic.
 *	Copying the contents from Input Files to Output File
 */
	for (i = 0; i < user_input->infile_count; i++) {
		fr = NULL;
		fr = filp_open(user_input->infiles[i], O_RDONLY, 0);

		if (!fr || IS_ERR(fr)) {
			printk(KERN_ERR "wrapfs_read_file err\n");
			result = (int) PTR_ERR(fr);
			goto CleanUp;
		}
		printk(KERN_DEBUG "\n Open inputfile %s\n",
					user_input->infiles[i]);
		fr->f_pos = 0;
		oldfs = get_fs();
		set_fs(KERNEL_DS);
		do {
			bytes = 0;
			bytes = vfs_read(fr, buf, len, &fr->f_pos);
			bytes_written = vfs_write(fw, buf, bytes, &fw->f_pos);
			if (bytes_written < 0) {
				printk(KERN_ERR"Write is not successfull\n %d",
					 bytes_written);
#ifdef EXTRA_CREDIT
				if (isAtomic)
					goto ATOMIC;
#endif
					result = bytes_written;
					goto CleanUp;
				break;
			}
			if (bytes != bytes_written)
				break;
			total_bytes_written += bytes_written;
		} while (bytes >= len);

		set_fs(oldfs);
		filp_close(fr, NULL);
		files_successful++;
	}
printk("Copy Completed");
#ifdef EXTRA_CREDIT
/*
Atomic Mode:
	 Check	IF (file read-write operation failed or partial write)
			then  delete temp file without updating the output file
		Else
			rename the temp file to give output file name or delete
			 the already exisiting out file
*/

ATOMIC:
	if (isAtomic) {
		printk(KERN_INFO"In Atomic Code Area\n");
		if (files_successful != user_input->infile_count) {

			temp_inode = fw->f_dentry->d_parent->d_inode;
			temp_dentry = fw->f_dentry;
			filp_close(fw, NULL);
			printk(KERN_ERR"PartialWrite:"
					"not allowed in atomic behaviour\n");
			result = vfs_unlink(temp_inode, temp_dentry);
			goto CleanUp;

		} else if (isAppend) {
			fr = filp_open(user_input->outfile, O_RDWR, 0);
			temp_inode = fr->f_dentry->d_parent->d_inode;
			temp_dentry = fr->f_dentry;
			filp_close(fr, NULL);
			result = vfs_unlink(temp_inode, temp_dentry);
			if (result < 0) {
				printk(KERN_ERR "error while unlinking\n");
				goto CleanUp;
			}

			result = vfs_rename(fw->f_dentry->d_parent->d_inode,
					fw->f_dentry, temp_inode, temp_dentry);

			printk(KERN_INFO "File Renamed to %s",
				fw->f_dentry->d_name.name);
			if (result < 0) {
				printk(KERN_ERR "error while renaming\n");
				goto CleanUp;
			}
		}
	}
#endif
	switch (user_input->flags) {
	case 1:
		/*####	-N	####*/
	case 5:
		/*####	-A -N	####*/
		result =  files_successful;
		break;
	case 2:
		/*####	-P	####*/
	case 6:
		/* ####	-A -P	####*/
		result = ((int)(total_bytes_written/(int)file_size)) * 100;
		break;
	default:
		result = total_bytes_written;
		break;
	}

CleanUp:
	if (fw && !IS_ERR(fw))
		filp_close(fw, NULL);
	if (fr && !IS_ERR(fr))
		filp_close(fr, NULL);
	kfree(buf);
	if (user_input) {
		putname(user_input->outfile);
		for (i = 0; i < user_input->infile_count; i++)
			putname(user_input->infiles[i]);

		kfree(user_input->infiles);
		kfree(user_input);
	}
	printk(KERN_DEBUG "Return Value %d\n", result);
	return result;
}
/*   ##### Validates all the inputs passed to the system call #####*/

static inline int validateInputs(struct inputParameters *inp)
{
	struct file *fp_inp = NULL, *fp_out = NULL;
	int i = 0, rval = 0, file_size = 0;
	bool isExisting = true;

	if (inp->outfile == NULL) {
		rval = -EINVAL;
		printk(KERN_ERR "\n Output file is not specified.\n");
		goto val_cleanUP;
	}
	if (inp->infile_count == 0 || inp->infile_count > 10) {
		rval = -EINVAL;
		printk(KERN_ERR "The input files must range"
				"between minimum 1 and maximum 10\n");
		goto val_cleanUP;
	}

	/*Validating the special flags.
	* 0 - default flag number of bytes
	* 1 - -N umber of files
	* 2 - -P ercentage
	* 4 - -A tomic
	* 5 - -N -A
	* 6 - -P -A
	*/
	switch (inp->flags) {
	case 0:
	case 1:
	case 2:

#ifdef EXTRA_CREDIT
	case 4:
	case 5:
	case 6:
#endif
		break;
	default:
		rval =  -EINVAL;
		printk(KERN_ERR"\nInvalid Flags(-N-P)or ExtraCredit notset\n");
		goto val_cleanUP;
	}

/* Validation of whether output filename and input file are same or not*/

	if ((inp->oflags&O_APPEND || inp->oflags&O_TRUNC)
		&& !(inp->oflags&O_CREAT)) {
		fp_out = filp_open(inp->outfile, O_RDONLY, 0);
		if (!fp_out || IS_ERR(fp_out)) {
			printk(KERN_ERR "file not found err %d\n",
				(int)PTR_ERR(fp_out));
			rval = (int) PTR_ERR(fp_out);
			goto val_cleanUP;
		}
	}
	if (inp->oflags&O_APPEND || inp->oflags&O_CREAT) {
		fp_out = filp_open(inp->outfile, O_RDONLY, 0);
		if (!fp_out || IS_ERR(fp_out))
			isExisting = false;
	}
/*Validation of input files*/
	for (i = 0; i < inp->infile_count; i++) {
		fp_inp = filp_open(inp->infiles[i], O_RDONLY, 0);
		if (!fp_inp || IS_ERR(fp_inp)) {
			printk(KERN_ERR "file not found err%d\n",
				(int)PTR_ERR(fp_inp));
			rval = (int) PTR_ERR(fp_inp);
			goto val_cleanUP;
		}
		if (inp->oflags&O_CREAT && !isExisting) {
			if (strcmp(inp->outfile,
				fp_inp->f_dentry->d_name.name) == 0) {
				rval = -EEXIST;
				printk(KERN_ERR "Inputfile is Outputfile\n");
			}
		} else
		if (fp_inp->f_dentry->d_inode == fp_out->f_dentry->d_inode) {
			rval = -EEXIST;
			printk(KERN_ERR "Inputfile is Outputfile\n");
			goto val_cleanUP;
		}
		file_size += fp_inp->f_dentry->d_inode->i_size;
		filp_close(fp_inp, NULL);
	}
	rval = file_size;
val_cleanUP:
	if (fp_inp && !IS_ERR(fp_inp))
		filp_close(fp_inp, NULL);
	if (fp_out && !IS_ERR(fp_out))
		filp_close(fp_out, NULL);
	return rval;
}

static int __init init_sys_xconcat(void)
{
	printk(KERN_INFO "installed new sys_xconcat module\n");
	if (sysptr == NULL)
		sysptr = xconcat;
	return 0;
}
static void  __exit exit_sys_xconcat(void)
{
	if (sysptr != NULL)
		sysptr = NULL;
	printk(KERN_INFO "removed sys_xconcat module\n");
}
module_init(init_sys_xconcat);
module_exit(exit_sys_xconcat);
MODULE_LICENSE("GPL");
