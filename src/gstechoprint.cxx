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

#include <echoprint/Codegen.h>

#include "gstechoprint.h"

GST_DEBUG_CATEGORY_STATIC (gst_echoprint_debug);
#define GST_CAT_DEFAULT gst_echoprint_debug

/***
 * For old gstreamer in Diablo 
 */
#ifndef GST_MESSAGE_SRC_NAME
#define GST_MESSAGE_SRC_NAME(message)   (GST_MESSAGE_SRC(message) ? \
     GST_OBJECT_NAME (GST_MESSAGE_SRC(message)) : "(NULL)")
#endif
#ifndef GST_CHECK_VERSION
#define	GST_CHECK_VERSION(major,minor,micro)	\
    (GST_VERSION_MAJOR > (major) || \
    (GST_VERSION_MAJOR == (major) && GST_VERSION_MINOR > (minor)) || \
    (GST_VERSION_MAJOR == (major) && GST_VERSION_MINOR == (minor) && \
     GST_VERSION_MICRO >= (micro)))
#endif

/* Filter signals and args */
enum
{
LAST_SIGNAL
};

enum
{
PROP_0,
PROP_INTERVAL,
PROP_SECONDS,
};

#define ECHOPRINT_CAPS "audio/x-raw-float, channels=(int)1, rate=(int)11025, width=(int)32 , endianness=1234"
#define EP_RATE (11025)

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

#if GST_CHECK_VERSION(0,10,14)
gst_element_class_set_details_simple (element_class,
	"echoprint",
	"Filter/Echoprint",
	"Echoprint codegenerator using libcodegen",
	"Kaj-Michael Lang <milang@tal.org>");
#else
GstElementDetails details;

details.longname = "echoprint";
details.klass = "Filter/Echoprint";
details.description = "Echoprint codegenerator using libcodegen";
details.author = "Kaj-Michael Lang <milang@tal.org>";

gst_element_class_set_details(element_class, &details);
#endif

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

g_object_class_install_property (gobject_class, PROP_INTERVAL, g_param_spec_boolean ("interval", "Interval", "Post code every 10 second intervals up to Seconds", FALSE, (GParamFlags)G_PARAM_READWRITE));
g_object_class_install_property (gobject_class, PROP_SECONDS, g_param_spec_int ("seconds", "Seconds", "Max seconds of audio to use for code generation", 10, 120, 30, (GParamFlags)G_PARAM_READWRITE));

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
filter->interval=FALSE;
filter->nextint=10;
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

filter->nextint=10;
filter->done=FALSE;
filter->buffer=gst_buffer_new();

return TRUE;
}

static gboolean
gst_echoprint_stop(GstBaseTransform *base)
{
Gstechoprint *filter=GST_ECHOPRINT(base);

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
	case PROP_INTERVAL:
		GST_OBJECT_LOCK(object);
		filter->interval=g_value_get_boolean (value);
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
	case PROP_INTERVAL:
		GST_OBJECT_LOCK(object);
		g_value_set_boolean (value, filter->interval);
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

static GstFlowReturn
gst_echoprint_transform_ip(GstBaseTransform *base, GstBuffer *outbuf)
{
Codegen *cg;
const float *rawdata;
std::string ecode;
Gstechoprint *filter=GST_ECHOPRINT(base);
guint size, samp, use_seconds;

/* If we are done, skip all work */
if (filter->done)
	return GST_FLOW_OK;

/* Collect buffers for codegeneration, until we have enough to work on */
gst_buffer_ref(outbuf);
filter->buffer=gst_buffer_join(filter->buffer, outbuf);
size=GST_BUFFER_SIZE(filter->buffer);

use_seconds=filter->interval ? filter->nextint : filter->seconds;

if ((size/sizeof(float))/EP_RATE < use_seconds)
	return GST_FLOW_OK;

rawdata=(const float *)GST_BUFFER_DATA(filter->buffer);

samp=use_seconds*EP_RATE;

cg=new Codegen(rawdata, samp, 0);
ecode=cg->getCodeString();
filter->code=ecode.c_str();
gst_element_post_message(GST_ELEMENT (filter), gst_echoprint_message_new(filter));

if (filter->interval)
	filter->nextint+=10;

if (filter->interval==FALSE || (filter->interval==TRUE && filter->nextint>filter->seconds)) {
	filter->done=TRUE;
	gst_buffer_unref(filter->buffer);
	filter->buffer=NULL;
}

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
	"Echoprint codegen element",
	echoprint_init,
	VERSION,
	"LGPL",
	"GStreamer",
	"http://gstreamer.net/"
)
