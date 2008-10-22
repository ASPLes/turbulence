/*  Turbulence:  BEEP application server
 *  Copyright (C) 2008 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; version 2.1 of the
 *  License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are wellcome to
 *  develop propietary applications using this library withtout any
 *  royalty or fee but returning back any change, improvement or
 *  addition in the form of source code, project image, documentation
 *  patches, etc.
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
#include <turbulence-expr.h>

#include <pcre.h>

/** 
 * \defgroup turbulence_expr Turbulence expr: regular expression support.
 */

/** 
 * \addtogroup turbulence_expr
 * @{
 */

struct _TurbulenceExpr {
	pcre * expr;
	int    negative;
};

/** 
 * @internal Function used to check if the string provided have
 * content that must be revised to help its clarity.
 */
int       turbulence_expr_has_escapable_chars        (const char * expression,
						      int          expression_size,
						      int        * added_size)
{
	int      iterator = 0;
	int      result   = false;

	/* reset additional size value */
	*added_size = 0;

	/* calculate the content size */
	expression_size = strlen (expression);

	/* basic case where the string received is exactily '*' that
	 * should be expanded into '.*' */
	if (expression_size == 1 && expression [0] == '*') {
		*added_size = 1;
		return true;
	} /* end if */

	/* iterate over all content defined */
	while (iterator < expression_size) {

		/* check for .* */
		if (expression [iterator] == '*' && expression [iterator - 1] != '.' ) {
			result = true;
			(*added_size) += 1;
		}

		/* check for \/ */
		if (expression [iterator] == '/' && expression [iterator - 1] != '\\' ) {
			result = true;
			(*added_size) += 1;
		}

		/* update the iterator */
		iterator++;
	} /* end if */

	/* return results */
	return result;
} /* end __turbulence_ppath_has_escapable_chars */

/** 
 * @internal Support function which used information provided by
 * previous function to expand the string providing a perl regular
 * expression compatible if used / and *.
 */
char * turbulence_expr_copy_and_escape (const char * expression, 
					int          expression_size, 
					int          additional_size)
{
	int    iterator  = 0;
	int    iterator2 = 0;
	char * result;
	axl_return_val_if_fail (expression, false);

	/* allocate the memory to be returned */
	result = axl_new (char, expression_size + additional_size + 1);
	
	/* check for basic case '*' */
	if (axl_cmp (expression, "*")) {
		memcpy (result, ".*", 2);
		return result;
	}

	/* iterate over all expression defined */
	while (iterator2 < expression_size) {
		if (iterator2 > 0) {
			/* check for .* */
			if (expression [iterator2] == '*' && expression [iterator2 - 1] != '.') {
				memcpy (result + iterator, ".*", 2);
				iterator += 2;
				iterator2++;
				continue;
			}
			
			/* check for .* */
			if (expression [iterator2] == '/' && expression [iterator2 - 1] != '\\') {
				memcpy (result + iterator, "\\/", 2);
				iterator += 2;
				iterator2++;
				continue;
			}
		} /* end if */

		/* copy value received because it is not an escape
		 * sequence */
		memcpy (result + iterator, expression + iterator2, 1);

		/* update the iterator */
		iterator++;
		iterator2++;
	} /* end while */

	/* return results */
	return result;
} /* end __turbulence_ppath_copy_and_escape */

/* check if the expression is negative */
const char * turbulence_expr_check_negative_expr (const char * expression, int  * negative)
{
	int iterator;
	int length;

	/* do not oper if null is received */
	if (expression == NULL)
		return expression;

	/* check if the negative expression is found */
	iterator = 0;
	length   = strlen (expression);

	/* skip white spaces */
	while (iterator < length) {
		if (expression [iterator] == ' ')
			iterator++;
		else
			break;
	} /* end while */

	/* check negative expression */
	if (expression [iterator] == '!' || 
	    (((iterator + 2) < length) && expression[iterator] == 'n' && expression[iterator + 1] == 'o' && expression[iterator + 2] == 't')) {
		/* negative expression found, return the updated
		 * reference */
		*negative = true;
		if (expression[iterator] == 'n')
			return (expression  + iterator + 4);
		else
			return (expression  + iterator + 2);
	} /* end if */

	/* return the expression as is */
	return expression;
}


