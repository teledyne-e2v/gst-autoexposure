# gst-autoexposure
Autoexposure gstreamer plugin for OPTIMOM module

# Version 1.1

# About

This plugin implement an autoexposure algorithm. (using global mean histogram will be add in the future), all histogram parameters are currently useless.

the algorithm always favors the use of the exposure time then the analog gain and finally the digital gain to reduce the noise as much as possible, look at the parameters if you want to apply additional constraints.

# Dependencies

The following libraries are required for this plugin.
- v4l-utils
- libv4l-dev
- libgstreamer1.0-dev
- libgstreamer-plugins-base1.0-dev
- gcc
- meson (>= 0.49)
- ninja
- gstreamer-1.0


### Debian based system (Jetson): 

```
sudo apt install v4l-utils libv4l-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
```
Meson >= 0.49 is required, you can download the good version on the official debian repositories :
https://packages.debian.org/buster/all/meson/download.

Once you have download your package, you can install it with the command : 
```
sudo apt install ./meson_0.49.2-1_all.deb
```

This should install the ninja package, if not use the command : 
```
sudo apt install ninja
```

### Yocto based system (IMX): 

Teledyne provide a bbappend file which provides all packages needed :
https://github.com/teledyne-e2v/Yocto-files

##### Note : You can also compile them on your installed distribution but it will take a long time to compile (Do it only if you miss one or two packages)


# Compilation

## Ubuntu (Jetson)
First you must make sure that your device's clock is correctly setup.
Otherwise the compilation will fail.

In the **gst-autoexposure** folder do:

```
meson build
```
```
ninja -C build
```
```
sudo ninja -C build install
```

## Yocto (IMX)
First you must make sure that your device's clock is correctly setup.
Otherwise the compilation will fail.

In the **gst-autoexposure** folder do:

```
meson build
```
```
ninja -C build install
```

# Installation test
To test if the plugin has been correctly install, do:

	export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0/
	gst-inspect-1.0 autoexposure

If the plugin failed to install the following message will be displayed: ```No such element or plugin 'autoexposure'```

# Uninstall

	sudo rm /usr/local/lib/gstreamer-1.0/libgstautoexposure.*

# Usage

By default the plugin is installed in ```/usr/local/lib/gstreamer-1.0```. 
It is then required to tell gstreamer where to find it with the command:

	export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0/

The plugin can be used in any gstreamer pipeline by adding ```autoexposure```, the name of the plugin.


## Pipeline examples:

### Without NVIDIA plugins

Simple autoexposure using default parameters:

	gst-launch-1.0 v4l2src ! autoexposure ! queue ! videoconvert ! queue ! xvimagesink sync=false

Autoexposure using only gains and using optimization to reduce CPU load:

	gst-launch-1.0 v4l2src ! autoexposure useExpositionTime=false optimize=2 ! queue ! videoconvert ! queue ! xvimagesink sync=false

Autoexposure with limited usage of gain to limit noise:

	gst-launch-1.0 v4l2src ! autoexposure maxAnalogGain=8 useDigitalGain=false maxExposition=200000 ! queue ! videoconvert ! queue ! xvimagesink sync=false

### With NVIDIA plugins

Note : You should have update the nvvidconv plugin to support GRAY8, if not the image will be grayed out.

Simple autoexposure using default parameters:

	gst-launch-1.0 v4l2src ! 'video/x-raw,width=1920,height=1080,format=GRAY8' ! autoexposure ! nvvidconv ! 'video/x-raw(memory:NVMM),format=I420' ! nv3dsink sync=0
	
Autoexposure using only gains and using optimization to reduce CPU load:

	gst-launch-1.0 v4l2src ! 'video/x-raw,width=1920,height=1080,format=GRAY8' ! autoexposure useExpositionTime=false optimize=2 ! nvvidconv ! 'video/x-raw(memory:NVMM),format=I420' ! nv3dsink sync=0

Autoexposure with limited usage of gain to limit noise:

	gst-launch-1.0 v4l2src ! 'video/x-raw,width=1920,height=1080,format=GRAY8' ! autoexposure maxAnalogGain=8 useDigitalGain=false ! nvvidconv ! 'video/x-raw(memory:NVMM),format=I420' ! nv3dsink sync=0

# Plugin parameters

-  silent              : Produce verbose output
	- flags: readable, writable
	- Boolean. 
	- Default: false

-  work                : enable/disable autoexposure (usefull only for applications)
	- flags: readable, writable
	- Boolean. 
	- Default: true

-  optimize            : Optimization level, used to reduce CPU load
	- flags: readable, writable
	- Integer. 
	- Range: 0 - 5 
	- Default: 1 

-  maxExposition       : Maximum exposition tolerate
	- flags: readable, writable
	- Integer. 
	- Range: 5 - 200000 
	- Default: 20000 

-  maxAnalogGain       : Maximum analog gain tolerate
	- flags: readable, writable
	- Integer. 
	- Range: 0 - 15 
	- Default: 15 

-  useDigitalGain      : Enable/disable digital gain usage
	- flags: readable, writable
	- Boolean. 
	- Default: true

-  useExpositionTime   : enable/disable exposition time usage
	- flags: readable, writable
	- Boolean. 
	- Default: true

-  latency             : Pipeline latency, Really important, if the image is flickering, this is the most probable cause
	- flags: readable, writable
	- Integer. 
	- Range: 0 - 100 
	- Default: 4 

-  target              : Targeted mean of the image, an higher the target will produce brighter the image
	- flags: readable, writable
	- Integer.
	- Range: 0 - 255 
	- Default: 60 

-  roi1x               : Roi coordinates
	- flags: readable, writable
	- Integer. 
	- Range: 0 - 1920 
	- Default: 0 

-  roi1y               : Roi coordinates
	- flags: readable, writable
	- Integer. 
	- Range: 0 - 1080 
	- Default: 0 

-  roi2x               : Roi coordinates
	- flags: readable, writable
	- Integer. 
	- Range: 0 - 1920 
	- Default: 1920 

-  roi2y               : Roi coordinates
	- flags: readable, writable
	- Integer. 
	- Range: 0 - 1080 
	- Default: 1080 

-  useHistogram        : not implemented yet, please do not use
	- flags: readable, writable
	- Boolean. 
	- Default: false

-  loadAndSaveConf     : Load and save the exposure / gain parameters and load them when starting the plugin
	- flags: readable, writable
	- Boolean. 
	- Default: true
