/*
 * Copyright (C) 2001-2013 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <Python.h>
#include <stdio.h>

#include "libcheckisomd5.h"
#include "libimplantisomd5.h"

static PyObject * doCheckIsoMD5Sum(PyObject * s, PyObject * args);
static PyObject * doImplantIsoMD5Sum(PyObject * s, PyObject * args);

static PyMethodDef isomd5sumMethods[] = {
    { "checkisomd5sum", (PyCFunction) doCheckIsoMD5Sum, METH_VARARGS, NULL },
    { "implantisomd5sum", (PyCFunction) doImplantIsoMD5Sum, METH_VARARGS, NULL },
    { NULL }
} ;

/* Call python object with offset and total
 * If the object returns true return 1 to abort the check
 */
int pythonCB(void *cbdata, long long offset, long long total) {
    PyObject *arglist, *result;
    int rc;

    arglist = Py_BuildValue("(LL)", offset, total);
    result = PyObject_CallObject(cbdata, arglist);
    Py_DECREF(arglist);

    if (result == NULL)
       return 1;

    rc = PyObject_IsTrue(result);
    Py_DECREF(result);
    return (rc > 0);
}

static PyObject * doCheckIsoMD5Sum(PyObject * s, PyObject * args) {
    PyObject *callback = NULL;
    char *isofile;
    int rc;

    if (!PyArg_ParseTuple(args, "s|O", &isofile, &callback))
        return NULL;

    if (callback) {
        if (!PyCallable_Check(callback)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be callable");
            return NULL;
        }

        rc = mediaCheckFile(isofile, pythonCB, callback);
        Py_DECREF(callback);
    } else {
        rc = mediaCheckFile(isofile, NULL, NULL);
    }

    return Py_BuildValue("i", rc);
}

static PyObject * doImplantIsoMD5Sum(PyObject * s, PyObject * args) {
    char *isofile, *errstr;
    int forceit, supported;
    int rc;

    if (!PyArg_ParseTuple(args, "sii", &isofile, &supported, &forceit))
        return NULL;

    rc = implantISOFile(isofile, supported, forceit, 1, &errstr);

    return Py_BuildValue("i", rc);
}

#ifdef PYTHON_ABI_VERSION
static struct PyModuleDef pyisomd5sum = {
    PyModuleDef_HEAD_INIT,
    "pyisomd5sum",
    NULL,
    -1,
    isomd5sumMethods
};

PyMODINIT_FUNC PyInit_pyisomd5sum(void) {
    return PyModule_Create(&pyisomd5sum);
}
#else
void initpyisomd5sum(void) {
    (void)Py_InitModule("pyisomd5sum", isomd5sumMethods);
}
#endif
