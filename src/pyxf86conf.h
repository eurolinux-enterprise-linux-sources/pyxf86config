#include "Python.h"
#include <xf86Parser.h>

typedef struct {
  PyObject_HEAD
  XF86ConfigPtr config;
} XF86ConfigObject;

typedef struct {
  PyObject_HEAD
  PyObject *owner; /* Parent in tree, NULL for orphans */
  void *struct_ptr;
} XF86WrapperObject;

typedef struct {
  PyObject_HEAD
  PyObject *owner; /* Parent in tree, must be non-NULL */
  GenericListPtr *list_head;
  PyTypeObject *list_type;
}  XF86ListWrapperObject;

typedef struct {
  PyObject_HEAD
  PyObject *owner; /* Parent in tree, must be non-NULL */
  void *array;
  int element_type;
  int size;
}  XF86ArrayWrapperObject;
