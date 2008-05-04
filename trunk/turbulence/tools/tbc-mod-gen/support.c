/*  tbc-mod-gen: A tool to produce modules for the Turbulence BEEP server
 *  Copyright (C) 2008 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; version 2.1 of the
 *  License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *
 *  For comercial support on build BEEP enabled solutions, supporting
 *  turbulence based solutions, etc, contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Antonio Suarez Nº10, Edificio Alius A, Despacho 102
 *         Alcala de Henares, 28802 (MADRID)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/turbulence
 */
#include <support.h>


FILE         * opened_file                      = NULL;
char         * opened_file_name                 = NULL;
char         * original_user_file_to_merge      = NULL;
int            support_num_tabs           = 0;


/** 
 * @internal
 *
 * Moves a file from the detination provided to the destination provided.
 * 
 * @param from The source destination.
 * @param to The file destination.
 */
void   support_move_file (TurbulenceCtx * ctx, char * from, char * to)
{

#ifdef AXL_OS_WIN32
	/* first remove the file to be overwrited. On unix platforms
	 * this is not necesary because move (rename) already
	 * overwrite destination file. On windows platform is not
	 * posible to rename a file with a file on the destination. */
	if (vortex_support_file_test (to, FILE_IS_REGULAR) && (unlink (to) != 0)) {
		error ("unable to remove destination file: %s to be overwrited by: %s at rename operation",
		       FALSE, to, from);
		return;
	}
#endif

	/* now move the file */
	if (rename (from, to) != 0) {
		error ("unable to move file: %s to %s", from, to);
	}
	return;
}

/** 
 * @brief Allows to know if two files are equal.
 * 
 * @param file1 First file to check.
 * @param file2 Second file to check.
 * 
 * @return true if both files are equal (false if not).
 */
bool support_are_equal (char * file1 , char * file2)
{
	int fd1;
	int fd2;
	char buffer1[1];
	char buffer2[1];
	int  read1;
	int  read2;

	/* check that both files exists to ensure that the function
	 * returns true for files that exists. */
	if (! vortex_support_file_test (file1, FILE_EXISTS))
		return false;

	if (! vortex_support_file_test (file2, FILE_EXISTS))
		return false;

	/* open both files */
	fd1 = open (file1, O_RDONLY);
	if (fd1 < 0) {
		return false;
	}

	fd2 = open (file2, O_RDONLY);
	if (fd2 < 0) {
		close (fd1);
		return false;
	}
	
	read1 = read (fd1, buffer1, 1);
	read2 = read (fd2, buffer2, 1);

	/* while both contents are equal */
	while ((read1 == read2) && buffer1[0] == buffer2[0] && read1 != 0 ) {
		
		/* read the next */
		read1 = read (fd1, buffer1, 1);
		read2 = read (fd2, buffer2, 1);
		
	} /* end while */

	close (fd1);
	close (fd2);

	
	
	return (read1 == read2) && (read1 == 0);
}

/** 
 * @brief Allows to open a file, that is located at the path provided
 * for the printf-like format.
 * 
 * @param format A path that identifies the file to be opened.
 */
void    support_open_file       (TurbulenceCtx * ctx, const char * format, ...)
{
	
	va_list   args;
	va_start (args, format);
	original_user_file_to_merge = axl_strdup_printfv (format, args);
	va_end (args);

	/* because we are using a backslash representation to open
	 * files we need to do some foo to be run-time portable with
	 * windows platforms.  original_user_file_to_merge =
	 * af_gen_support_get_native_file_name
	 * (original_user_file_to_merge); */
	opened_file_name = axl_strdup_printf ("%s.tbc-mod-gen-version", original_user_file_to_merge);

	if (opened_file) {
		fclose (opened_file);
		opened_file = NULL;
	}

	opened_file = fopen (opened_file_name, "w");
	if (!opened_file) {
		error ("unable to create %s file\n", opened_file_name);
		exit (-1);
	}

	/* reset automatic tab indentation */
	support_num_tabs = 0;

	return;
}

