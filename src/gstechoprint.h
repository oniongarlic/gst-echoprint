/* 
 * GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 * Copyright (C) 2009-2010 Kaj-Michael Lang <milang@tal.org>
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
 
#ifndef __GST_ECHOPRINT_H__
#define __GST_ECHOPRINT_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/gstaudiofilter.h>

G_BEGIN_DECLS

#define GST_TYPE_ECHOPRINT \
  (gst_echoprint_get_type())
#define GST_ECHOPRINT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_ECHOPRINT,Gstechoprint))
#define GST_ECHOPRINT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_ECHOPRINT,GstechoprintClass))
#define GST_IS_ECHOPRINT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ECHOPRINT))
#define GST_IS_ECHOPRINT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_ECHOPRINT))

typedef struct _Gstechoprint      Gstechoprint;
typedef struct _GstechoprintClass GstechoprintClass;

struct _Gstechoprint {
	GstAudioFilter element;

	/* Private */
	gboolean silent;
	gboolean done;
	guint seconds;
	GstBuffer *buffer;
	const gchar *code;
};

struct _GstechoprintClass {
	GstAudioFilterClass parent_class;
};

GType gst_echoprint_get_type (void);

G_END_DECLS

#endif /* __GST_ECHOPRINT_H__ */
