# plugindir is set in configure

plugin_LTLIBRARIES = libgstechoprint.la

# sources used to compile this plug-in
libgstechoprint_la_SOURCES = gstechoprint.cxx

# flags used to compile this plugin
# add other _CFLAGS and _LIBS as needed
libgstechoprint_la_CFLAGS = $(GST_CFLAGS) $(ECHOPRINT_CFLAGS)
libgstechoprint_la_LIBADD = $(GST_LIBS) $(GST_BASE_LIBS) $(ECHOPRINT_LIBS)
libgstechoprint_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstechoprint_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstechoprint.h
