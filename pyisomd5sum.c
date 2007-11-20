/*
 * Copyright (C) 2001-2007 Red Hat, Inc.
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


static PyObject * doCheckIsoMD5Sum(PyObject * s, PyObject * args) {
    char *isofile;
    int rc;

    if (!PyArg_ParseTuple(args, "s", &isofile))
	return NULL;
 
    rc = mediaCheckFile(isofile, 1);

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


void initpyisomd5sum(void) {
    PyObject * m, * d;

    m = Py_InitModule("pyisomd5sum", isomd5sumMethods);
    d = PyModule_GetDict(m);
}
