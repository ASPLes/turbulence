/*  Turbulence:  BEEP application server
 *  Copyright (C) 2007 Advanced Software Production Line, S.L.
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
 *         C/ Dr. Michavila Nº 14
 *         Coslada 28820 Madrid
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://fact.aspl.es
 */
#include <turbulence-ppath.h>
#include <pcre.h>

typedef enum {
	PROFILE_ALLOW, 
	PROFILE_IF
} TurbulencePPathItemType;

typedef struct _TurbulencePPathItem {
	/* The type of the profile item path  */
	TurbulencePPathItemType type;
	
	/* support for the profile to be matched by this profile item
	 * path */
#if defined(ENABLE_PCRE_SUPPORT)
	pcre * profile;
#else   
	char * profile;
#endif
	
} TurbulencePPathItem;

typedef struct _TurbulencePPathDef {
	/* the name of the profile path group (optional value) */
	char * path_name;

	/* the server name pattern to be used to match the profile
	 * path. If turbulence wasn't built with pcre support, it will
	 * compiled as an string. */
#if defined(ENABLE_PCRE_SUPPORT)
	pcre * serverName;
#else   
	char * serverName;
#endif

	/* source filter pattern. Again, if the library doesn't
	 * support regular expression, the source is taken as an
	 * string */
#if defined(ENABLE_PCRE_SUPPORT)
	pcre * src;
#else   
	char * src;
#endif
	/* a reference to the list of profile path supported */
	TurbulencePPathItem * ppath_item;
	
} TurbulencePPathDef;

typedef struct _TurbulencePPath {
	/* list of profile paths found */
	TurbulencePPathDef * items;
	
} TurbulencePPath;

/** 
 * @internal Prepares the runtime execution to provide profile path
 * support according to the current configuration.
 * 
 */
void turbulence_ppath_init ()
{
	
}

/** 
 * @internal Terminates the profile path module, cleanup all memory
 * used.
 */
void turbulence_ppath_cleanup ()
{
	
}

