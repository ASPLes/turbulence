/*  Turbulence BEEP application server
 *  Copyright (C) 2010 Advanced Software Production Line, S.L.
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
 *  at COPYING file. This is LGPL software: you are welcome to develop
 *  proprietary applications using this library without any royalty or
 *  fee but returning back any change, improvement or addition in the
 *  form of source code, project image, documentation patches, etc.
 *
 *  For commercial support on build BEEP enabled solutions, supporting
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
#ifndef __TURBULENCE_CTX_PRIVATE_H__
#define __TURBULENCE_CTX_PRIVATE_H__


struct _TurbulenceCtx {
	/* Reference to the turbulence vortex context associated.
	 */
	VortexCtx          * vortex_ctx;

	/* default signal handlers */
	TurbulenceSignalHandler signal_handler;

	/* Controls if messages must be send to the console log.
	 */
	int                  console_enabled;
	int                  console_debug;
	int                  console_debug2;
	int                  console_debug3;
	int                  console_color_debug;

	/* wait queue: this queue is used by turbulence_ctx_wait to
	 * implement wait blocking wait without allocatting every time
	 * a custom wait is required */
	VortexAsyncQueue   * wait_queue;

	/* Turbulence current pid (process identifier) */
	int                  pid;
	axl_bool             is_main_process;
	TurbulencePPathDef * child_ppath;
	
	/* some variables used to terminate turbulence. */
	axl_bool             is_exiting;
	VortexMutex          exit_mutex;
	
	/* Mutex to protect the list of db list opened. */
	VortexMutex          db_list_mutex;
	
	/* List of already opened db list, used to implement automatic
	 * features such automatic closing on turbulence exit,
	 * automatic reloading.. */
	axlList            * db_list_opened;
	axlDtd             * db_list_dtd;

	/*** turbulence ppath module ***/
	int                  ppath_next_id;
	TurbulencePPath    * paths;
	axl_bool             all_rules_address_based;
	/* profile_attr_alias: this hash allows to establish a set of
	 * alias that is used by profile path module to check for a
	 * particular attribute found on the connection instead of the
	 * particular profile requested. This is useful for TLS case
	 * (and other tuning profiles) that reset the connection,
	 * causing the channel that represents it to disapear (RFC
	 * 3080, page 30, all channels closed) making the profile path
	 * configuration to not detect the channel after tuning have
	 * been completed. */
	axlHash            * profile_attr_alias;

	/*** turbulence log module ***/
	int                  general_log;
	int                  error_log;
	int                  vortex_log;
	int                  access_log;
	TurbulenceLoop     * log_manager;

	/*** turbulence config module ***/
	axlDoc             * config;

	/* turbulence loading modules module */
	axlList            * registered_modules;
	VortexMutex          registered_modules_mutex;

	/* turbulence connection manager module */
	VortexMutex          conn_mgr_mutex;
	axlHash            * conn_mgr_hash; 

	/* turbulence stored data */
	axlHash            * data;
	VortexMutex          data_mutex;

	/* turbulence run  module */
	axl_bool             clean_start;
	axl_bool             started;

	/* DTd used by the turbulence-run module to validate module
	 * pointers */
	axlDtd             * module_dtd;

	/*** turbulence process module ***/
	axlHash                 * child_process;
	VortexMutex               child_process_mutex;
	VortexAsyncQueue        * child_wait;

	/*** turbulence mediator module ***/
	axlHash            * mediator_hash;
	VortexMutex          mediator_hash_mutex;

};

/** 
 * @brief Private definition that represents a child process created.
 */
struct _TurbulenceChild {
	int                  pid;
	TurbulenceCtx      * ctx;
	TurbulencePPathDef * ppath;
#if defined(AXL_OS_UNIX)
	int                  child_connection;
	char               * socket_control_path;
#endif
};

typedef enum {
	PROFILE_ALLOW, 
	PROFILE_IF
} TurbulencePPathItemType;

struct _TurbulencePPathItem {
	/* The type of the profile item path  */
	TurbulencePPathItemType type;
	
	/* support for the profile to be matched by this profile item
	 * path */
	TurbulenceExpr * profile;

	/* optional expression to match a mark that must have a
	 * connection holding the profile */
	char * connmark;
	
	/* optional configuration that allows to configure the number
	   number of channels opened with a particular profile */
	int    max_per_con;

	/* optional expression to match a pre-mark that must have the
	 * connection before accepting the profile. */
	char * preconnmark;

	/* Another list for all profile path item found inside this
	 * profile path item. This is only used by PROFILE_IF items */
	TurbulencePPathItem ** ppath_items;
	
};

struct _TurbulencePPathDef {
	int    id;

	/* the name of the profile path group (optional value) */
	char                 * path_name;

	/* the server name pattern to be used to match the profile
	 * path. If turbulence wasn't built with pcre support, it will
	 * compiled as an string. */
	TurbulenceExpr       * serverName;

	/* source filter pattern. Again, if the library doesn't
	 * support regular expression, the source is taken as an
	 * string */
	TurbulenceExpr       * src;

	/* destination filter pattern. Again, if the library doesn't
	 * support regular expression, the source is taken as an
	 * string */
	TurbulenceExpr       * dst;

	/* a reference to the list of profile path supported (first
	 * xml level of <allow> and <if-sucess> nodes). */
	TurbulencePPathItem ** ppath_items;

#if defined(AXL_OS_UNIX)
	/* user id to that must be used to run the process */
	int  user_id;

	/* group id that must be used to run the process */
	int  group_id;
#endif

	/* allows to control if turbulence must run the connection
	 * received in the context of a profile path in a separate
	 * process */
	axl_bool separate;

	/* allows to control if childs created for each profile path
	   are reused. */
	axl_bool reuse;

	/* allows to change working directory to the provided value */
	const char * chroot;	

	/* allows to configure a working directory associated to the
	 * profile path. */
	const char * work_dir;

};

/** 
 * @internal Connection management done by conn-mgr module.
 */
typedef struct _TurbulenceConnMgrState {
	VortexConnection * conn;
	TurbulenceCtx    * ctx;

	/* a hash that contains the set of profiles running on this
	 * connection and how many times */
	axlHash          * profiles_running;

	/* reference to handler ids to be removed */
	axlPointer         added_channel_id;
	axlPointer         removed_channel_id;
} TurbulenceConnMgrState;

#endif
