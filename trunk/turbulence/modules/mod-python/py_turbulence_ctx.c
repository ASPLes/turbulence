/** 
 *  PyTurbulence: Python bindings for Turbulence API 
 *  Copyright (C) 2009 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
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
 *  For commercial support on build BEEP enabled solutions contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Antonio Suarez Nº 10, 
 *         Edificio Alius A, Despacho 102
 *         Alcalá de Henares 28802 (Madrid)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/vortex
 */

/* include header */
#include <py_turbulence.h>

struct _PyTurbulenceCtx {
	/* header required to initialize python required bits for
	   every python object */
	PyObject_HEAD

	/* pointer to the turbulence context */
	TurbulenceCtx * ctx;

	/* pointer to the PyVortexCtx */
	PyObject      * py_vortex_ctx;
};

static int py_turbulence_ctx_init_type (PyTurbulenceCtx *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

/** 
 * @brief Function used to allocate memory required by the object turbulence.ctx
 */
static PyObject * py_turbulence_ctx_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyTurbulenceCtx *self;

	/* create the object */
	self = (PyTurbulenceCtx *)type->tp_alloc(type, 0);

	/* do not create the context here, rather allow the caller to
	 * set it from outside this context */

	return (PyObject *)self;
}

/** 
 * @brief Function used to finish and dealloc memory used by the object turbulence.ctx
 */
static void py_turbulence_ctx_dealloc (PyTurbulenceCtx* self)
{
	/* decrease vortex.Ctx reference */
	py_vortex_log (PY_VORTEX_DEBUG, "finishing py_turbulence_ctx ref (%p), with py_vortex_ctx ref (%p)",
		       self, self->py_vortex_ctx);
	Py_XDECREF (self->py_vortex_ctx);
	self->py_vortex_ctx = NULL;

	/* free the node it self */
	self->ob_type->tp_free ((PyObject*)self);

	return;
}

/** 
 * @brief Function used to nullify and release PyVortexCtx object
 * associated to the PyTurbulenceCtx to avoid memory leaks on
 * turbulence finalization.
 */
void       py_turbulence_ctx_nullify_vortex_ctx (PyTurbulenceCtx * self)
{
	if (self->py_vortex_ctx) {
		py_vortex_log (PY_VORTEX_DEBUG, "release and nullify py_vortex_ctx ref (%p) inside py_tbc_ctx (%p)",
			       self->py_vortex_ctx, self);
		Py_XDECREF (self->py_vortex_ctx);
		self->py_vortex_ctx = NULL;
	}

	return;
}

/** 
 * @brief This function implements the generic attribute getting that
 * allows to perform complex member resolution (not merely direct
 * member access).
 */
PyObject * py_turbulence_ctx_get_attr (PyObject *o, PyObject *attr_name) {
	const char        * attr = NULL;
	PyObject          * result;
	PyTurbulenceCtx   * self = (PyTurbulenceCtx *) o; 

	/* now implement other attributes */
	if (! PyArg_Parse (attr_name, "s", &attr))
		return NULL;

	py_vortex_log (PY_VORTEX_DEBUG, "received request to report channel attr name %s (self: %p)",
		       attr, o);

	if (axl_cmp (attr, "vortex_ctx")) {
		/* returns to return PyVortexCtx associated */
		Py_INCREF (self->py_vortex_ctx);
		return self->py_vortex_ctx;
	} /* end if */

	/* first implement generic attr already defined */
	result = PyObject_GenericGetAttr (o, attr_name);
	if (result)
		return result;
	
	return NULL;
}

void py_turbulence_ctx_sanitize (char * message)
{
	int        iterator = 0;

	/* replace all % by # to avoid printf functions to interpret
	   arguments that are not available */
	while (message[iterator]) {
		/* check for scapable characters */
		if (message[iterator] == '%') 
			message[iterator] = '#';

		/* check next position */
		iterator++;
	} /* end while */

	return;
}

/** 
 * @brief Implements attribute set operation.
 */
