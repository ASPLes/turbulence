/**
 * C client stub to invoke services exported by the XML-RPC component: sasl-radmin.
 *
 * This file was generated by xml-rpc-gen tool, from Vortex Library
 * project.
 *
 * Vortex Library homepage: http://vortex.aspl.es
 * Axl Library homepage: http://xml.aspl.es
 * Advanced Software Production Line: http://www.aspl.es
 */
#include <sasl_radmin_types.h>

/* opaque definition */
struct __SaslUserArray {
	SaslUser ** array;
	int count;
};

XmlRpcArray    * sasl_radmin_sasluserarray_marshall   (VortexCtx * _ctx_, SaslUserArray * ref, axl_bool  dealloc)
{
	/* array and method value */
	XmlRpcArray * _result;
	XmlRpcMethodValue * _array_value;

	/* iterator */
	int  iterator = 0;

	/* access variables */
	SaslUser * _value;
	XmlRpcStruct * _struct;

	if (ref == NULL)
		return NULL;

	/* create the XmlRpcArray */
	_result = vortex_xml_rpc_array_new (ref->count);

	while (iterator < ref->count) {
		/* get the array value */
		_value = ref->array[iterator];

		/* translate the value */
		_struct = sasl_radmin_sasluser_marshall (_ctx_, _value, axl_false);
		_array_value = method_value_new (_ctx_, XML_RPC_STRUCT_VALUE, _struct);

		/* add the value to the array */
		vortex_xml_rpc_array_add (_result, _array_value);

		/* update the iterator */
		iterator++;
	}

	/* dealloc the SaslUserArray reference */
	if (dealloc)
		sasl_radmin_sasluserarray_free (ref);

	/* return the array created */
	return _result;
}

SaslUserArray * sasl_radmin_sasluserarray_unmarshall (XmlRpcArray * ref, axl_bool  dealloc)
{
	SaslUserArray * _result;
	SaslUser * _value;
	XmlRpcStruct * _rpc_value;
	XmlRpcMethodValue * _array_value;
	int  iterator = 0;

	if (ref == NULL)
		return NULL;

	/* create the array */
	_result = sasl_radmin_sasluserarray_new (vortex_xml_rpc_array_count (ref));

	while (iterator < _result->count) {
		/* get the method value inside */
		_array_value = vortex_xml_rpc_array_get (ref, iterator);

		/* translate the value */
		_rpc_value = method_value_get_as_struct (_array_value);
		_value     = sasl_radmin_sasluser_unmarshall (_rpc_value, axl_false);
		
		/* set the value */
		_result->array[iterator] = _value;

		/* update the iterator */
		iterator++;
	}
	/* deallocate memory used by the xml-rpc array */
	if (dealloc)
		vortex_xml_rpc_array_free (ref);

	return _result;
}

SaslUserArray * sasl_radmin_sasluserarray_new  (int count)
{
	SaslUserArray * array;
	/* create the reference */
	array        = axl_new (SaslUserArray, 1);
	array->count = count;
	array->array = axl_new (SaslUser *, count);

	/* return the result */
	return array;
}

SaslUserArray * sasl_radmin_sasluserarray_copy (SaslUserArray * ref)
{
	SaslUserArray * array;
	int         iterator = 0;

	if (ref == NULL)
		return NULL;

	/* create the reference */
	array        = axl_new (SaslUserArray, 1);
	array->count = ref->count;
	if (array->count == 0)
		return array;

	/* allocate enough space */
	array->array = axl_new (SaslUser *, array->count);
	while (iterator < ref->count) {

		/* copy position */
		array->array[iterator] = sasl_radmin_sasluser_copy (ref->array[iterator]);

		/* update the iterator */
		iterator++;
	}

	/* return array created */
	return array;
}

void sasl_radmin_sasluserarray_free (SaslUserArray * ref)
{
	int iterator = 0;

	if (ref == NULL)
		return;

	while (iterator < ref->count) {
		/* release the content */
		sasl_radmin_sasluser_free (ref->array[iterator]);

		/* update the iterator */
		iterator++;
	}

	/* release the array reference itself */
	axl_free (ref->array);
	axl_free (ref);

	return;
}

SaslUser * sasl_radmin_sasluserarray_get (SaslUserArray * ref, int index)
{
	/* check received reference */
	v_return_val_if_fail (ref, NULL);

	/* check index access */
	v_return_val_if_fail (index >= 0 &&  index < ref->count,  NULL);

	/* return the content */
	return ref->array[index];
}

void sasl_radmin_sasluserarray_set (SaslUserArray * ref, int index, SaslUser * value)
{
	v_return_if_fail (ref);
	/* check index access */
	v_return_if_fail (index >= 0 &&  index < ref->count);

	/* set the value */
	ref->array [index] = value;

	return;
}

void sasl_radmin_sasluserarray_add (SaslUserArray * ref, SaslUser * value)
{
	int iterator = 0;

	/* check index access */
	v_return_if_fail (ref && value);

	/* find the next free bucket */
	while (iterator < ref->count) {

		/* check free bucket */
		if (ref->array[iterator] == NULL) {
			/* found free bucket, set the data and return */
			ref->array[iterator] = value;
			return;
		} /* end if */

		/* next position */
		iterator++;

	} /* end while */

	return;
}

int sasl_radmin_sasluserarray_count (SaslUserArray * ref)
{
	/* perform some checks */
	v_return_val_if_fail (ref, -1);

	/* return the count */
	return ref->count;
}

