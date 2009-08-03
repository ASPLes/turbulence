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
	py_vortex_log (PY_VORTEX_DEBUG, "collecting turbulence.Ctx ref: %p (self->ctx: %p)", self, self->ctx);

	/* free the node it self */
	self->ob_type->tp_free ((PyObject*)self);

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
		return py_vortex_ctx_create (turbulence_ctx_get_vortex_ctx (self->ctx));
	} /* end if */

	/* first implement generic attr already defined */
	result = PyObject_GenericGetAttr (o, attr_name);
	if (result)
		return result;
	
	return NULL;
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
	TurbulenceCtx * ctx = self->ctx;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "z", &message))
		return NULL;

	/* drop the log */
	msg (message);
	
	Py_INCREF (Py_None);
	return Py_None;
}

static PyMethodDef py_turbulence_ctx_methods[] = { 
	/* msg */
	{"msg", (PyCFunction) py_turbulence_ctx_msg, METH_VARARGS,
	 "Records a turbulence log message (msg ()). This is sent to the configured log and showed on the console according to the configuration."},
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
}