int py_turbulence_ctx_set_attr (PyObject *o, PyObject *attr_name, PyObject *v)
{
	const char      * attr = NULL;
/*	PyTurbulenceCtx * self = (PyTurbulenceCtx *) o; */
/*	axl_bool          boolean_value = axl_false; */

	/* now implement other attributes */
	if (! PyArg_Parse (attr_name, "s", &attr))
		return -1;

	/* now implement generic setter */
	return PyObject_GenericSetAttr (o, attr_name, v);
}

static PyObject * py_turbulence_ctx_msg (PyTurbulenceCtx * self, PyObject * args)
{
	char          * message = NULL;
#if ! defined(SHOW_FORMAT_BUGS)
	TurbulenceCtx * ctx = self->ctx;
#endif

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "z", &message))
		return NULL;

	if (message) {
		/* prepare message */
		py_turbulence_ctx_sanitize (message);

		/* drop the log */
		msg (message);
	} /* end if */
	
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject * py_turbulence_ctx_msg2 (PyTurbulenceCtx * self, PyObject * args)
{
	char          * message = NULL;
#if ! defined(SHOW_FORMAT_BUGS)
	TurbulenceCtx * ctx = self->ctx;
#endif

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "z", &message))
		return NULL;

	if (message) {
		/* prepare message */
		py_turbulence_ctx_sanitize (message);

		/* drop the log */
		msg2 (message);
	} /* end if */
	
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject * py_turbulence_ctx_wrn (PyTurbulenceCtx * self, PyObject * args)
{
	char          * message = NULL;
#if ! defined(SHOW_FORMAT_BUGS)
	TurbulenceCtx * ctx = self->ctx;
#endif

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "z", &message))
		return NULL;

	if (message) {
		/* prepare message */
		py_turbulence_ctx_sanitize (message);

		/* drop the log */
		wrn (message);
	} /* end if */
	
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject * py_turbulence_ctx_error (PyTurbulenceCtx * self, PyObject * args)
{
	char          * message = NULL;
#if ! defined(SHOW_FORMAT_BUGS)
	TurbulenceCtx * ctx = self->ctx;
#endif

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "z", &message))
		return NULL;

	if (message) {
		/* prepare message */
		py_turbulence_ctx_sanitize (message);

		/* drop the log */
		error (message);
	} /* end if */
	
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject * py_turbulence_ctx_find_conn_by_id (PyTurbulenceCtx * self, PyObject * args)
{
	int                conn_id = -1;
	TurbulenceCtx    * ctx = self->ctx;
	VortexConnection * conn;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "i", &conn_id))
		return NULL;

	/* get the connection */
	conn = turbulence_conn_mgr_find_by_id (ctx, conn_id);
	if (conn == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	/* build connection reference */
	return py_vortex_connection_find_reference (conn);
}

/** 
 * @internal Function used to bridge filter notification for each
 * connection to report back filtering.
 */
static axl_bool  py_turbulence_ctx_broadcast_msg_bridge (VortexConnection * conn, axlPointer user_data)
{
	/* PyGILState_STATE           state; */
	PyObject                 * py_conn = NULL;
	PyTurbulenceInvokeData   * invoke_data = user_data;
	PyObject                 * args;
	PyObject                 * result;
	int                        result_value;
	TurbulenceCtx            * ctx = NULL;
	PyTurbulenceCtx          * py_ctx = (PyTurbulenceCtx *)invoke_data->py_ctx;

	/* get ctx reference */
	if (invoke_data && invoke_data->py_ctx) 
		ctx = py_ctx->ctx;

	/* get connection reference */
	py_conn = py_vortex_connection_find_reference (conn);
	if (py_conn == NULL) {
		error ("Failed to find PyConn connection reference, null returned after providing conn=%p and py_ctx=%p", conn, invoke_data->py_ctx);
		return axl_true; /* filter on failure */
	} /* end if */

	/* acquire the GIL */
	/* state = PyGILState_Ensure(); */

	/* create a tuple to contain arguments */
	args = PyTuple_New (2);
	PyTuple_SetItem (args, 0, py_conn);
	Py_INCREF (invoke_data->data);
	PyTuple_SetItem (args, 1, invoke_data->data);

	/* now invoke */
	result = PyObject_Call (invoke_data->handler, args, NULL);

	/* handle exceptions */
	py_vortex_log (PY_VORTEX_DEBUG, "broadcast_msg_bridge notification finished, checking for exceptions..");
	py_vortex_handle_and_clear_exception (py_conn);

	/* now implement other attributes */
	result_value = 0;
	if (! PyArg_Parse (result, "i", &result_value)) {
		error ("Failed to parse result, expected to find integer or boolean value");
	} /* end if */

	Py_XDECREF (result);
	Py_DECREF (args);

	/* release the GIL */
	/* PyGILState_Release (state); */

	/* filter according state */
	return result_value;
}

