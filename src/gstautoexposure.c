/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 *
 * Copyright (C) 2023 Teledyne e2V, Author Loic Chevallier
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
#include <config.h>
#endif
#include <unistd.h>
#include <stdio.h>
#include <gst/gst.h>
#include "gstautoexposure.h"
#include "algorithm.h"

GST_DEBUG_CATEGORY_STATIC(gst_autoexposure_debug);
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
  PROP_WORK,
  PROP_OPTIMIZE,
  PROP_MAXEXPOSITION,
  PROP_MAXANALOGGAIN,
  PROP_USEDIGITALGAIN,
  PROP_MINDIGITALGAIN,
  PROP_MAXDIGITALGAIN,
  PROP_USEEXPOSITIONTIME,
  PROP_LATENCY,
  PROP_TARGET,
  PROP_THRESHOLD,
  PROP_ROI1X,
  PROP_ROI1Y,
  PROP_ROI2X,
  PROP_ROI2Y,
  PROP_USEHISTOGRAM,
  PROP_LOADANDSAVECONF,
  PROP_DEBUG

  /*,
  PROP_HISTOGRAM*/
};


void write_conf(int exposure, int analog_gain, int digital_gain);
int read_conf(int *exposure, int *analog_gain, int *digital_gain);
double valeur_moyenne(int histo[], int taille);

void write_conf(int exposure, int analog_gain, int digital_gain)
{
  FILE *fichier = fopen("/tmp/exposure.txt", "w");
  if (fichier == NULL)
  {
    printf("Error : Can't open tmp file\n");
    return;
  }

  fprintf(fichier, "%d %d %d", exposure, analog_gain, digital_gain);
  fclose(fichier);
}

int read_conf(int *exposure, int *analog_gain, int *digital_gain)
{
int err;
  FILE *fichier = fopen("/tmp/exposure.txt", "r");
  if (fichier == NULL)
  {
    return 0;
  }

  err = fscanf(fichier, "%d %d %d", exposure, analog_gain, digital_gain);
  if (err == EOF)
  {
    return 0;
  }

  fclose(fichier);
  return 1;
}

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
                                                                   GST_PAD_SINK,
                                                                   GST_PAD_ALWAYS,
                                                                   GST_STATIC_CAPS("ANY"));

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE("src",
                                                                  GST_PAD_SRC,
                                                                  GST_PAD_ALWAYS,
                                                                  GST_STATIC_CAPS("ANY"));

#define gst_autoexposure_parent_class parent_class
G_DEFINE_TYPE(Gstautoexposure, gst_autoexposure, GST_TYPE_ELEMENT);

static void gst_autoexposure_set_property(GObject *object, guint prop_id,
                                          const GValue *value, GParamSpec *pspec);
static void gst_autoexposure_get_property(GObject *object, guint prop_id,
                                          GValue *value, GParamSpec *pspec);

static gboolean gst_autoexposure_sink_event(GstPad *pad, GstObject *parent, GstEvent *event);
static GstFlowReturn gst_autoexposure_chain(GstPad *pad, GstObject *parent, GstBuffer *buf);

/* GObject vmethod implementations */

