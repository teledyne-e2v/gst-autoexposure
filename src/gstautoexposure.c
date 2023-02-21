/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2023 teledyne <<user@hostname.org>>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
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
 * SECTION:element-autoexposure
 *
 * FIXME:Describe autoexposure here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! autoexposure ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include "api.h"
#include "gstautoexposure.h"

GST_DEBUG_CATEGORY_STATIC (gst_autoexposure_debug);
#define GST_CAT_DEFAULT gst_autoexposure_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT,
  PROP_WORK
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

#define gst_autoexposure_parent_class parent_class
G_DEFINE_TYPE (Gstautoexposure, gst_autoexposure, GST_TYPE_ELEMENT);

static void gst_autoexposure_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_autoexposure_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_autoexposure_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_autoexposure_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

/* GObject vmethod implementations */

/* initialize the autoexposure's class */
static void
gst_autoexposure_class_init (GstautoexposureClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_autoexposure_set_property;
  gobject_class->get_property = gst_autoexposure_get_property;
  gobject_class->finalize = gst_autoexposure_finalize;
  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));
 g_object_class_install_property (gobject_class, PROP_WORK,
      g_param_spec_boolean ("work", "Work", "enable/disable work",
          TRUE, G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class,
    "autoexposure",
    "FIXME:Generic",
    "FIXME:Generic Template Element",
    "teledyne <<user@hostname.org>>");
	
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_autoexposure_init (Gstautoexposure * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_autoexposure_sink_event));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_autoexposure_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->silent = FALSE;
  filter->work = TRUE;


  initialization("/dev/video0",2);
}

static void
gst_autoexposure_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  Gstautoexposure *filter = GST_AUTOEXPOSURE (object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    case PROP_WORK:
      filter->work = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_autoexposure_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  Gstautoexposure *filter = GST_AUTOEXPOSURE (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    case PROP_WORK:
      g_value_set_boolean (value, filter->work);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_autoexposure_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstautoexposure *filter;
  gboolean ret;

  filter = GST_AUTOEXPOSURE (parent);

  GST_LOG_OBJECT (filter, "Received %s event: %" GST_PTR_FORMAT,
      GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps * caps;

      gst_event_parse_caps (event, &caps);
      /* do something with the caps */

      /* and forward */
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }

  return ret;
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_autoexposure_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  Gstautoexposure *filter;

  filter = GST_AUTOEXPOSURE (parent);


  /* just push out the incoming buffer without touching it */

GstMapInfo map;
gst_buffer_map(buf, &map, GST_MAP_READ);
GstCaps *caps = gst_pad_get_current_caps(pad);
GstStructure *s = gst_caps_get_structure(caps, 0);
gboolean res;
gint width, height;
// we need to get the final caps on the buffer to get the size
res = gst_structure_get_int(s, "width", &width);
res |= gst_structure_get_int(s, "height", &height);
if (!res)
{
g_print("could not get snapshot dimension\n");
exit(-1);
}



if(filter->work){

int tmp_mean;
float global_mean=0;
for(int y=0;y<height;y++)
{
	tmp_mean=0;
	for (int x=0;x<width;x++)
	{
		tmp_mean+=map.data[(y*width)+x];
	}
	global_mean+=tmp_mean/((float)width);
}
global_mean=global_mean/height;
g_print("%f\n",global_mean);

if(global_mean<60)
{
	int gain = get_control("analog_gain");
	if(gain== get_control_max("analog_gain"))
	{
		int gain_d=get_control("digital_gain");
		set_control("digital_gain",gain_d+20);
	}
	else
	{
		set_control("analog_gain",gain+1);
	}
	
}
else if(global_mean>110)
{	
	int gain=get_control("digital_gain");
	if(gain== get_control_min("digital_gain"))
	{
		int gain_a=get_control("analog_gain");
		set_control("analog_gain",gain_a-1);
	}
	else
	{
		set_control("digital_gain",gain-20);
	}
}

}

gst_buffer_unmap(buf, &map);




  return gst_pad_push (filter->srcpad, buf);
}

static void gst_autoexposure_finalize(GObject *object)
{
	g_print("driver closed\n");
	close_driver_access();
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
autoexposure_init (GstPlugin * autoexposure)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template autoexposure' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_autoexposure_debug, "autoexposure",
      0, "Template autoexposure");

  return gst_element_register (autoexposure, "autoexposure", GST_RANK_NONE,
      GST_TYPE_AUTOEXPOSURE);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstautoexposure"
#endif

/* gstreamer looks for this structure to register autoexposures
 *
 * exchange the string 'Template autoexposure' with your autoexposure description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    autoexposure,
    "Template autoexposure",
    autoexposure_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)