static PyObject * py_turbulence_ctx_broadcast_msg (PyTurbulenceCtx * self, PyObject * args)
{
	TurbulenceCtx          * ctx         = self->ctx;
	const char             * message     = NULL;
	int                      size        = 0;
	const char             * profile     = NULL;
	PyObject               * handler     = NULL;
	PyObject               * data        = NULL;
	PyTurbulenceInvokeData * invoke_data = NULL;
	axl_bool                 result;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "zis|OO", &message, &size, &profile, &handler, &data))
		return NULL;

	/* update size if -1 is provided */
	if (size == -1)
		size = strlen (message);

	/* check to create invoke data */
	if (handler) {
		/* create invoke data */
		invoke_data          = axl_new (PyTurbulenceInvokeData, 1);
		invoke_data->handler = handler;
		invoke_data->py_ctx  = __PY_OBJECT (self);
		/* set handler data */
		invoke_data->data    = data;
		if (invoke_data->data == NULL)
			invoke_data->data = Py_None;
	} /* end if */

	/* get the connection */
	result = turbulence_conn_mgr_broadcast_msg (ctx, message, size, profile, 
						    /* invoke bridge */
						    invoke_data ? py_turbulence_ctx_broadcast_msg_bridge : NULL, 
						    /* invoke data */
						    invoke_data);

	/* release invoke data if any */
	axl_free (invoke_data);

	if (! result) {
		Py_INCREF (Py_False);
		return Py_False;
	}

	/* return proper result */
	Py_INCREF (Py_True);
	return Py_True;
}

static PyMethodDef py_turbulence_ctx_methods[] = { 
	/*** base module ***/
	/* msg */
	{"msg", (PyCFunction) py_turbulence_ctx_msg, METH_VARARGS,
	 "Records a turbulence log message (msg ()). This is sent to the configured log and showed on the console according to the configuration."},
	{"msg2", (PyCFunction) py_turbulence_ctx_msg2, METH_VARARGS,
	 "Records a turbulence log message (msg2 ()). This is sent to the configured log and showed on the console according to the configuration."},
	/* wrn */
	{"wrn", (PyCFunction) py_turbulence_ctx_wrn, METH_VARARGS,
	 "Records a turbulence log message (wrn ()). This is sent to the configured log and showed on the console according to the configuration."},
	/* error */
	{"error", (PyCFunction) py_turbulence_ctx_error, METH_VARARGS,
	 "Records a turbulence error message (error ()). This is sent to the configured log and showed on the console according to the configuration."},
	/*** conn-mgr module ***/
	/* find_conn_by_id */
	{"find_conn_by_id", (PyCFunction) py_turbulence_ctx_find_conn_by_id, METH_VARARGS,
	 "Allows to find a connection registered on the turbulence connection manager with the provided connection id. The method returns the connection (vortex.Connection) or None in case of failure. This python method provides access to turbulence_conn_mgr_find_by_id"},
	/* broadcast_msg */
	{"broadcast_msg", (PyCFunction) py_turbulence_ctx_broadcast_msg, METH_VARARGS,
	 "Allows to broadcast the provided message on all registered connections matching the provided profile and optionally connections not filtered by the provided filter method. This python method provides access to turbulence_conn_mgr_broadcast_msg."},
 	{NULL, NULL, 0, NULL}  
}; 