char * next_line (FILE * file)
{
	long   current;
	int    desp = 0;
	char   value;
	char * result;
	char   temp[1024];

	/* check end of file */
	if (feof (file) != 0) {
		printf ("end of file reached..\n");
		return NULL;
	}

	/* check file error */
	if (ferror (file) != 0) {
		printf ("error found....\n");
		return NULL;
	}

	/* get current file position */
	if (file != stdin) {
		current = ftell (file);
		
		/* get the desp */
		while (fread (&value, 1, 1, file) == 1 && value != '\n')
			desp++;
		
		if (ferror (file) != 0) {
			printf ("error found..\n");
			return NULL;
		}

		/* reconfigure descriptor */
		if (fseek (file, current, SEEK_SET) != 0) {
#if defined(AXL_OS_UNIX)
			printf ("failed to reconfigure current file stream: %ld, error=%s..\n", current, strerror (errno));
#elif defined(AXL_OS_WIN32)
			printf ("failed to reconfigure current file stream: %ld\n", current);
#endif
			return NULL;
		}
		
		/* allocate enough memory */
		result = axl_new (char, desp + 1);
		
		/* get the desp */
		if (fread (result, 1, desp, file) != desp) {
			printf ("failed to read data..\n");
			axl_free (result);
			return NULL;
		} /* end if */


		if (feof (file) == 0) {
			/* consume the latest \n */
			fread (&value, 1, 1, file);
		} /* end if */
	} else {
		memset (temp, 0, 1024);
		
		/* get the desp */
		while (desp < 1024 && fread (temp + desp, 1, 1, file) == 1 && temp[desp] != '\n')
			desp++;

		/* allocate enough memory */
		result = axl_new (char, desp + 1);
		
		/* copy content */
		memcpy (result, temp, desp);
		
	} /* end if */
	
	return result;
}



/** 
 * @brief Close the current opened file.
 */
void    support_close_file      (TurbulenceCtx * ctx)
{
	char    * path  = original_user_file_to_merge;
	char    * reply = NULL;

	/* close the file opened */
	fclose (opened_file);
	opened_file = NULL;

	/* check force option */
	if (exarg_is_defined ("force"))
		goto write_file;

	if (vortex_support_file_test (path, FILE_EXISTS)) {
		/* check if both files differs */
		if (support_are_equal (original_user_file_to_merge, opened_file_name)) {
			msg ("skiping file not modified: %s", original_user_file_to_merge);
			unlink (opened_file_name);
			goto finish_close_file;
		}

		/* ask the user to overwrite this file */
	get_option:
		wrn    ("file already exists, and differs: %s", path);
		wrn_sl ("Do you want me to: Write (w), Skip (s): ");
		reply = next_line (stdin);
		if (axl_cmp (reply, "w")) {
			goto write_file;
		} else if (axl_cmp (reply, "s")) {
			msg ("skiping creating: %s", path);
		} else {
			/* option not supported */
			error ("option not supported, try again");
			goto get_option;
		} /* end if */
		
		/* flag ok result */
		axl_free (reply);
	} else {
		/* write the file case */
	write_file:
		msg ("creating file:             %s", path);

		/* move the file to the final destination */
		support_move_file (ctx, opened_file_name, original_user_file_to_merge);
	} /* end if */
	
 finish_close_file:

	/* free original file to merge: currently not supported */
	axl_free (original_user_file_to_merge);
	original_user_file_to_merge = NULL;

	/* free file name value */
	axl_free (opened_file_name);
	opened_file_name = NULL;
	
	return;
}	

/** 
 * @brief Writes the provided content, using a printf-like format, to
 * the current opened file.
 * 
 * @param format The format to write (printf format).
 */
void support_write (const char * format, ...) {
	
	va_list     args;
	int       i;

	/* write tabular according to current indent configuration */
	if (support_num_tabs > 0) {
		for (i = 0; i < support_num_tabs; i++) {
			fprintf (opened_file, "\t");
		}
	}

	/* opend the std arg content to write it to the file */
	va_start (args, format);
	vfprintf (opened_file, format, args);
	va_end (args);
	
	/* done */
	return;
}

/** 
 * @brief Allows to write content to the opened file as defined for
 * \ref support_write but without placing tabular content.
 * 
 * @param format A printf-like format especifying the content to write
 * to the file.
 */
void    support_sl_write        (const char * format, ...)
{
	va_list   args;

	/* write content defined by the parameters */
	va_start (args, format);
	vfprintf (opened_file, format, args);
	va_end (args);

	/* return */
	return;

}

/** 
 * @brief Push current indent increasing the tab size.
 *
 * This is used to modify default behaviour provided by write
 * functions:
 * 
 *  - \ref support_write
 *  - \ref support_sl_write
 */
void    support_push_indent     ()
{
	support_num_tabs++;
}

/** 
 * @brief Pop current indent decreasing the tab size.
 *
 * This is used to modify default behaviour provided by write
 * functions:
 * 
 *  - \ref support_write
 *  - \ref support_sl_write
 */
void    support_pop_indent      ()
{
	support_num_tabs--;
}

/** 
 * @brief Dumps the axl document received to the file located at the
 * provided path.
 * 
 * @param doc The document to dump.
 *
 * @param tabular The number of espaces to use while tabbing the
 * content.
 *
 * @param format_path The path to be used to dump the content. This
 * value is a printf-like format.
 *
 * 
 * @return true if the document was dumped, false if not.
 */
