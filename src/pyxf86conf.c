#include <glib.h>
#include <locale.h>
#include "pyxf86conf.h"
#include "xf86ParserExt.h"

/************* globals ******************/

static GHashTable *wrappers_hash = NULL;

static PyMethodDef no_methods[] = {
    {NULL, NULL, 0, NULL}           /* sentinel */
};

typedef enum {
  ATTRIBUTE_STRING,
  ATTRIBUTE_INT,
  ATTRIBUTE_ULONG,
  ATTRIBUTE_FLOAT,
  ATTRIBUTE_RGB,
  ATTRIBUTE_RANGE,
  ATTRIBUTE_ARRAY,
  ATTRIBUTE_LIST,
  ATTRIBUTE_WRAPPER
} XF86ConfAttributeType;

typedef struct {
  const char *name;
  int struct_offset;
  XF86ConfAttributeType type;
  void *data;
  int array_size;
} WrapperAttribute;

static PyObject *pyxf86config_wrap (void *struct_ptr,
				    PyObject *owner,
				    PyTypeObject *type);
static PyObject *pyxf86config_wraplist (void *list_head,
					PyObject *owner,
					PyTypeObject *type);
static PyObject *pyxf86config_wraparray (void *array,
					 PyObject *owner,
					 XF86ConfAttributeType type,
					 int size);
static PyObject *pyxf86wrapper_getattr (XF86WrapperObject *self, char *name,
					WrapperAttribute *attributes);
static int pyxf86wrapper_setattr (XF86WrapperObject *self, char *name,
				  PyObject *obj, WrapperAttribute *attributes);
static void pyxf86wrapper_break (void *struct_ptr, PyTypeObject *type);

/********** Option list ****************/

static void
pyxf86option_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else {
    xf86freeOption (wrapper->struct_ptr);
  }

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute option_attributes[] = {
  { "name", offsetof(XF86OptionRec,opt_name), ATTRIBUTE_STRING},
  { "val", offsetof(XF86OptionRec,opt_val), ATTRIBUTE_STRING},
  { "used", offsetof(XF86OptionRec,opt_used), ATTRIBUTE_INT},
  { "comment", offsetof(XF86OptionRec,opt_comment), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86option_repr (XF86WrapperObject *self)
{
  XF86OptionPtr option = self->struct_ptr;
  char *str;
  PyObject *py_str;

  str = g_strdup_printf ("<XF86Option name='%s' val='%s' used=%d>",
			 option->opt_name, option->opt_val, option->opt_used);
  py_str = PyString_FromString (str);
  g_free (str);
  return py_str;
}

static PyObject *
pyxf86option_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, option_attributes);
}

static int
pyxf86option_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, option_attributes);
}

static PyTypeObject XF86OptionType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86Option",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86option_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86option_getattr,
  (setattrfunc) pyxf86option_setattr,
  (cmpfunc)     0,
  (reprfunc)    pyxf86option_repr,
};

static PyObject *
pyxf86option_new (PyObject *self, PyObject *args)
{
  char *name = NULL;
  char *val = NULL;
  XF86OptionPtr option;

  if (!PyArg_ParseTuple (args, "|zz", &name, &val))
    return NULL;

  option = calloc (1, sizeof (XF86OptionRec));

  if (name)
    option->opt_name = strdup (name);
  if (val)
    option->opt_val = strdup (val);

  return pyxf86config_wrap (option,
			    NULL,
			    &XF86OptionType);
}

/********** Load section ****************/

static void
pyxf86confload_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeLoad (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute load_attributes[] = {
  { "type", offsetof(XF86LoadRec,load_type), ATTRIBUTE_STRING},
  { "name", offsetof(XF86LoadRec,load_name), ATTRIBUTE_STRING},
  { "options", offsetof(XF86LoadRec,load_opt), ATTRIBUTE_LIST, &XF86OptionType},
  { "comment", offsetof(XF86LoadRec,load_comment), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confload_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, load_attributes);
}

static int
pyxf86confload_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, load_attributes);
}

static PyTypeObject XF86ConfLoadType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfLoad",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confload_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confload_getattr,
  (setattrfunc) pyxf86confload_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confload_new (PyObject *self, PyObject *args)
{
  XF86LoadPtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86LoadRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ConfLoadType);
}


/********** Module section ****************/

static void
pyxf86confmodule_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeModules (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute module_attributes[] = {
  { "load", offsetof(XF86ConfModuleRec,mod_load_lst), ATTRIBUTE_LIST, &XF86ConfLoadType},
  { "comment", offsetof(XF86ConfModuleRec,mod_comment), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confmodule_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, module_attributes);
}

static int
pyxf86confmodule_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, module_attributes);
}

static PyTypeObject XF86ConfModuleType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfModule",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confmodule_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confmodule_getattr,
  (setattrfunc) pyxf86confmodule_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confmodule_new (PyObject *self, PyObject *args)
{
  XF86ConfModulePtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfModuleRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ConfModuleType);
}

/********** Flags section ****************/

static void
pyxf86confflags_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeFlags (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute flags_attributes[] = {
  { "options", offsetof(XF86ConfFlagsRec,flg_option_lst), ATTRIBUTE_LIST, &XF86OptionType},
  { "comment", offsetof(XF86ConfFlagsRec,flg_comment), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confflags_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, flags_attributes);
}

static int
pyxf86confflags_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, flags_attributes);
}

static PyTypeObject XF86ConfFlagsType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfFlags",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confflags_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confflags_getattr,
  (setattrfunc) pyxf86confflags_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confflags_new (PyObject *self, PyObject *args)
{
  XF86ConfFlagsPtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfFlagsRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ConfFlagsType);
}

/********** VideoPort section ****************/

static void
pyxf86confvideoport_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeVideoPort (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute videoport_attributes[] = {
  { "identifier", offsetof(XF86ConfVideoPortRec,vp_identifier), ATTRIBUTE_STRING},
  { "options", offsetof(XF86ConfVideoPortRec,vp_option_lst), ATTRIBUTE_LIST, &XF86OptionType},
  { "comment", offsetof(XF86ConfVideoPortRec,vp_comment), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confvideoport_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, videoport_attributes);
}

static int
pyxf86confvideoport_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, videoport_attributes);
}

static PyTypeObject XF86ConfVideoPortType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfVideoPort",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confvideoport_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confvideoport_getattr,
  (setattrfunc) pyxf86confvideoport_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confvideoport_new (PyObject *self, PyObject *args)
{
  XF86ConfVideoPortPtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfVideoPortRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ConfVideoPortType);
}

/********** VideoAdaptor section ****************/

static void
pyxf86confvideoadaptor_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeVideoAdaptor (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute videoadaptor_attributes[] = {
  { "identifier", offsetof(XF86ConfVideoAdaptorRec,va_identifier), ATTRIBUTE_STRING},
  { "vendor", offsetof(XF86ConfVideoAdaptorRec,va_vendor), ATTRIBUTE_STRING},
  { "board", offsetof(XF86ConfVideoAdaptorRec,va_board), ATTRIBUTE_STRING},
  { "busid", offsetof(XF86ConfVideoAdaptorRec,va_busid), ATTRIBUTE_STRING},
  { "driver", offsetof(XF86ConfVideoAdaptorRec,va_driver), ATTRIBUTE_STRING},
  { "options", offsetof(XF86ConfVideoAdaptorRec,va_option_lst), ATTRIBUTE_LIST, &XF86OptionType},
  { "ports", offsetof(XF86ConfVideoAdaptorRec,va_port_lst), ATTRIBUTE_LIST, &XF86ConfVideoPortType},
  { "fwdref", offsetof(XF86ConfVideoAdaptorRec,va_fwdref), ATTRIBUTE_STRING},
  { "comment", offsetof(XF86ConfVideoAdaptorRec,va_comment), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confvideoadaptor_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, videoadaptor_attributes);
}

static int
pyxf86confvideoadaptor_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, videoadaptor_attributes);
}

static PyTypeObject XF86ConfVideoAdaptorType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfVideoAdaptor",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confvideoadaptor_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confvideoadaptor_getattr,
  (setattrfunc) pyxf86confvideoadaptor_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confvideoadaptor_new (PyObject *self, PyObject *args)
{
  XF86ConfVideoAdaptorPtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfVideoAdaptorRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ConfVideoAdaptorType);
}


/********** ModeLine section ****************/