/** 
 * @brief Compiles the expression contained inside (expression)
 * returning a reference to a regular expression that can be used to
 * match strings.
 *
 * @param ctx The context where the match operation will take place.
 *
 * @param expression The regular expression definition.
 *
 * @param error_msg This is an optional error message that can be used
 * by the function to perform a better error reporting. For example,
 * in the case you are compiling an expression for some particular
 * item, you can provide an string such "Failed to compile expression
 * to be used for...". This will help administrators to configure
 * turbulence.
 *
 * @return A newly created reference to the expression compiled or
 * NULL if it fails. The expression returned must be terminated to
 * return resouces used by using \ref turbulence_expr_free.
 */
TurbulenceExpr * turbulence_expr_compile (TurbulenceCtx * ctx, 
					  const char    * expression, 
					  const char    * error_msg)
{
	TurbulenceExpr * expr;
	const char     * error;
	int              erroroffset;
	int              dealloc = false;
	int              additional_size;

	/* create the turbulence expression node */
	expr = axl_new (TurbulenceExpr, 1);

	/* check if the expression is negative */
	msg ("checking negative expression: %s", expression);
	expression = turbulence_expr_check_negative_expr (expression, &expr->negative);
	if (expr->negative) {
		msg ("  found, updated expression to: not %s", expression);
	}
	
	/* do some regular expression support to avoid making it
	 * painful */
	if (turbulence_expr_has_escapable_chars (expression, strlen (expression), &additional_size)) {
		/* expand all * and / values which are not preceded as
		 * .* or \/ */
		msg ("NOTE: expanding expression: '%s'..", expression);
		expression = turbulence_expr_copy_and_escape (expression, strlen (expression), additional_size);
		msg ("      to: '%s'..", expression);
		dealloc    = true;
	} /* end if */
	
	/* compile expression */
	expr->expr = pcre_compile (
		/* the pattern to compile */
		expression, 
		/* no flags */
		0,
		/* provide a reference to the error reporting */
		&error, &erroroffset, NULL);

	if (expr->expr == NULL) {
		/* failed to parse server name matching */
		error ("%s: %s, error: %s, at: %d",
		       error_msg, expression, error, erroroffset);

		/* free expr */
		axl_free (expr);

		/* check and dealloc */
		if (dealloc)
			axl_free ((char*) expression);
		return NULL;
	} /* end if */

	/* check and dealloc */
	if (dealloc)
		axl_free ((char *) expression);

	/* return expression */
	return expr;

} /* end turbulence_expr_compile */

/** 
 * @brief Function that allows perform matching against the
 * expression, using the subject received. 
 *
 * @param expr The expression to use to match the string provided
 * (subject). This expression was created with \ref
 * turbulence_expr_compile.
 *
 * @param subject The string that is going to be matched.
 *
 * @return The function return true in the case the expression (expr)
 * match the string provided (subject).
 */
int  turbulence_expr_match (TurbulenceExpr * expr, const char * subject)
{
	/* return false if either values received are null */
	if (subject == NULL || expr == NULL)
		return false;

	/* check against the pcre expression */
	if (expr->negative) {
		return ! (pcre_exec (expr->expr, NULL, subject, strlen (subject), 0, 0, NULL, 0) >= 0);
	}		

	/* non negative expression. */
	return pcre_exec (expr->expr, NULL, subject, strlen (subject), 0, 0, NULL, 0) >= 0;
}

/** 
 * @brief Terminate the regular expression compiled by \ref
 * turbulence_expr_compile.
 */
void turbulence_expr_free (TurbulenceExpr * expr)
{
	if (expr == NULL)
		return;

	/* free the expression and then the node itself */
	pcre_free (expr->expr);
	axl_free (expr);
	
	return;
}

/** 
 * @}
 */