/* initialize the autoexposure's class */
static void
gst_autoexposure_class_init(GstautoexposureClass *klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *)klass;
  gstelement_class = (GstElementClass *)klass;

  gobject_class->set_property = gst_autoexposure_set_property;
  gobject_class->get_property = gst_autoexposure_get_property;
  gobject_class->finalize = gst_autoexposure_finalize;
  g_object_class_install_property(gobject_class, PROP_SILENT,
                                  g_param_spec_boolean("silent", "Silent", "Produce verbose output ?",
                                                       FALSE, G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_WORK,
                                  g_param_spec_boolean("work", "Work", "enable/disable autoexposure (usefull only for applications)",
                                                       TRUE, G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_USEEXPOSITIONTIME,
                                  g_param_spec_boolean("useExpositionTime", "UseExpositionTime", "enable/disable exposition time usage",
                                                       TRUE, G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_USEHISTOGRAM,
                                  g_param_spec_boolean("useHistogram", "UseHistogram", "not implemented yet, please do not use",
                                                       FALSE, G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_OPTIMIZE,
                                  g_param_spec_int("optimize", "Optimize", "Optimization level, used to reduce CPU load", 0, 5, 1, G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_TARGET,
                                  g_param_spec_int("target", "Target", "Targeted mean of the image, an higher the target will produce brighter the image", 0, 255, 60, G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_LATENCY,
                                  g_param_spec_int("latency", "Latency", "Pipeline latency, Really important, if the image is flickering, this is the most probable cause", 0, 100, 4, G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_LOADANDSAVECONF,
                                  g_param_spec_boolean("loadAndSaveConf", "LoadAndSaveConf", "Load and save the exposure / gain parameters and load them when starting the plugin",
                                                       TRUE, G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class, PROP_ROI1X,
                                  g_param_spec_int("roi1x", "Roi1x", "Roi coordinates", 0, 1920, 0, G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_ROI1Y,
                                  g_param_spec_int("roi1y", "Roi1y", "Roi coordinates", 0, 1080, 0, G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_ROI2X,
                                  g_param_spec_int("roi2x", "Roi2x", "Roi coordinates", 0, 1920, 1920, G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_ROI2Y,
                                  g_param_spec_int("roi2y", "Roi2y", "Roi coordinates", 0, 1080, 1080, G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class, PROP_MAXEXPOSITION,
                                  g_param_spec_int("maxExposition", "MaxExposition", "Maximum exposition tolerate",
                                                   5, 200000, 5, G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_MAXANALOGGAIN,
                                  g_param_spec_int("maxAnalogGain", "MaxAnalogGain", "Maximum analog gain tolerate",
                                                   0, 15, 15, G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_USEDIGITALGAIN,
                                  g_param_spec_boolean("useDigitalGain", "UseDigitalGain", "Enable/disable digital gain usage",
                                                       FALSE, G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_DEBUG,
                                  g_param_spec_boolean("debug", "Debug", "Enable/disable debug mode, show the exposure parametre / time to converge",
                                                       FALSE, G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_THRESHOLD,
                                  g_param_spec_int("threshold", "Threshold", "Threshold of target",
                                                   5, 100, 10, G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_MINDIGITALGAIN,
                                  g_param_spec_int("minDigitalGain", "MinDigitalGain", "minimum digital gain useable by the algorithm",
                                                   1, 2000, 256, G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_MAXDIGITALGAIN,
                                  g_param_spec_int("maxDigitalGain", "MaxDigitalGain", "maximum digital gain useable by the algorithm",
                                                   1, 4096, 4096, G_PARAM_READWRITE));
  gst_element_class_set_details_simple(gstelement_class,
                                       "autoexposure",
                                       "FIXME:Generic",
                                       "FIXME:Generic Template Element",
                                       "teledyne <<user@hostname.org>>");

  gst_element_class_add_pad_template(gstelement_class,
                                     gst_static_pad_template_get(&src_factory));
  gst_element_class_add_pad_template(gstelement_class,
                                     gst_static_pad_template_get(&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_autoexposure_init(Gstautoexposure *filter)
{
  char flux[] = "/dev/video0";
  filter->sinkpad = gst_pad_new_from_static_template(&sink_factory, "sink");
  gst_pad_set_event_function(filter->sinkpad,
                             GST_DEBUG_FUNCPTR(gst_autoexposure_sink_event));
  gst_pad_set_chain_function(filter->sinkpad,
                             GST_DEBUG_FUNCPTR(gst_autoexposure_chain));
  GST_PAD_SET_PROXY_CAPS(filter->sinkpad);
  gst_element_add_pad(GST_ELEMENT(filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template(&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS(filter->srcpad);
  gst_element_add_pad(GST_ELEMENT(filter), filter->srcpad);

  filter->silent = FALSE;
  filter->work = TRUE;
  filter->optimize = 1;
  filter->maxExposition = 20000;
  filter->useExpositionTime = TRUE;
  filter->latency = 4;
  filter->target = 60;
  filter->ROI1x = 0;
  filter->ROI1y = 0;
  filter->ROI2x = 1920;
  filter->ROI2y = 1080;
  filter->useHistogram = FALSE;
  filter->maxAnalogGain = 15;
  filter->useDigitalGain = TRUE;
  filter->loadAndSaveConf = TRUE;
  filter->debug = FALSE;
  filter->threshold = 10;
  filter->minDigitalGain = 256;
  filter->maxDigitalGain = 4096;
  

  initialization(flux, 2);
}

static void
gst_autoexposure_set_property(GObject *object, guint prop_id,
                              const GValue *value, GParamSpec *pspec)
{
  Gstautoexposure *filter = GST_AUTOEXPOSURE(object);

  switch (prop_id)
  {
  case PROP_SILENT:
    filter->silent = g_value_get_boolean(value);
    break;
  case PROP_WORK:
    filter->work = g_value_get_boolean(value);
    break;
  case PROP_USEEXPOSITIONTIME:
    filter->useExpositionTime = g_value_get_boolean(value);
    break;
  case PROP_USEHISTOGRAM:
    filter->useHistogram = g_value_get_boolean(value);
    break;
  case PROP_OPTIMIZE:
    filter->optimize = g_value_get_int(value);
    break;
  case PROP_TARGET:
    filter->target = g_value_get_int(value);
    break;
  case PROP_THRESHOLD:
    filter->threshold = g_value_get_int(value);
    break;
  case PROP_USEDIGITALGAIN:
    filter->useDigitalGain = g_value_get_boolean(value);
    break;
  case PROP_MAXEXPOSITION:
    filter->maxExposition = g_value_get_int(value);
    break;
  case PROP_MAXANALOGGAIN:
    filter->maxAnalogGain = g_value_get_int(value);
    break;
  case PROP_LATENCY:
    filter->latency = g_value_get_int(value);
    break;
  case PROP_ROI1X:
    filter->ROI1x = g_value_get_int(value);
    break;
  case PROP_ROI1Y:
    filter->ROI1y = g_value_get_int(value);
    break;
  case PROP_ROI2X:
    filter->ROI2x = g_value_get_int(value);
    break;
  case PROP_ROI2Y:
    filter->ROI2y = g_value_get_int(value);
    break;
  case PROP_LOADANDSAVECONF:
    filter->loadAndSaveConf = g_value_get_boolean(value);
    break;
  case PROP_DEBUG:
    filter->debug = g_value_get_boolean(value);
    break;
  case PROP_MINDIGITALGAIN:
    filter->minDigitalGain = g_value_get_int(value);
    break;
  case PROP_MAXDIGITALGAIN:
    filter->maxDigitalGain = g_value_get_int(value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void
gst_autoexposure_get_property(GObject *object, guint prop_id,
                              GValue *value, GParamSpec *pspec)
{
  Gstautoexposure *filter = GST_AUTOEXPOSURE(object);

  switch (prop_id)
  {
  case PROP_SILENT:
    g_value_set_boolean(value, filter->silent);
    break;
  case PROP_WORK:
    g_value_set_boolean(value, filter->work);
    break;
  case PROP_USEEXPOSITIONTIME:
    g_value_set_boolean(value, filter->useExpositionTime);
    break;
  case PROP_USEDIGITALGAIN:
    g_value_set_boolean(value, filter->useDigitalGain);
    break;
  case PROP_USEHISTOGRAM:
    g_value_set_boolean(value, filter->useHistogram);
    break;
  case PROP_LOADANDSAVECONF:
    g_value_set_boolean(value, filter->loadAndSaveConf);
    break;
  case PROP_OPTIMIZE:
    g_value_set_int(value, filter->optimize);
    break;
  case PROP_TARGET:
    g_value_set_int(value, filter->target);
    break;
  case PROP_LATENCY:
    g_value_set_int(value, filter->latency);
    break;
  case PROP_MAXEXPOSITION:
    g_value_set_int(value, filter->maxExposition);
    break;
  case PROP_MAXANALOGGAIN:
    g_value_set_int(value, filter->maxAnalogGain);
    break;
  case PROP_ROI1X:
    g_value_set_int(value, filter->ROI1x);
    break;
  case PROP_ROI1Y:
    g_value_set_int(value, filter->ROI1y);
    break;
  case PROP_ROI2X:
    g_value_set_int(value, filter->ROI2x);
    break;
  case PROP_ROI2Y:
    g_value_set_int(value, filter->ROI2y);
    break;
  case PROP_DEBUG:
    g_value_set_boolean(value, filter->debug);
    break;
  case PROP_THRESHOLD:
    g_value_set_int(value, filter->threshold);
    break;
  case PROP_MINDIGITALGAIN:
    g_value_set_int(value, filter->minDigitalGain);
    break;
  case PROP_MAXDIGITALGAIN:
    g_value_set_int(value, filter->maxDigitalGain);
    break;
    /*
  case PROP_HISTOGRAM:
    g_value_set_pointer(value, filter->histogram);
    break;*/
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_autoexposure_sink_event(GstPad *pad, GstObject *parent, GstEvent *event)
{
  Gstautoexposure *filter;
  gboolean ret;

  filter = GST_AUTOEXPOSURE(parent);

  GST_LOG_OBJECT(filter, "Received %s event: %" GST_PTR_FORMAT,
                 GST_EVENT_TYPE_NAME(event), event);

  switch (GST_EVENT_TYPE(event))
  {
  case GST_EVENT_CAPS:
  {
    GstCaps *caps;

    gst_event_parse_caps(event, &caps);
    /* do something with the caps */

    /* and forward */
    ret = gst_pad_event_default(pad, parent, event);
    break;
  }
  default:
    ret = gst_pad_event_default(pad, parent, event);
    break;
  }

  return ret;
}

/* chain function
 * this function does the actual processing
 */
double valeur_moyenne(int histo[], int taille)
{
  long int somme = 0;
  int nb_elements = 0;

  for (int i = 0; i < taille; i++)
  {
    somme += (long int)histo[i] * i;
    nb_elements += histo[i];
  }

  if (nb_elements == 0)
  {
    return 0.0;
  }
  else
  {
    return (double)somme / nb_elements;
  }
}

static GstFlowReturn
gst_autoexposure_chain(GstPad *pad, GstObject *parent, GstBuffer *buf)
{

  GstMapInfo map;

  GstCaps *caps = gst_pad_get_current_caps(pad);
  GstStructure *s = gst_caps_get_structure(caps, 0);
  gboolean res;
  gint width, height;

  char dig_gain[] = "digital_gain";
  char ana_gain[] = "analog_gain";
  char expo[] = "exposure";
  Gstautoexposure *filter;

  filter = GST_AUTOEXPOSURE(parent);
  if (proc_once && filter->loadAndSaveConf)
  {
    int tmp_exp, tmp_analog, tmp_digital;
    if (read_conf(&tmp_exp, &tmp_analog, &tmp_digital))
    {
      int exp = get_control(expo);
      printf("Initial conf : exposure = %d analog_gain = %d digital_gain = %d\n", tmp_exp, tmp_analog, tmp_digital);

      printf("exp actual %d \n", exp);
      set_control(expo, tmp_exp);
      set_control(ana_gain, tmp_analog);
      set_control(dig_gain, tmp_digital);
      exp = get_control(ana_gain);
      printf("exp actual %d \n", exp);
    }
    proc_once = 0;
  }
  /* just push out the incoming buffer without touching it */


  gst_buffer_map(buf, &map, GST_MAP_READ);
  // we need to get the final caps on the buffer to get the size
  res = gst_structure_get_int(s, "width", &width);
  res |= gst_structure_get_int(s, "height", &height);
  if (!res)
  {
    g_print("could not get snapshot dimension\n");
    exit(-1);
  }
  if (filter->work)
  {
    if (!filter->useHistogram)
    {

      int tmp_mean;
      float global_mean = 0;
      for (int y = filter->ROI1y; y < filter->ROI2y; y += 1 + filter->optimize)
      {
        tmp_mean = 0;
        for (int x = filter->ROI1x; x < filter->ROI2x; x += 1 + filter->optimize)
        {
          tmp_mean += map.data[(y * width) + x];
        }
        global_mean += (tmp_mean * (1 + filter->optimize)) / ((float)filter->ROI2x - filter->ROI1x);
      }
      global_mean = (global_mean * (1 + filter->optimize)) / ((float)filter->ROI2y - filter->ROI1y);

      if (global_mean > filter->target - filter->threshold && global_mean < filter->target + filter->threshold)
      {
        if (converge == false)
        {


	  int exp = get_control(expo);
	  int an_gain = get_control(ana_gain);
	  int dg_gain = get_control(dig_gain);
          converge=true;
	  if(filter->debug)
	  {
          g_print("Frames to converge : %d   exposure : %d   analog_gain : %d   digital_gain : %d\n",frames_to_converge,exp,an_gain,dg_gain);
}
          frames_to_converge=0;

        }
      }
      else
      {
	converge=false;
        frames_to_converge++;
      }
      if (filter->useExpositionTime)
      {
        algorithm_with_exposition(global_mean, filter->latency, filter->target, filter->maxExposition, filter->maxAnalogGain, filter->useDigitalGain, filter->minDigitalGain, filter->maxDigitalGain, filter->threshold);
      }
      else
      {
        algorithm_without_exposition(global_mean, filter->latency, filter->target, filter->maxAnalogGain, filter->useDigitalGain, filter->minDigitalGain, filter->maxDigitalGain, filter->threshold);
      }
    } /*
     else
     {
       int hist[256];
       for (int i = 0; i < 256; i++)
       {
         hist[i] = 0;
       }

       for (int y = filter->ROI1y; y < filter->ROI2y; y += 1 + filter->optimize)
       {
         for (int x = filter->ROI1x; x < filter->ROI2x; x += 1 + filter->optimize)
         {
           hist[map.data[(y * width) + x]] += 1;
         }
       }

       int global_mean = valeur_moyenne(hist, 256);
       if (filter->useExpositionTime)
       {
         algorithm_with_exposition(global_mean, filter->latency,  filter->target,  filter->maxExposition, filter->maxAnalogGain, filter->useDigitalGain);
       }
       else
       {
         algorithm_without_exposition(global_mean, filter->latency, filter->target, filter->maxAnalogGain, filter->useDigitalGain);
       }
     }*/
  }

  gst_buffer_unmap(buf, &map);

  return gst_pad_push(filter->srcpad, buf);
}

static void gst_autoexposure_finalize(GObject *object)
{
  char dig_gain[] = "digital_gain";
  char ana_gain[] = "analog_gain";
  char expo[] = "exposure";
  Gstautoexposure *filter;

  filter = GST_AUTOEXPOSURE(object);
  if (filter->loadAndSaveConf)
  {
    int tmp_exp, tmp_analog, tmp_digital;

    tmp_exp = get_control(expo);
    tmp_analog = get_control(ana_gain);
    tmp_digital = get_control(dig_gain);
    write_conf(tmp_exp, tmp_analog, tmp_digital);
  }

  g_print("driver closed\n");
  close_driver_access();
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
autoexposure_init(GstPlugin *autoexposure)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template autoexposure' with your description
   */
  GST_DEBUG_CATEGORY_INIT(gst_autoexposure_debug, "autoexposure",
                          0, "Template autoexposure");

  return gst_element_register(autoexposure, "autoexposure", GST_RANK_NONE,
                              GST_TYPE_AUTOEXPOSURE);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "autoexposure"
#endif

/* gstreamer looks for this structure to register autoexposures
 *
 * exchange the string 'Template autoexposure' with your autoexposure description
 */
GST_PLUGIN_DEFINE(
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    autoexposure,
    "Template autoexposure",
    autoexposure_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/")