static void
pyxf86confmodeline_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeModeLine (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute modeline_attributes[] = {
  { "identifier", offsetof(XF86ConfModeLineRec,ml_identifier), ATTRIBUTE_STRING},
  { "clock", offsetof(XF86ConfModeLineRec,ml_clock), ATTRIBUTE_INT},
  { "hdisplay", offsetof(XF86ConfModeLineRec,ml_hdisplay), ATTRIBUTE_INT},
  { "hsyncstart", offsetof(XF86ConfModeLineRec,ml_hsyncstart), ATTRIBUTE_INT},
  { "hsyncend", offsetof(XF86ConfModeLineRec,ml_hsyncend), ATTRIBUTE_INT},
  { "htotal", offsetof(XF86ConfModeLineRec,ml_htotal), ATTRIBUTE_INT},
  { "vdisplay", offsetof(XF86ConfModeLineRec,ml_vdisplay), ATTRIBUTE_INT},
  { "vsyncstart", offsetof(XF86ConfModeLineRec,ml_vsyncstart), ATTRIBUTE_INT},
  { "vsyncend", offsetof(XF86ConfModeLineRec,ml_vsyncend), ATTRIBUTE_INT},
  { "vtotal", offsetof(XF86ConfModeLineRec,ml_vtotal), ATTRIBUTE_INT},
  { "vscan", offsetof(XF86ConfModeLineRec,ml_vscan), ATTRIBUTE_INT},
  { "flags", offsetof(XF86ConfModeLineRec,ml_flags), ATTRIBUTE_INT},
  { "hskew", offsetof(XF86ConfModeLineRec,ml_hskew), ATTRIBUTE_INT},
  { "comment", offsetof(XF86ConfModeLineRec,ml_comment), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confmodeline_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, modeline_attributes);
}

static int
pyxf86confmodeline_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, modeline_attributes);
}

static PyTypeObject XF86ConfModeLineType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfModeLine",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confmodeline_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confmodeline_getattr,
  (setattrfunc) pyxf86confmodeline_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confmodeline_new (PyObject *self, PyObject *args)
{
  XF86ConfModeLinePtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfModeLineRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ConfModeLineType);
}

/********** Modes section ****************/

static void
pyxf86confmodes_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeVideoAdaptor (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute modes_attributes[] = {
  { "identifier", offsetof(XF86ConfModesRec,modes_identifier), ATTRIBUTE_STRING},
  { "modelines", offsetof(XF86ConfModesRec,mon_modeline_lst), ATTRIBUTE_LIST, &XF86ConfModeLineType},
  { "comment", offsetof(XF86ConfModesRec,modes_comment), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confmodes_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, modes_attributes);
}

static int
pyxf86confmodes_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, modes_attributes);
}

static PyTypeObject XF86ConfModesType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfModes",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confmodes_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confmodes_getattr,
  (setattrfunc) pyxf86confmodes_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confmodes_new (PyObject *self, PyObject *args)
{
  XF86ConfModesPtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfModesRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ConfModesType);
}

/********** ModesLink section ****************/

static void
pyxf86confmodeslink_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeModesLink (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute modeslink_attributes[] = {
  { "modes", offsetof(XF86ConfModesLinkRec,ml_modes_str), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confmodeslink_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, modeslink_attributes);
}

static int
pyxf86confmodeslink_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, modeslink_attributes);
}

static PyTypeObject XF86ConfModesLinkType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfModesLink",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confmodeslink_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confmodeslink_getattr,
  (setattrfunc) pyxf86confmodeslink_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confmodeslink_new (PyObject *self, PyObject *args)
{
  XF86ConfModesLinkPtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfModesLinkRec));

  return pyxf86config_wrap (s,
		    NULL,
			    &XF86ConfModesLinkType);
}

/********** AdaptorLink section ****************/

static void
pyxf86confadaptorlink_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeAdaptorLink (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute adaptorlink_attributes[] = {
  { "adaptor", offsetof(XF86ConfAdaptorLinkRec,al_adaptor_str), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confadaptorlink_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, adaptorlink_attributes);
}

static int
pyxf86confadaptorlink_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, adaptorlink_attributes);
}

static PyTypeObject XF86ConfAdaptorLinkType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfAdaptorLink",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confadaptorlink_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confadaptorlink_getattr,
  (setattrfunc) pyxf86confadaptorlink_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confadaptorlink_new (PyObject *self, PyObject *args)
{
  XF86ConfAdaptorLinkPtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfAdaptorLinkRec));

  return pyxf86config_wrap (s,
		    NULL,
			    &XF86ConfAdaptorLinkType);
}


/********** Monitor section ****************/

static void
pyxf86confmonitor_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeMonitor (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute monitor_attributes[] = {
  { "identifier", offsetof(XF86ConfMonitorRec,mon_identifier), ATTRIBUTE_STRING},
  { "vendor", offsetof(XF86ConfMonitorRec,mon_vendor), ATTRIBUTE_STRING},
  { "modelname", offsetof(XF86ConfMonitorRec,mon_modelname), ATTRIBUTE_STRING},
  { "width", offsetof(XF86ConfMonitorRec,mon_width), ATTRIBUTE_INT},
  { "height", offsetof(XF86ConfMonitorRec,mon_height), ATTRIBUTE_INT},
  { "modelines", offsetof(XF86ConfMonitorRec,mon_modeline_lst), ATTRIBUTE_LIST, &XF86ConfModeLineType},
  { "n_hsync", offsetof(XF86ConfMonitorRec,mon_n_hsync), ATTRIBUTE_INT},
  { "hsync", offsetof(XF86ConfMonitorRec,mon_hsync), ATTRIBUTE_ARRAY, GINT_TO_POINTER(ATTRIBUTE_RANGE), CONF_MAX_HSYNC},
  { "n_vrefresh", offsetof(XF86ConfMonitorRec,mon_n_vrefresh), ATTRIBUTE_INT},
  { "vrefresh", offsetof(XF86ConfMonitorRec,mon_vrefresh), ATTRIBUTE_ARRAY, GINT_TO_POINTER(ATTRIBUTE_RANGE), CONF_MAX_VREFRESH},
  { "gamma_red", offsetof(XF86ConfMonitorRec,mon_gamma_red), ATTRIBUTE_FLOAT},
  { "gamma_green", offsetof(XF86ConfMonitorRec,mon_gamma_green), ATTRIBUTE_FLOAT},
  { "gamma_blue", offsetof(XF86ConfMonitorRec,mon_gamma_blue), ATTRIBUTE_FLOAT},
  { "options", offsetof(XF86ConfMonitorRec,mon_option_lst), ATTRIBUTE_LIST, &XF86OptionType},
  { "modes", offsetof(XF86ConfMonitorRec,mon_modes_sect_lst), ATTRIBUTE_LIST, &XF86ConfModesLinkType},
  { "comment", offsetof(XF86ConfMonitorRec,mon_comment), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confmonitor_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, monitor_attributes);
}

static int
pyxf86confmonitor_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, monitor_attributes);
}

static PyTypeObject XF86ConfMonitorType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfMonitor",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confmonitor_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confmonitor_getattr,
  (setattrfunc) pyxf86confmonitor_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confmonitor_new (PyObject *self, PyObject *args)
{
  XF86ConfMonitorPtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfMonitorRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ConfMonitorType);
}

/********** Input section **************/

static void
pyxf86confinput_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeInput (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute input_attributes[] = {
  { "identifier", offsetof(XF86ConfInputRec,inp_identifier), ATTRIBUTE_STRING},
  { "driver", offsetof(XF86ConfInputRec,inp_driver), ATTRIBUTE_STRING},
  { "options", offsetof(XF86ConfInputRec,inp_option_lst), ATTRIBUTE_LIST, &XF86OptionType},
  { "comment", offsetof(XF86ConfInputRec,inp_comment), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confinput_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, input_attributes);
}

static int
pyxf86confinput_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, input_attributes);
}

static PyTypeObject XF86ConfInputType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfInput",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confinput_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confinput_getattr,
  (setattrfunc) pyxf86confinput_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};


static PyObject *
pyxf86confinput_new (PyObject *self, PyObject *args)
{
  XF86ConfInputPtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfInputRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ConfInputType);
}

/********** Device section ****************/