static PyTypeObject PyTurbulenceCtxType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size*/
    "turbulence.Ctx",              /* tp_name*/
    sizeof(PyTurbulenceCtx),       /* tp_basicsize*/
    0,                         /* tp_itemsize*/
    (destructor)py_turbulence_ctx_dealloc, /* tp_dealloc*/
    0,                         /* tp_print*/
    0,                         /* tp_getattr*/
    0,                         /* tp_setattr*/
    0,                         /* tp_compare*/
    0,                         /* tp_repr*/
    0,                         /* tp_as_number*/
    0,                         /* tp_as_sequence*/
    0,                         /* tp_as_mapping*/
    0,                         /* tp_hash */
    0,                         /* tp_call*/
    0,                         /* tp_str*/
    py_turbulence_ctx_get_attr,    /* tp_getattro*/
    py_turbulence_ctx_set_attr,    /* tp_setattro*/
    0,                         /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags*/
    "Turbulence context object required to function with Turbulence API",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    py_turbulence_ctx_methods,     /* tp_methods */
    0, /* py_turbulence_ctx_members, */     /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)py_turbulence_ctx_init_type,      /* tp_init */
    0,                         /* tp_alloc */
    py_turbulence_ctx_new,         /* tp_new */

};

/** 
 * @brief Allows to check if the PyObject received represents a
 * PyTurbulenceCtx reference.
 */
axl_bool             py_turbulence_ctx_check    (PyObject          * obj)
{
	/* check null references */
	if (obj == NULL)
		return axl_false;

	/* return check result */
	return PyObject_TypeCheck (obj, &PyTurbulenceCtxType);
}

/** 
 * @brief Allows to configure the context reference to be used by the
 * provided PyTurbulenceCtx
 *
 * @param py_tbc_ctx The PyTurbulenceCtx to configure.
 * @param tbc_ctx The TurbulenceCtx to configure.
 */
void     py_turbulence_ctx_set   (PyObject      * py_tbc_ctx, 
				  TurbulenceCtx * tbc_ctx)
{
	PyTurbulenceCtx * _py_tbc_ctx = (PyTurbulenceCtx *) py_tbc_ctx;

	/* check reference received */
	if (! py_turbulence_ctx_check (py_tbc_ctx))
		return;

	/* configure context */
	_py_tbc_ctx->ctx = tbc_ctx;

	return;
}

/** 
 * @brief Allows to create a PyTurbulenceCtx reference which
 * configures the TurbulenceCtx reference provided.
 *
 * @param ctx The TurbulenceCtx to configure in the newly created
 * reference.
 *
 * @return A new reference to a python object representing the
 * turbulence context.
 */
PyObject * py_turbulence_ctx_create   (TurbulenceCtx * ctx)
{
	/* return a new instance */
	PyTurbulenceCtx * obj = (PyTurbulenceCtx *) PyObject_CallObject ((PyObject *) &PyTurbulenceCtxType, NULL); 

	/* check ref created */
	if (obj == NULL) {
		py_vortex_log (PY_VORTEX_CRITICAL, "Failed to create PyTurbulenceCtx object, returning NULL");
		return NULL;
	} /* end if */

	/* configure context reference */
	obj->ctx = ctx;

	/* now create vortex.Ctx reference and associated it to this
	   instance. */
	obj->py_vortex_ctx = py_vortex_ctx_create (TBC_VORTEX_CTX (ctx));
	
	/* return reference created */
	return __PY_OBJECT (obj);
}

void init_turbulence_ctx (PyObject * module)
{
	/* register type */
	if (PyType_Ready(&PyTurbulenceCtxType) < 0)
		return;
	
	Py_INCREF (&PyTurbulenceCtxType);
	PyModule_AddObject(module, "Ctx", (PyObject *)&PyTurbulenceCtxType);
	return;
}
