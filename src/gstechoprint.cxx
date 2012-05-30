/*
 * GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 * Copyright (C) 2012 Kaj-Michael Lang <milang@tal.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-echoprint
 *
 * Listens to ~30 seconds of audio and posts an echoprint code.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>

#include <Codegen.h>

#include "gstechoprint.h"

GST_DEBUG_CATEGORY_STATIC (gst_echoprint_debug);
#define GST_CAT_DEFAULT gst_echoprint_debug

/* Filter signals and args */
enum
{
LAST_SIGNAL
};

enum
{
PROP_0,
PROP_SILENT,
PROP_SECONDS,
};

#define ECHOPRINT_CAPS "audio/x-raw-float, channels=(int)1, rate=(int)11025, width=(int)32 , endianness=1234"

/* the capabilities of the inputs and outputs.
 *
 */
static GstStaticPadTemplate sink_template=
GST_STATIC_PAD_TEMPLATE (
	"sink",
	GST_PAD_SINK,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS (ECHOPRINT_CAPS)
);

static GstStaticPadTemplate src_template=
GST_STATIC_PAD_TEMPLATE (
	"src",
	GST_PAD_SRC,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS (ECHOPRINT_CAPS)
);

/* debug category for fltering log messages
 *
 */
#define DEBUG_INIT(bla) \
	GST_DEBUG_CATEGORY_INIT (gst_echoprint_debug, "echoprint", 0, "echoprint");

GST_BOILERPLATE_FULL (Gstechoprint, gst_echoprint, GstBaseTransform, GST_TYPE_BASE_TRANSFORM, DEBUG_INIT);

static void gst_echoprint_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_echoprint_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static gboolean gst_echoprint_set_caps (GstBaseTransform * btrans, GstCaps * incaps, GstCaps * outcaps);
static GstFlowReturn gst_echoprint_transform_ip (GstBaseTransform * base, GstBuffer * outbuf);
static gboolean gst_echoprint_start (GstBaseTransform * base);
static gboolean gst_echoprint_stop (GstBaseTransform * base);
static gboolean gst_echoprint_src_event (GstBaseTransform *trans, GstEvent *event);

/* GObject vmethod implementations */

static void
gst_echoprint_base_init(gpointer klass)
{
GstElementClass *element_class=GST_ELEMENT_CLASS (klass);

gst_element_class_set_details_simple (element_class,
	"echoprint",
	"Echoprint/Filter",
	"Echoprint codegenerator using libcodegen",
	" <milang@tal.org>");

gst_element_class_add_pad_template (element_class, gst_static_pad_template_get (&src_template));
gst_element_class_add_pad_template (element_class, gst_static_pad_template_get (&sink_template));
}

/* initialize the echoprint's class */
static void
gst_echoprint_class_init(GstechoprintClass * klass)
{
GObjectClass *gobject_class;

gobject_class=(GObjectClass *) klass;
gobject_class->set_property=gst_echoprint_set_property;
gobject_class->get_property=gst_echoprint_get_property;

g_object_class_install_property (gobject_class, PROP_SILENT, g_param_spec_boolean ("silent", "Silent", "Turn of bus messages", FALSE, (GParamFlags)G_PARAM_READWRITE));
g_object_class_install_property (gobject_class, PROP_SECONDS, g_param_spec_int ("seconds", "Seconds", "Seconds of audio to buffer", 20, 40, 30, (GParamFlags)G_PARAM_READWRITE));

GST_BASE_TRANSFORM_CLASS (klass)->set_caps=GST_DEBUG_FUNCPTR (gst_echoprint_set_caps);
GST_BASE_TRANSFORM_CLASS (klass)->transform_ip=GST_DEBUG_FUNCPTR (gst_echoprint_transform_ip);
GST_BASE_TRANSFORM_CLASS (klass)->start=GST_DEBUG_FUNCPTR (gst_echoprint_start);
GST_BASE_TRANSFORM_CLASS (klass)->stop=GST_DEBUG_FUNCPTR (gst_echoprint_stop);
GST_BASE_TRANSFORM_CLASS (klass)->src_event=GST_DEBUG_FUNCPTR (gst_echoprint_src_event);
}