static void
pyxf86confdevice_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeDevice (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute device_attributes[] = {
  { "identifier", offsetof(XF86ConfDeviceRec,dev_identifier), ATTRIBUTE_STRING},
  { "vendor", offsetof(XF86ConfDeviceRec,dev_vendor), ATTRIBUTE_STRING},
  { "board", offsetof(XF86ConfDeviceRec,dev_board), ATTRIBUTE_STRING},
  { "chipset", offsetof(XF86ConfDeviceRec,dev_chipset), ATTRIBUTE_STRING},
  { "busid", offsetof(XF86ConfDeviceRec,dev_busid), ATTRIBUTE_STRING},
  { "card", offsetof(XF86ConfDeviceRec,dev_card), ATTRIBUTE_STRING},
  { "driver", offsetof(XF86ConfDeviceRec,dev_driver), ATTRIBUTE_STRING},
  { "ramdac", offsetof(XF86ConfDeviceRec,dev_ramdac), ATTRIBUTE_STRING},
  { "dac_speeds", offsetof(XF86ConfDeviceRec,dev_dacSpeeds), ATTRIBUTE_ARRAY, GINT_TO_POINTER(ATTRIBUTE_INT), CONF_MAXDACSPEEDS},
  { "videoram", offsetof(XF86ConfDeviceRec,dev_videoram), ATTRIBUTE_INT},
  { "textclockfreq", offsetof(XF86ConfDeviceRec,dev_textclockfreq), ATTRIBUTE_INT},
  { "bios_base", offsetof(XF86ConfDeviceRec,dev_bios_base), ATTRIBUTE_ULONG},
  { "mem_base", offsetof(XF86ConfDeviceRec,dev_mem_base), ATTRIBUTE_ULONG},
  { "io_base", offsetof(XF86ConfDeviceRec,dev_io_base), ATTRIBUTE_ULONG},
  { "clockchip", offsetof(XF86ConfDeviceRec,dev_clockchip), ATTRIBUTE_STRING},
  { "n_clocks", offsetof(XF86ConfDeviceRec,dev_clocks), ATTRIBUTE_INT},
  { "clocks", offsetof(XF86ConfDeviceRec,dev_clock), ATTRIBUTE_ARRAY, GINT_TO_POINTER(ATTRIBUTE_INT), CONF_MAXCLOCKS},
  { "chipid", offsetof(XF86ConfDeviceRec,dev_chipid), ATTRIBUTE_INT},
  { "chiprev", offsetof(XF86ConfDeviceRec,dev_chiprev), ATTRIBUTE_INT},
  { "irq", offsetof(XF86ConfDeviceRec,dev_irq), ATTRIBUTE_INT},
  { "screen", offsetof(XF86ConfDeviceRec,dev_screen), ATTRIBUTE_INT},
  { "options", offsetof(XF86ConfDeviceRec,dev_option_lst), ATTRIBUTE_LIST, &XF86OptionType},
  { "comment", offsetof(XF86ConfDeviceRec,dev_comment), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confdevice_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, device_attributes);
}

static int
pyxf86confdevice_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, device_attributes);
}

static PyTypeObject XF86ConfDeviceType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfDevice",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confdevice_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confdevice_getattr,
  (setattrfunc) pyxf86confdevice_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confdevice_new (PyObject *self, PyObject *args)
{
  XF86ConfDevicePtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfDeviceRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ConfDeviceType);
}

/********** Mode section ****************/

static void
pyxf86confmode_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeMode (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute mode_attributes[] = {
  { "name", offsetof(XF86ModeRec,mode_name), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confmode_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, mode_attributes);
}

static int
pyxf86confmode_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, mode_attributes);
}

static PyTypeObject XF86ModeType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86Mode",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confmode_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confmode_getattr,
  (setattrfunc) pyxf86confmode_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confmode_new (PyObject *self, PyObject *args)
{
  XF86ModePtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ModeRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ModeType);
}

/********** Display section ****************/

static void
pyxf86confdisplay_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeDisplay (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute display_attributes[] = {
  { "frameX0", offsetof(XF86ConfDisplayRec,disp_frameX0), ATTRIBUTE_INT},
  { "frameY0", offsetof(XF86ConfDisplayRec,disp_frameY0), ATTRIBUTE_INT},
  { "virtualX", offsetof(XF86ConfDisplayRec,disp_virtualX), ATTRIBUTE_INT},
  { "virtualY", offsetof(XF86ConfDisplayRec,disp_virtualY), ATTRIBUTE_INT},
  { "depth", offsetof(XF86ConfDisplayRec,disp_depth), ATTRIBUTE_INT},
  { "bpp", offsetof(XF86ConfDisplayRec,disp_bpp), ATTRIBUTE_INT},
  { "weight", offsetof(XF86ConfDisplayRec,disp_weight), ATTRIBUTE_RGB},
  { "black", offsetof(XF86ConfDisplayRec,disp_black), ATTRIBUTE_RGB},
  { "white", offsetof(XF86ConfDisplayRec,disp_white), ATTRIBUTE_RGB},
  { "modes", offsetof(XF86ConfDisplayRec,disp_mode_lst), ATTRIBUTE_LIST, &XF86ModeType},
  { "options", offsetof(XF86ConfDisplayRec,disp_option_lst), ATTRIBUTE_LIST, &XF86OptionType},
  { "comment", offsetof(XF86ConfDisplayRec,disp_comment), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confdisplay_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, display_attributes);
}

static int
pyxf86confdisplay_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, display_attributes);
}

static PyTypeObject XF86ConfDisplayType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfDisplay",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confdisplay_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confdisplay_getattr,
  (setattrfunc) pyxf86confdisplay_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confdisplay_new (PyObject *self, PyObject *args)
{
  XF86ConfDisplayPtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfDisplayRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ConfDisplayType);
}

/********** Screen section ****************/

static void
pyxf86confscreen_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeScreen (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute screen_attributes[] = {
  { "identifier", offsetof(XF86ConfScreenRec,scrn_identifier), ATTRIBUTE_STRING},
  { "obso_driver", offsetof(XF86ConfScreenRec,scrn_obso_driver), ATTRIBUTE_STRING},
  { "defaultdepth", offsetof(XF86ConfScreenRec,scrn_defaultdepth), ATTRIBUTE_INT},
  { "defaultbpp", offsetof(XF86ConfScreenRec,scrn_defaultbpp), ATTRIBUTE_INT},
  { "defaultfbbpp", offsetof(XF86ConfScreenRec,scrn_defaultfbbpp), ATTRIBUTE_INT},
  { "monitor", offsetof(XF86ConfScreenRec,scrn_monitor_str), ATTRIBUTE_STRING},
  { "device", offsetof(XF86ConfScreenRec,scrn_device_str), ATTRIBUTE_STRING},
  { "adaptor", offsetof(XF86ConfScreenRec,scrn_adaptor_lst), ATTRIBUTE_LIST, &XF86ConfAdaptorLinkType},
  { "display", offsetof(XF86ConfScreenRec,scrn_display_lst), ATTRIBUTE_LIST, &XF86ConfDisplayType},
  { "options", offsetof(XF86ConfScreenRec,scrn_option_lst), ATTRIBUTE_LIST, &XF86OptionType},
  { "comment", offsetof(XF86ConfScreenRec,scrn_comment), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confscreen_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, screen_attributes);
}

static int
pyxf86confscreen_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, screen_attributes);
}

static PyTypeObject XF86ConfScreenType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfScreen",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confscreen_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confscreen_getattr,
  (setattrfunc) pyxf86confscreen_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confscreen_new (PyObject *self, PyObject *args)
{
  XF86ConfScreenPtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfScreenRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ConfScreenType);
}

/********** Adjacency section ****************/

static void
pyxf86confadjacency_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeAdjacency (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute adjacency_attributes[] = {
  { "screen", offsetof(XF86ConfAdjacencyRec, adj_screen_str), ATTRIBUTE_STRING},
  { "scrnum", offsetof(XF86ConfAdjacencyRec, adj_scrnum), ATTRIBUTE_INT},
  { "top", offsetof(XF86ConfAdjacencyRec, adj_top_str), ATTRIBUTE_STRING},
  { "bottom", offsetof(XF86ConfAdjacencyRec, adj_bottom_str), ATTRIBUTE_STRING},
  { "left", offsetof(XF86ConfAdjacencyRec, adj_left_str), ATTRIBUTE_STRING},
  { "right", offsetof(XF86ConfAdjacencyRec, adj_right_str), ATTRIBUTE_STRING},
  { "where", offsetof(XF86ConfAdjacencyRec, adj_where), ATTRIBUTE_INT},
  { "x", offsetof(XF86ConfAdjacencyRec, adj_x), ATTRIBUTE_INT},
  { "y", offsetof(XF86ConfAdjacencyRec, adj_y), ATTRIBUTE_INT},
  { "refscreen", offsetof(XF86ConfAdjacencyRec, adj_refscreen), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confadjacency_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, adjacency_attributes);
}

static int
pyxf86confadjacency_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, adjacency_attributes);
}

