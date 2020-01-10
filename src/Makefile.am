AM_CFLAGS = @CFLAGS@ -I../include -I@PYTHON_INC@ @GLIB_CFLAGS@ @XORG_SERVER_CFLAGS@ @x_cflags@ -fvisibility=hidden

pythondir = $(libdir)/python@PYTHON_VERSION@/site-packages

python_PROGRAMS = ixf86configmodule.so
python_DATA = xf86config.py

EXTRA_DIST=xf86config.py

ixf86configmodule_so_SOURCES = pyxf86conf.h pyxf86conf.c xf86ParserExt.h xf86config_ext.c
ixf86configmodule_so_LDFLAGS = -module -shared @GOBJECT_LIBS@
ixf86configmodule_so_LDADD = @X_LIBS@ -lxf86config