static gboolean
gst_echoprint_set_caps(GstBaseTransform * btrans, GstCaps * incaps, GstCaps * outcaps)
{
return TRUE;
}

/* initialize the new element
 * initialize instance structure
 */
static void
gst_echoprint_init(Gstechoprint *filter, GstechoprintClass * klass)
{
g_debug("init");

filter->silent=FALSE;
filter->seconds=30;
filter->done=FALSE;
}

static gboolean
gst_echoprint_src_event(GstBaseTransform *trans, GstEvent *event)
{
return TRUE;
}

static gboolean
gst_echoprint_start(GstBaseTransform *base)
{
Gstechoprint *filter=GST_ECHOPRINT(base);

g_debug("start");

filter->silent=FALSE;
filter->done=FALSE;
filter->buffer=gst_buffer_new();

return TRUE;
}

static gboolean
gst_echoprint_stop(GstBaseTransform *base)
{
Gstechoprint *filter=GST_ECHOPRINT(base);

g_debug("stop");

filter->silent=FALSE;
filter->done=FALSE;
if (filter->buffer)
	gst_buffer_unref(filter->buffer);

return TRUE;
}

static void
gst_echoprint_set_property(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
Gstechoprint *filter=GST_ECHOPRINT(object);

switch (prop_id) {
	case PROP_SILENT:
		GST_OBJECT_LOCK(object);
		filter->silent=g_value_get_boolean (value);
		GST_OBJECT_UNLOCK(object);
	break;
	case PROP_SECONDS:
		GST_OBJECT_LOCK(object);
		filter->seconds=g_value_get_int (value);
		GST_OBJECT_UNLOCK(object);
	break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	break;
}
}

static void
gst_echoprint_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
Gstechoprint *filter=GST_ECHOPRINT(object);

switch (prop_id) {
	case PROP_SILENT:
		GST_OBJECT_LOCK(object);
		g_value_set_boolean (value, filter->silent);
		GST_OBJECT_UNLOCK(object);
	break;
	case PROP_SECONDS:
		GST_OBJECT_LOCK(object);
		g_value_set_int (value, filter->seconds);
		GST_OBJECT_UNLOCK(object);
	break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	break;
}
}

inline GstMessage *
gst_echoprint_message_new(Gstechoprint *filter)
{
return gst_message_new_element(GST_OBJECT(filter), gst_structure_new ("echoprint", "code", G_TYPE_STRING, filter->code, NULL));
}

/* GstBaseTransform vmethod implementations */

static GstFlowReturn
gst_echoprint_transform_ip(GstBaseTransform *base, GstBuffer *outbuf)
{
Codegen *cg;
const float *rawdata;
std::string ecode;
Gstechoprint *filter=GST_ECHOPRINT(base);
guint size;

if (filter->done)
	return GST_FLOW_OK;

filter->buffer=gst_buffer_join(filter->buffer, outbuf);
size=GST_BUFFER_SIZE(filter->buffer);

if ((size/sizeof(float))/11025 < filter->seconds)
	return GST_FLOW_OK;

rawdata=(const float *)GST_BUFFER_DATA(filter->buffer);

cg=new Codegen(rawdata, size / sizeof(float), 0);
ecode=cg->getCodeString();

filter->code=ecode.c_str();

gst_element_post_message(GST_ELEMENT (filter), gst_echoprint_message_new(filter));

filter->done=TRUE;

gst_buffer_unref(filter->buffer);
filter->buffer=NULL;

return GST_FLOW_OK;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
echoprint_init (GstPlugin *echoprint)
{
return gst_element_register(echoprint, "echoprint", GST_RANK_NONE, GST_TYPE_ECHOPRINT);
}

/* gstreamer looks for this structure to register echoprints
 *
 */
GST_PLUGIN_DEFINE (
	GST_VERSION_MAJOR,
	GST_VERSION_MINOR,
	"echoprint",
	"Data Matrix barcodes decoder element",
	echoprint_init,
	VERSION,
	"LGPL",
	"GStreamer",
	"http://gstreamer.net/"
)