static PyTypeObject XF86ConfAdjacencyType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfAdjacency",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confadjacency_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confadjacency_getattr,
  (setattrfunc) pyxf86confadjacency_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confadjacency_new (PyObject *self, PyObject *args)
{
  XF86ConfAdjacencyPtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfAdjacencyRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ConfAdjacencyType);
}

/********** Inactive section ****************/

static void
pyxf86confinactive_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeInactive (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute inactive_attributes[] = {
  { "device", offsetof(XF86ConfInactiveRec, inactive_device_str), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confinactive_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, inactive_attributes);
}

static int
pyxf86confinactive_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, inactive_attributes);
}

static PyTypeObject XF86ConfInactiveType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfInactive",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confinactive_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confinactive_getattr,
  (setattrfunc) pyxf86confinactive_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confinactive_new (PyObject *self, PyObject *args)
{
  XF86ConfInactivePtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfInactiveRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ConfInactiveType);
}

/********** Inputref section ****************/

static void
pyxf86confinputref_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeInputref (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute inputref_attributes[] = {
  { "inputdev", offsetof(XF86ConfInputrefRec, iref_inputdev_str), ATTRIBUTE_STRING},
  { "options", offsetof(XF86ConfInputrefRec,iref_option_lst), ATTRIBUTE_LIST, &XF86OptionType},
  { NULL }
};

static PyObject *
pyxf86confinputref_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, inputref_attributes);
}

static int
pyxf86confinputref_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, inputref_attributes);
}

static PyTypeObject XF86ConfInputrefType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfInputref",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confinputref_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confinputref_getattr,
  (setattrfunc) pyxf86confinputref_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confinputref_new (PyObject *self, PyObject *args)
{
  XF86ConfInputrefPtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfInputrefRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ConfInputrefType);
}


/********** Layout section ****************/

static void
pyxf86conflayout_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeLayout (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute layout_attributes[] = {
  { "identifier", offsetof(XF86ConfLayoutRec,lay_identifier), ATTRIBUTE_STRING},
  { "adjacencies", offsetof(XF86ConfLayoutRec,lay_adjacency_lst), ATTRIBUTE_LIST, &XF86ConfAdjacencyType},
  { "inactives", offsetof(XF86ConfLayoutRec,lay_inactive_lst), ATTRIBUTE_LIST, &XF86ConfInactiveType},
  { "inputs", offsetof(XF86ConfLayoutRec,lay_input_lst), ATTRIBUTE_LIST, &XF86ConfInputrefType},
  { "options", offsetof(XF86ConfLayoutRec,lay_option_lst), ATTRIBUTE_LIST, &XF86OptionType},
  { "comment", offsetof(XF86ConfLayoutRec,lay_comment), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86conflayout_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, layout_attributes);
}

static int
pyxf86conflayout_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, layout_attributes);
}

static PyTypeObject XF86ConfLayoutType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfLayout",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86conflayout_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86conflayout_getattr,
  (setattrfunc) pyxf86conflayout_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86conflayout_new (PyObject *self, PyObject *args)
{
  XF86ConfLayoutPtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfLayoutRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ConfLayoutType);
}

/********** buffers section ****************/

static void
pyxf86confbuffers_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeBuffers (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute buffers_attributes[] = {
  { "count", offsetof(XF86ConfBuffersRec,buf_count), ATTRIBUTE_INT},
  { "size", offsetof(XF86ConfBuffersRec,buf_size), ATTRIBUTE_INT},
  { "flags", offsetof(XF86ConfBuffersRec,buf_flags), ATTRIBUTE_STRING},
  { "comment", offsetof(XF86ConfBuffersRec,buf_comment), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confbuffers_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, buffers_attributes);
}

static int
pyxf86confbuffers_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, buffers_attributes);
}

static PyTypeObject XF86ConfBuffersType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfBuffers",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confbuffers_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confbuffers_getattr,
  (setattrfunc) pyxf86confbuffers_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confbuffers_new (PyObject *self, PyObject *args)
{
  XF86ConfBuffersPtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfBuffersRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ConfBuffersType);
}

/********** DRI section ****************/

static void
pyxf86confdri_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeDRI (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute dri_attributes[] = {
  { "group_name", offsetof(XF86ConfDRIRec,dri_group_name), ATTRIBUTE_STRING},
  { "group", offsetof(XF86ConfDRIRec,dri_group), ATTRIBUTE_INT},
  { "mode", offsetof(XF86ConfDRIRec,dri_mode), ATTRIBUTE_INT},
  { "buffers", offsetof(XF86ConfDRIRec,dri_buffers_lst), ATTRIBUTE_LIST, &XF86ConfBuffersType},
  { "comment", offsetof(XF86ConfDRIRec,dri_comment), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confdri_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, dri_attributes);
}

static int
pyxf86confdri_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, dri_attributes);
}

static PyTypeObject XF86ConfDRIType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfDRI",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confdri_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confdri_getattr,
  (setattrfunc) pyxf86confdri_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confdri_new (PyObject *self, PyObject *args)
{
  XF86ConfDRIPtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfDRIRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ConfDRIType);
}

/********** VendSub section ****************/

static void
pyxf86confvendsub_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeVendSub (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute vendsub_attributes[] = {
  { "name", offsetof(XF86ConfVendSubRec,vs_name), ATTRIBUTE_STRING},
  { "identifier", offsetof(XF86ConfVendSubRec,vs_identifier), ATTRIBUTE_STRING},
  { "options", offsetof(XF86ConfVendSubRec,vs_option_lst), ATTRIBUTE_LIST, &XF86OptionType},
  { "comment", offsetof(XF86ConfVendSubRec,vs_comment), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confvendsub_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, vendsub_attributes);
}

static int
pyxf86confvendsub_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, vendsub_attributes);
}

static PyTypeObject XF86ConfVendSubType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfVendSub",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confvendsub_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confvendsub_getattr,
  (setattrfunc) pyxf86confvendsub_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confvendsub_new (PyObject *self, PyObject *args)
{
  XF86ConfVendSubPtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfVendSubRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ConfVendSubType);
}

/********** Vendor section ****************/

static void
pyxf86confvendor_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeVendor (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute vendor_attributes[] = {
  { "identifier", offsetof(XF86ConfVendorRec,vnd_identifier), ATTRIBUTE_STRING},
  { "options", offsetof(XF86ConfVendorRec,vnd_option_lst), ATTRIBUTE_LIST, &XF86OptionType},
  { "sub", offsetof(XF86ConfVendorRec,vnd_sub_lst), ATTRIBUTE_LIST, &XF86ConfVendSubType},
  { "comment", offsetof(XF86ConfVendorRec,vnd_comment), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86confvendor_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, vendor_attributes);
}

static int
pyxf86confvendor_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, vendor_attributes);
}

static PyTypeObject XF86ConfVendorType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfVendor",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86confvendor_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86confvendor_getattr,
  (setattrfunc) pyxf86confvendor_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86confvendor_new (PyObject *self, PyObject *args)
{
  XF86ConfVendorPtr s;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  s = calloc (1, sizeof (XF86ConfVendorRec));

  return pyxf86config_wrap (s,
			    NULL,
			    &XF86ConfVendorType);
}

/********** Files section **************/

static void
pyxf86conffiles_dealloc (XF86WrapperObject *wrapper)
{
  if (wrapper->owner)
    Py_DECREF (wrapper->owner);
  else 
    xf86freeFiles (wrapper->struct_ptr);

  g_hash_table_remove (wrappers_hash, wrapper->struct_ptr);
  PyObject_DEL (wrapper);
}

WrapperAttribute files_attributes[] = {
  { "logfile", offsetof(XF86ConfFilesRec,file_logfile), ATTRIBUTE_STRING},
  { "module", offsetof(XF86ConfFilesRec,file_modulepath), ATTRIBUTE_STRING},
  { "fontpath", offsetof(XF86ConfFilesRec,file_fontpath), ATTRIBUTE_STRING},
  { "comment", offsetof(XF86ConfFilesRec,file_comment), ATTRIBUTE_STRING},
  { NULL }
};