bool support_dump_file (TurbulenceCtx * ctx, axlDoc * doc, int tabular, const char * format_path, ...)
{
	va_list   args;
	bool      result = false;
	char    * path;
	char    * reply = NULL;
	axlDoc  * doc2;
	
	/* open varidic arguments */
	va_start (args, format_path);
	
	/* create the path */
	path = axl_strdup_printfv (format_path, args);

	/* close varidic argument */
	va_end (args);

	if (vortex_support_file_test (path, FILE_EXISTS)) {
		
		/* load the document and check if it is equal */
		doc2   = axl_doc_parse_from_file (path, NULL);
		result = (doc2 != NULL && axl_doc_are_equal_trimmed (doc, doc2));
		axl_doc_free (doc2);
		if (result) {
			msg ("skiping file not modified: %s", path);
			goto finish_write;
		}


		/* ask the user to overwrite this file */
	get_option:
		wrn   ("file already exists, and differs: %s", path);
		wrn_sl ("Do you want me to: Write (w), Skip (s): ");
		reply = next_line (stdin);
		if (axl_cmp (reply, "w")) {
			goto write_file;
		} else if (axl_cmp (reply, "s")) {
			msg ("skiping creating: %s", path);
		} else {
			/* option not supported */
			error ("option not supported, try again");
			goto get_option;
		} /* end if */
		
		/* flag ok result */
		result = true;
		axl_free (reply);
	} else {
		/* write the file case */
	write_file:
		msg ("creating file:             %s", path);

		/* dump the file */
		result = axl_doc_dump_pretty_to_file (doc, path, tabular);
	}
	
 finish_write:

	/* free path */
	axl_free (path);

	/* return result */
	return result;
}

/** 
 * @brief Produce a clean representation for the string received.
 * 
 * @param name The name to be represented in clean form.
 * 
 * @return A newly allocated norma value from the name received.
 */
char  * support_clean_name          (const char * name)
{
	char     * result   = axl_strdup (name);
	int        iterator = 0;
	int        index    = 0;
	
	/* translate all upper values to lower ones, removing all
	 * characters that are letters or digits. */
	while (name[iterator]) {
		/* copy as is character '-' */
		if (name[iterator] == '-') {
			result[index] = '_';
			index++;
		}

		if (isalnum (name[iterator])) {
			result[index] = tolower (name[iterator]);
			index++;
		}
		iterator++;
	}

	/* close the value */
	result[index] = 0;

	return result;
}

/** 
 * @internal
 * 
 * Internal implementation that support support_to_lower and
 * support_to_upper.
 */
char * __support_common_name (const char * name, bool to_upper)
{
	char * result;
	int    iterator;

	axl_return_val_if_fail (name, NULL);
	
	/* get a lower copy */
	if (to_upper)
		result   = axl_stream_to_upper_copy (name);
	else
		result   = axl_stream_to_lower_copy (name);

	iterator = 0;

	/* check every character */
	while (result [iterator] != 0) {
		/* change the value */
		if (! isalnum (result [iterator]))
			result [iterator] = '_';

		/* update iterator */
		iterator++;
	}
	
	/* return result */
	return result;
}

/** 
 * @brief Allows to get the lower representation, not only including
 * alphabetic values but other values (like '-', which are translated
 * into _).
 * 
 * @param name The name to get its lower version.
 * 
 * @return A newly allocated string or NULL if it fails.
 */
char  * support_to_lower            (const char * name)
{
	/* makes a to lower operation */
	return __support_common_name (name, false);
}

/** 
 * @brief Allows to get the upper representation, not only including
 * alphabetic values but other values (like '-', which are translated
 * into _).
 * 
 * @param name The name to get its lower version.
 * 
 * @return A newly allocated string or NULL if it fails.
 */
char  * support_to_upper            (const char * name)
{
	/* makes a to upper operation */
	return __support_common_name (name, true);
}

/** 
 * @internal
 * 
 * Makes the provided file, located at the path formed by the
 * printf-like function, to be executable.
 * 
 * @param format The file to be executable, especified by the a
 * printf-like path.
 */
void    support_make_executable (TurbulenceCtx * ctx, const char * format, ...)
{
	char * result;
	
	va_list   args;

	/* write content defined by the parameters */
	va_start (args, format);
	result = axl_strdup_printfv (format, args);
	va_end (args);
	
	msg ("making executable:         %s", result);

	/* make a chmod operation */
	if (chmod (result, 0770) < 0) {
		error ("unable to make executable the file: '%s'\n", result);
	}
	
	/* release the memory hold */
	axl_free (result);

	/* return */
	return;
	
}