static PyObject *
pyxf86conffiles_getattr (XF86WrapperObject *self, char *name)
{
  return pyxf86wrapper_getattr (self, name, files_attributes);
}

static int
pyxf86conffiles_setattr (XF86WrapperObject *self, char *name, PyObject *obj)
{
  return pyxf86wrapper_setattr (self, name, obj, files_attributes);
}

static PyTypeObject XF86ConfFilesType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86ConfFiles",       /* name */
  sizeof (XF86WrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86conffiles_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86conffiles_getattr,
  (setattrfunc) pyxf86conffiles_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86conffiles_new (PyObject *self, PyObject *args)
{
  XF86ConfFilesPtr files;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;
  
  files = calloc (1, sizeof (XF86ConfFilesRec));

  return pyxf86config_wrap (files,
			    NULL,
			    &XF86ConfFilesType);
}


/****************** Generic wrappers ****************/

static void
attr_typename (GString *str, XF86ConfAttributeType type, void *data)
{
  switch (type)
    {
    case ATTRIBUTE_STRING:
      g_string_append (str, "string");
      break;
    case ATTRIBUTE_INT:
      g_string_append (str, "int");
      break;
    case ATTRIBUTE_ULONG:
      g_string_append (str, "ulong");
      break;
    case ATTRIBUTE_FLOAT:
      g_string_append (str, "float");
      break;
    case ATTRIBUTE_RGB:
      g_string_append (str, "rbg");
      break;
    case ATTRIBUTE_RANGE:
      g_string_append (str, "range");
      break;
    case ATTRIBUTE_ARRAY:
      g_string_append (str, "array of ");
      attr_typename (str, GPOINTER_TO_INT (data), 0);
      break;
    case ATTRIBUTE_LIST:
      g_string_append (str, "list of ");
      g_string_append (str, ((PyTypeObject *)data)->tp_name);
      break;
    case ATTRIBUTE_WRAPPER:
      g_string_append (str, ((PyTypeObject *)data)->tp_name);
      break;
    default:
      g_assert_not_reached ();
    }
}

static PyObject *
pyxf86wrapper_getattr (XF86WrapperObject *self, char *name,
		       WrapperAttribute *attributes)
{
  char *struct_ptr = self->struct_ptr;
  parser_rgb *rgb;
  PyObject *retval;

  if (strcmp (name, "attrs") == 0)
    {
      GString *attrs;

      attrs = g_string_new ("<attributes: ");
      
      while (attributes->name != NULL)
	{
	  g_string_append (attrs, attributes->name);
	  g_string_append (attrs, ":");
	  attr_typename (attrs, attributes->type, attributes->data);
	  
	  attributes++;
	  if (attributes->name != NULL)
	    g_string_append (attrs, ", ");
	}
      g_string_append (attrs, ">");

      retval = Py_BuildValue ("s", attrs->str);
      g_string_free (attrs, TRUE);
      return retval;
    }
  
  while (attributes->name != NULL)
    {
      if (strcmp (attributes->name, name) == 0)
	{
	  switch (attributes->type)
	    {
	    case ATTRIBUTE_STRING:
	      return Py_BuildValue ("z", *(char **)(struct_ptr + attributes->struct_offset));
	      break;
	    case ATTRIBUTE_INT:
	      return Py_BuildValue ("i", *(int *)(struct_ptr + attributes->struct_offset));
	      break;
	    case ATTRIBUTE_ULONG:
	      return Py_BuildValue ("l", *(unsigned long *)(struct_ptr + attributes->struct_offset));
	      break;
	    case ATTRIBUTE_FLOAT:
	      return Py_BuildValue ("f", *(float *)(struct_ptr + attributes->struct_offset));
	      break;
	    case ATTRIBUTE_RGB:
	      rgb = (parser_rgb *)(struct_ptr + attributes->struct_offset);
	      return Py_BuildValue ("(iii)", rgb->red, rgb->green, rgb->blue);
	      break;
	    case ATTRIBUTE_ARRAY:
	      return pyxf86config_wraparray (struct_ptr + attributes->struct_offset,
					     (PyObject *)self,
					     GPOINTER_TO_INT (attributes->data),
					     attributes->array_size);
	    case ATTRIBUTE_LIST:
	      return pyxf86config_wraplist (struct_ptr + attributes->struct_offset,
					    (PyObject *)self,
					    attributes->data);
	      break;
	    case ATTRIBUTE_WRAPPER:
	      return pyxf86config_wrap (*(void **)(struct_ptr + attributes->struct_offset),
					(PyObject *)self,
					attributes->data);
	      break;
	    default:
	      g_assert_not_reached ();
	    }
	}
      
      attributes++;
    }

  return Py_FindMethod(no_methods, (PyObject *)self, name);
}

static int
pyxf86wrapper_setattr (XF86WrapperObject *self, char *name,
		       PyObject *obj, WrapperAttribute *attributes)
{
  char *struct_ptr = self->struct_ptr;
  char **char_ptr, *c;
  void **void_ptr;
  PyObject *item;
  XF86WrapperObject *wrapper;
  parser_rgb rgb;

  while (attributes->name != NULL)
    {
      if (strcmp (attributes->name, name) == 0)
	{
	  switch (attributes->type)
	    {
	    case ATTRIBUTE_STRING:
	      if (obj != Py_None && !PyString_Check (obj))
		{
		  PyErr_SetString (PyExc_TypeError, "Expected a string");
		  return 1;
		}
	      
	      char_ptr = (char **)(struct_ptr + attributes->struct_offset);
	      if (*char_ptr != NULL)
		free (*char_ptr);
	      
	      *char_ptr = NULL;

	      if (obj != Py_None)
		{
		  c = PyString_AsString(obj);
		  if (c != NULL)
		    *char_ptr = strdup (c);
		}
	      
	      return 0;
	      break;
	      
	    case ATTRIBUTE_INT:
	      if (!PyInt_Check (obj))
		{
		  PyErr_SetString (PyExc_TypeError, "Expected an integer");
		  return 1;
		}
	      *(int *)(struct_ptr + attributes->struct_offset) = (int)PyInt_AsLong(obj);
	      return 0;
	      break;
	      
	    case ATTRIBUTE_ULONG:
	      if (!PyLong_Check (obj))
		{
		  PyErr_SetString (PyExc_TypeError, "Expected an integer");
		  return 1;
		}
	      *(unsigned long *)(struct_ptr + attributes->struct_offset) = (int)PyLong_AsUnsignedLong(obj);
	      return 0;
	      break;
	      
	    case ATTRIBUTE_FLOAT:
	      if (!PyFloat_Check (obj))
		{
		  PyErr_SetString (PyExc_TypeError, "Expected a float");
		  return 1;
		}
	      *(float *)(struct_ptr + attributes->struct_offset) = (float)PyFloat_AsDouble(obj);
	      return 0;
	      break;
	    case ATTRIBUTE_RGB:
	      if (!PyTuple_Check (obj) || PyTuple_Size (obj) != 3)
		{
		  PyErr_SetString (PyExc_TypeError, "Expected an int 3-tuple");
		  return 1;
		}
	      item = PyTuple_GetItem (obj, 0);
	      if (item == NULL || !PyInt_Check (item))
		{
		  PyErr_SetString (PyExc_TypeError, "Expected an int 3-tuple");
		  return 1;
		}
	      rgb.red = (int)PyInt_AsLong (item);
	      item = PyTuple_GetItem (obj, 1);
	      if (item == NULL || !PyInt_Check (item))
		{
		  PyErr_SetString (PyExc_TypeError, "Expected an int 3-tuple");
		  return 1;
		}
	      rgb.green = (int)PyInt_AsLong (item);
	      item = PyTuple_GetItem (obj, 2);
	      if (item == NULL || !PyInt_Check (item))
		{
		  PyErr_SetString (PyExc_TypeError, "Expected an int 3-tuple");
		  return 1;
		}
	      rgb.blue = (int)PyInt_AsLong (item);
	      
	      * (parser_rgb *)(struct_ptr + attributes->struct_offset) = rgb;
	      return 0;
	      break;
	    case ATTRIBUTE_WRAPPER:
	      if (!PyObject_TypeCheck (obj, attributes->data))
		{
		  char *error;
		  error = g_strdup_printf ("Expected an object of type %s",
					   ((PyTypeObject *)attributes->data)->tp_name);
		  PyErr_SetString (PyExc_TypeError, error);
		  g_free (error);
		  return 1;
		}

	      /* Remove the old value first */
	      
	      void_ptr = (void **)(struct_ptr + attributes->struct_offset);
	      
	      if (*void_ptr != NULL)
		{
		  /* Break out object from hierarchy */
		  pyxf86wrapper_break (*void_ptr, attributes->data);
		}

	      /* Assign new value */
	      
	      wrapper = (XF86WrapperObject *)obj;
	      *void_ptr = wrapper->struct_ptr;

	      /* Claim ownership of object */
	      Py_INCREF (self);
	      wrapper->owner = (PyObject *)self;

	      break;
	      
	    default:
	      break;
	    }
	}
      
      attributes++;
    }

  PyErr_SetString (PyExc_AttributeError, "No such attribute");
  return 1;
}

/* ------------------------------ arrays -------------------- */

static void
pyxf86genarray_dealloc (XF86ArrayWrapperObject *wrapper)
{
  Py_DECREF (wrapper->owner);
  g_hash_table_remove (wrappers_hash, (char *)wrapper->array + 1);
  PyObject_DEL (wrapper);
}

static int
pyxf86genarray_length (XF86ArrayWrapperObject *wrapper)
{
  return wrapper->size;
}

static PyObject *
pyxf86genarray_getitem (XF86ArrayWrapperObject *wrapper, int index)
{
  if (index < 0 || index >= wrapper->size)
    {
      PyErr_SetString (PyExc_IndexError, "index out-of-bounds");
      return NULL;
    }

  switch ((XF86ConfAttributeType)wrapper->element_type)
    {
    case ATTRIBUTE_INT:
      return Py_BuildValue ("i", ((int *)wrapper->array)[index]);
    case ATTRIBUTE_RANGE:
      return Py_BuildValue ("(f, f)",
			    ((parser_range *)wrapper->array)[index].lo,
			    ((parser_range *)wrapper->array)[index].hi);
    default:
      g_assert_not_reached ();
    }

  return NULL;
}

static int
pyxf86genarray_setitem (XF86ArrayWrapperObject *wrapper, int index, PyObject *obj)
{
  parser_range range;
  PyObject *item;
  
  if (index < 0 || index >= wrapper->size)
    {
      PyErr_SetString (PyExc_IndexError, "index out-of-bounds");
      return 1;
    }
  
  switch ((XF86ConfAttributeType)wrapper->element_type)
    {
    case ATTRIBUTE_INT:
      if (!PyInt_Check (obj))
	{
	  PyErr_SetString (PyExc_TypeError, "Expected an integer");
	  return 1;
	}
      range.lo = (float)PyInt_AsLong(obj);
      range.hi = (float)PyInt_AsLong(obj);
      ((parser_range *)wrapper->array)[index] = range;
      break;
    case ATTRIBUTE_RANGE:
      if (!PyTuple_Check (obj) || PyTuple_Size (obj) != 2)
	{
	  PyErr_SetString (PyExc_TypeError, "Expected an float 2-tuple");
	  return 1;
	}
      item = PyTuple_GetItem (obj, 0);
      if (item == NULL || !PyFloat_Check (item))
	{
	  PyErr_SetString (PyExc_TypeError, "Expected an float 2-tuple");
	  return 1;
	}
      range.lo = (float)PyFloat_AsDouble(item);
      item = PyTuple_GetItem (obj, 1);
      if (item == NULL || !PyFloat_Check (item))
	{
	  PyErr_SetString (PyExc_TypeError, "Expected an float 2-tuple");
	  return 1;
	}
      range.hi = (float)PyFloat_AsDouble(item);
      
      ((parser_range *)wrapper->array)[index] = range;
      
      break;
    default:
      g_assert_not_reached ();
    }
  return 0;
}

static PySequenceMethods genarray_as_sequence = {
  (inquiry)    pyxf86genarray_length,  /* sq_length : len(x) */
  (binaryfunc) 0,                     /* sq_concat : x + y  */
  (intargfunc) 0,                     /* sq_repeat : x * n  */
  (intargfunc) pyxf86genarray_getitem, /* sq_item   : x[i]   */
  (intintargfunc) 0,    /* sq_slice     : x[i:j]     */
  (intobjargproc) pyxf86genarray_setitem,    /* sq_ass_item  : x[i] = v   */
  (intintobjargproc) 0, /* sq_ass_slice : x[i:j] = v */
};

static PyTypeObject XF86GenArrayType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86GenericArray",  /* name */
  sizeof (XF86ArrayWrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86genarray_dealloc,
  (printfunc)   0,
  (getattrfunc) 0,
  (setattrfunc) 0,
  (cmpfunc)     0,
  (reprfunc)    0,

  /* Type categories */
  0,                     /* as_number */
  &genarray_as_sequence,  /* as_sequence */
  0,                     /* as_mapping */
  (hashfunc) 0,
  (ternaryfunc) 0,
  (reprfunc) 0,
};

static PyObject *
pyxf86config_wraparray (void *array,
			PyObject *owner,
			XF86ConfAttributeType type,
			int size)
{
  XF86ArrayWrapperObject *wrapper;
  char *array_key;
  
  if (wrappers_hash == NULL)
    wrappers_hash = g_hash_table_new (NULL, NULL);

  /* HACK: The array may be the same as an struct address
   * if the list is the first member of the struct. To handle
   * this we add 1 to all arrays. This assumes that pointers
   * can never be odd.
   */
  array_key = (char *)array + 1;
  
  wrapper = g_hash_table_lookup (wrappers_hash, array_key);
  if (wrapper)
    {
      Py_INCREF (wrapper);
      return (PyObject *)wrapper;
    }

  wrapper = PyObject_NEW (XF86ArrayWrapperObject, &XF86GenArrayType);
  if (wrapper == NULL)
    return NULL;
  
  Py_INCREF (owner);
  
  wrapper->owner = owner;
  wrapper->array = array;
  wrapper->element_type = type;
  wrapper->size = size;

  g_hash_table_insert (wrappers_hash,
		       array_key,
		       wrapper);
  
  return (PyObject *)wrapper;
}

/* ------------------------------ lists -------------------- */

static void
pyxf86genlist_dealloc (XF86ListWrapperObject *wrapper)
{
  Py_DECREF (wrapper->owner);
  g_hash_table_remove (wrappers_hash, (char *)wrapper->list_head + 1);
  PyObject_DEL (wrapper);
}

static int
pyxf86genlist_length (XF86ListWrapperObject *wrapper)
{
  GenericListPtr l;
  int len;

  l = *wrapper->list_head;

  len = 0;
  while (l)
    {
      len++;
      l = l->next;
    }
  return len;
}

static PyObject *
pyxf86genlist_getitem (XF86ListWrapperObject *wrapper, int index)
{
  GenericListPtr l;
  
  if (index < 0)
    {
      PyErr_SetString (PyExc_IndexError, "index out-of-bounds");
      return NULL;
    }


  l = *wrapper->list_head;
  while (l)
    {
      if (index == 0)
	return pyxf86config_wrap (l, (PyObject *)wrapper,
				  wrapper->list_type);
      
      index--;
      l = l->next;
    }
  
  PyErr_SetString (PyExc_IndexError, "index out-of-bounds");
  return NULL;
}

static PyObject *
pyxf86genlist_insert (XF86ListWrapperObject *wrapper, PyObject *args)
{
  int pos = -1;
  XF86WrapperObject *obj;
  GenericListPtr *l;
  
  if (!PyArg_ParseTuple (args, "O!|i", wrapper->list_type, &obj, &pos))
    return NULL;

  if (obj->owner != NULL)
    {
      PyErr_SetString (PyExc_ValueError, "You can only put a XF86Config type in one list");
      return NULL;
    }

  if (pos <0)
    pos = pyxf86genlist_length (wrapper);

  l = wrapper->list_head;
  while (TRUE)
    {
      if (pos == 0)
	{
	  /* Insert into list */
	  ((GenericListPtr)(obj->struct_ptr))->next = *l;
	  *l = obj->struct_ptr;

	  /* Claim ownership of object */
	  Py_INCREF (wrapper);
	  obj->owner = (PyObject *)wrapper;
	  
	  return Py_BuildValue ("");
	}
      
      if (*l == NULL)
	break;
      
      pos--;
      l = (GenericListPtr *)&( (*l)->next );
    }
  
  PyErr_SetString (PyExc_IndexError, "index out-of-bounds");
  return NULL;
}

static PyObject *
pyxf86genlist_remove (XF86ListWrapperObject *wrapper, PyObject *args)
{
  int pos;
  GenericListPtr *l;
  GenericListPtr found;
  
  if (!PyArg_ParseTuple (args, "i", &pos))
    return NULL;

  l = wrapper->list_head;
  while (*l)
    {
      if (pos == 0)
	{
	  /* Remove from list */
	  found = *l;
	  *l = found->next;
	  found->next = NULL;

	  /* Break out object from list */
	  pyxf86wrapper_break (found, wrapper->list_type);
	  
	  return Py_BuildValue ("");
	}
      
      if (*l == NULL)
	break;
      
      pos--;
      l = (GenericListPtr *)&( (*l)->next );
    }
  
  PyErr_SetString (PyExc_IndexError, "index out-of-bounds");
  return NULL;
}

static PyObject *
pyxf86genlist_size (XF86ListWrapperObject *wrapper, PyObject *args)
{
    return PyInt_FromLong (pyxf86genlist_length (wrapper));
}

static PyMethodDef genlist_methods[] = {
  {"insert", (PyCFunction)pyxf86genlist_insert, METH_VARARGS},
  {"remove", (PyCFunction)pyxf86genlist_remove, METH_VARARGS},
  {"size", (PyCFunction)pyxf86genlist_size, METH_VARARGS},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

static PyObject *
pyxf86genlist_getattr (XF86ListWrapperObject *self, char *name)
{
  return Py_FindMethod(genlist_methods, (PyObject *)self, name);
}

static PySequenceMethods genlist_as_sequence = {
  (inquiry)    pyxf86genlist_length,  /* sq_length : len(x) */
  (binaryfunc) 0,                     /* sq_concat : x + y  */
  (intargfunc) 0,                     /* sq_repeat : x * n  */
  (intargfunc) pyxf86genlist_getitem, /* sq_item   : x[i]   */
  (intintargfunc) 0,    /* sq_slice     : x[i:j]     */
  (intobjargproc) 0,    /* sq_ass_item  : x[i] = v   */
  (intintobjargproc) 0, /* sq_ass_slice : x[i:j] = v */
};

static PyTypeObject XF86GenListType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86GenericList",  /* name */
  sizeof (XF86ListWrapperObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86genlist_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86genlist_getattr,
  (setattrfunc) 0,
  (cmpfunc)     0,
  (reprfunc)    0,

  /* Type categories */
  0,                     /* as_number */
  &genlist_as_sequence,  /* as_sequence */
  0,                     /* as_mapping */
  (hashfunc) 0,
  (ternaryfunc) 0,
  (reprfunc) 0,
};

static PyObject *
pyxf86config_wraplist (void *list_head,
		       PyObject *owner,
		       PyTypeObject *type)
{
  XF86ListWrapperObject *wrapper;
  char *list_key;
  
  if (wrappers_hash == NULL)
    wrappers_hash = g_hash_table_new (NULL, NULL);

  /* HACK: The list head may be the same as an struct address
   * if the list is the first member of the struct. To handle
   * this we add 1 to all list keys. This assumes that pointers
   * can never be odd.
   */
  list_key = (char *)list_head + 1;
  
  wrapper = g_hash_table_lookup (wrappers_hash, list_key);
  if (wrapper)
    {
      Py_INCREF (wrapper);
      return (PyObject *)wrapper;
    }

  wrapper = PyObject_NEW (XF86ListWrapperObject, &XF86GenListType);
  if (wrapper == NULL)
    return NULL;
  
  Py_INCREF (owner);
  
  wrapper->owner = owner;
  wrapper->list_head = list_head;
  wrapper->list_type = type;

  g_hash_table_insert (wrappers_hash,
		       list_key,
		       wrapper);
  
  return (PyObject *)wrapper;
}


/* ------------------------------ structs -------------------- */

static PyObject *
pyxf86config_wrap (void *struct_ptr,
		   PyObject *owner,
		   PyTypeObject *type)
{
  XF86WrapperObject *wrapper;

  if (struct_ptr == NULL)
    return Py_BuildValue ("");

  if (wrappers_hash == NULL)
    wrappers_hash = g_hash_table_new (NULL, NULL);
  
  wrapper = g_hash_table_lookup (wrappers_hash, struct_ptr);
  if (wrapper)
    {
      Py_INCREF (wrapper);
      return (PyObject *)wrapper;
    }
  
  wrapper = PyObject_NEW (XF86WrapperObject, type);
  if (wrapper == NULL)
    return NULL;
  
  Py_XINCREF (owner);
  
  wrapper->owner = owner;
  wrapper->struct_ptr = struct_ptr;

  g_hash_table_insert (wrappers_hash,
		       struct_ptr,
		       wrapper);
  
  return (PyObject *)wrapper;
}

/* Breaks a structure out of the XF86Config hierarchy.
 * If there are no external refs to it, it is freed, otherwise
 * it is marked unowned so it will be freed on the last unref.
 */
static void
pyxf86wrapper_break (void *struct_ptr, PyTypeObject *type)
{
  XF86WrapperObject *wrapper;

  /* Gives new ref */
  wrapper = (XF86WrapperObject *)
    pyxf86config_wrap (struct_ptr, NULL, type);
  
  if (wrapper->owner)
    {
      /* Was an old one */
      Py_DECREF (wrapper->owner);
      wrapper->owner = NULL;
    }
  
  /* Kill our ref */
  Py_DECREF (wrapper);
}


/****************** Config object *******************/

staticforward PyTypeObject XF86ConfigType;

static XF86ConfigObject *
_pyxf86config_new (XF86ConfigPtr config)
{
  XF86ConfigObject *self;

  self = PyObject_NEW (XF86ConfigObject, &XF86ConfigType);
  if (self == NULL)
    return NULL;
  
  self->config = config;

  return self;
}

static void
pyxf86config_dealloc (XF86ConfigObject *config)
{
  xf86freeConfig (config->config);
  config->config = NULL;
  PyObject_DEL (config);
}

static PyObject *
pyxf86config_write (XF86ConfigObject *config, PyObject *args)
{
  char *filename;
  int res;
  char *old_locale;
  
  if (!PyArg_ParseTuple (args, "s", &filename))
    return NULL;

  /* HACK - if the module list is empty, set conf_modules to NULL so
   * X doesn't write out an empty section.  A better fix would be to
   * make del work on XF86ConfModule, but I don't want to rewrite this
   * stuff enough to make it work.
   */
  if (config->config->conf_modules && !config->config->conf_modules->mod_load_lst)
     config->config->conf_modules = NULL;

  old_locale = setlocale (LC_NUMERIC, "C");
  res = xf86writeConfigFile (filename, config->config);
  setlocale (LC_NUMERIC, old_locale);
  if (!res)
    {
      PyErr_SetString (PyExc_TypeError, "Error writing config file");
      return NULL;
    }
  
  return Py_BuildValue ("");
}


static PyMethodDef config_methods[] = {
  {"write", (PyCFunction)pyxf86config_write, METH_VARARGS},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

static PyObject *
pyxf86config_getattr (XF86ConfigObject *self, char *name)
{
  XF86ConfigPtr config = self->config;
  
  if (strcmp (name, "attrs") == 0) {
    return Py_BuildValue ("s", "<attributes: files, modules, flags, videoadaptor, modes, monitor, device, screen, input, layout, vendor, dri, comment>");
  } else if (strcmp (name, "files") == 0) {
    return pyxf86config_wrap (config->conf_files, (PyObject *)self,
			      &XF86ConfFilesType);
  } else if (strcmp (name, "modules") == 0) {
    return pyxf86config_wrap (config->conf_modules, (PyObject *)self,
			      &XF86ConfModuleType);
  } else if (strcmp (name, "flags") == 0) {
    return pyxf86config_wrap (config->conf_flags, (PyObject *)self,
			      &XF86ConfFlagsType);
  } else if (strcmp (name, "videoadaptor") == 0) {
    return pyxf86config_wraplist (&config->conf_videoadaptor_lst, (PyObject *)self,
				  &XF86ConfVideoAdaptorType);
  } else if (strcmp (name, "modes") == 0) {
    return pyxf86config_wraplist (&config->conf_modes_lst, (PyObject *)self,
				  &XF86ConfModesType);
  } else if (strcmp (name, "monitor") == 0) {
    return pyxf86config_wraplist (&config->conf_monitor_lst, (PyObject *)self,
				  &XF86ConfMonitorType);
  } else if (strcmp (name, "device") == 0) {
    return pyxf86config_wraplist (&config->conf_device_lst, (PyObject *)self,
				  &XF86ConfDeviceType);
  } else if (strcmp (name, "screen") == 0) {
    return pyxf86config_wraplist (&config->conf_screen_lst, (PyObject *)self,
				  &XF86ConfScreenType);
  } else if (strcmp (name, "input") == 0) {
    return pyxf86config_wraplist (&config->conf_input_lst,
				  (PyObject *)self,
				  &XF86ConfInputType);
  } else if (strcmp (name, "layout") == 0) {
    return pyxf86config_wraplist (&config->conf_layout_lst,
				  (PyObject *)self,
				  &XF86ConfLayoutType);
  } else if (strcmp (name, "vendor") == 0) {
    return pyxf86config_wraplist (&config->conf_vendor_lst,
				  (PyObject *)self,
				  &XF86ConfVendorType);
  } else if (strcmp (name, "dri") == 0) {
    return pyxf86config_wrap (config->conf_dri, (PyObject *)self,
			      &XF86ConfDRIType);
  } else if (strcmp (name, "comment") == 0) {
    return Py_BuildValue ("z", config->conf_comment);
  }

  return Py_FindMethod(config_methods, (PyObject *)self, name);
}

static int
set_obj (void **ptr, PyObject *obj,
	 XF86ConfigObject *owner, PyTypeObject *type)
{
  XF86WrapperObject *wrapper = (XF86WrapperObject *)obj;
  
  if (!PyObject_TypeCheck (obj, type))
    {
      char *error;
      error = g_strdup_printf ("Expected an object of type %s",
			       type->tp_name);
      PyErr_SetString (PyExc_TypeError, error);
      g_free (error);
      return 1;
    }

  if (*ptr != NULL)
    {
      /* Break out object from hierarchy */
      pyxf86wrapper_break (*ptr, type);
    }
  
  /* Assign new value */
  
  *ptr = wrapper->struct_ptr;
  
  /* Claim ownership of object */
  Py_INCREF (owner);
  wrapper->owner = (PyObject *)owner;
  return 0;
}

static int
pyxf86config_setattr (XF86ConfigObject *self, char *name, PyObject *obj)
{
  char *c;
  
  if (strcmp (name, "files") == 0) {
    return set_obj ((void **)&(self->config->conf_files), obj,
		    self, &XF86ConfFilesType);
  } else if (strcmp (name, "modules") == 0) {
    return set_obj ((void **)&(self->config->conf_modules), obj,
		    self, &XF86ConfModuleType);
  } else if (strcmp (name, "flags") == 0) {
    return set_obj ((void **)&(self->config->conf_flags), obj,
		    self, &XF86ConfFlagsType);
  } else if (strcmp (name, "dri") == 0) {
    return set_obj ((void **)&(self->config->conf_dri), obj,
		    self, &XF86ConfDRIType);
  } else if (strcmp (name, "comment") == 0) {
    if (!PyString_Check (obj))
      {
	PyErr_SetString (PyExc_TypeError, "Expected a string");
	return 1;
      }

    if (self->config->conf_comment != NULL)
      free (self->config->conf_comment);
    self->config->conf_comment = NULL;

    c = PyString_AsString(obj);
    if (c != NULL)
      self->config->conf_comment = strdup (c);

    return 0;
  }

  return 1;
}

static PyTypeObject XF86ConfigType = {
  PyObject_HEAD_INIT (&PyType_Type)
  0,                  /* size */
  "XF86Config",       /* name */
  sizeof (XF86ConfigObject), /* object size */
  0,                  /* itemsize */

  /* standard methods */
  (destructor)  pyxf86config_dealloc,
  (printfunc)   0,
  (getattrfunc) pyxf86config_getattr,
  (setattrfunc) pyxf86config_setattr,
  (cmpfunc)     0,
  (reprfunc)    0,
};

static PyObject *
pyxf86config_new (PyObject *self, PyObject *args)
{
  XF86ConfigPtr config;

  if (!PyArg_ParseTuple (args, ""))
    return NULL;

  config = calloc (1, sizeof (XF86ConfigRec));

  return (PyObject *)_pyxf86config_new (config);
}

/***************** Module *********************/

static PyObject *
pyxf86addComment(PyObject *self, PyObject *args)
{
  char *s1, *s2;
  PyObject *obj;

  if (!PyArg_ParseTuple (args, "ss", &s1, &s2))
    return NULL;

  s1 = strdup (s1);

  s1 = xf86addComment (s1, s2);

  obj = Py_BuildValue ("s", s1);
  
  free (s1);

  return obj;
}

static PyObject *
pyxf86readConfigFile(PyObject *self, PyObject *args)
{
  char *path = NULL;
  char *cmdline = NULL;
  char *projroot = NULL;
  const char *read_file;
  char *old_locale;
  XF86ConfigPtr config;
  
  if (!PyArg_ParseTuple (args, "|zzz", &path, &cmdline, &projroot))
    return NULL;
  
  config = NULL;
  
  old_locale = setlocale (LC_NUMERIC, "C");
  read_file = xf86openConfigFile (path, cmdline, projroot);
  setlocale (LC_NUMERIC, old_locale);
  if (read_file)
    {
      read_file = strdup (read_file);

      config = xf86readConfigFile ();
  
      xf86closeConfigFile ();
    }

  if (config)
    return Py_BuildValue ("(Ns)", _pyxf86config_new (config), read_file);
  else
    return Py_BuildValue ("(Os)", Py_None, read_file);
}

static struct PyMethodDef xf86config_methods[] = {
  { "readConfigFile", pyxf86readConfigFile, METH_VARARGS },
  { "addComment", pyxf86addComment, METH_VARARGS },
  { "XF86Option", pyxf86option_new, METH_VARARGS },
  { "XF86ConfFiles", pyxf86conffiles_new, METH_VARARGS },
  { "XF86ConfModule", pyxf86confmodule_new, METH_VARARGS },
  { "XF86ConfLoad", pyxf86confload_new, METH_VARARGS },
  { "XF86ConfFlags", pyxf86confflags_new, METH_VARARGS },
  { "XF86ConfVideoPort", pyxf86confvideoport_new, METH_VARARGS },
  { "XF86ConfVideoAdaptor", pyxf86confvideoadaptor_new, METH_VARARGS },
  { "XF86ConfModeLine", pyxf86confmodeline_new, METH_VARARGS },
  { "XF86ConfModes", pyxf86confmodes_new, METH_VARARGS },
  { "XF86ConfModesLink", pyxf86confmodeslink_new, METH_VARARGS },
  { "XF86ConfMonitor", pyxf86confmonitor_new, METH_VARARGS },
  { "XF86ConfInput", pyxf86confinput_new, METH_VARARGS },
  { "XF86ConfDevice", pyxf86confdevice_new, METH_VARARGS },
  { "XF86ConfAdaptorLink", pyxf86confadaptorlink_new, METH_VARARGS },
  { "XF86Mode", pyxf86confmode_new, METH_VARARGS },
  { "XF86ConfDisplay", pyxf86confdisplay_new, METH_VARARGS },
  { "XF86ConfScreen", pyxf86confscreen_new, METH_VARARGS },
  { "XF86ConfAdjacency", pyxf86confadjacency_new, METH_VARARGS },
  { "XF86ConfInactive", pyxf86confinactive_new, METH_VARARGS },
  { "XF86ConfInputref", pyxf86confinputref_new, METH_VARARGS },
  { "XF86ConfLayout", pyxf86conflayout_new, METH_VARARGS },
  { "XF86ConfBuffers", pyxf86confbuffers_new, METH_VARARGS },
  { "XF86ConfDRI", pyxf86confdri_new, METH_VARARGS },
  { "XF86ConfVendSub", pyxf86confvendsub_new, METH_VARARGS },
  { "XF86ConfVendor", pyxf86confvendor_new, METH_VARARGS },
  { "XF86Config", pyxf86config_new, METH_VARARGS },
  { NULL, NULL }
};

/* Module initialization function */
__attribute__((visibility("default"))) void
initixf86config(void)
{
  PyObject *m;
  m = Py_InitModule ("ixf86config", xf86config_methods);
}
