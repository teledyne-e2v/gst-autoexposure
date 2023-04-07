#include "algorithm.h"
#include "math.h"


float algorithm_digital_gain(int target, float global_mean, int digital_gain)
{
	int new_digital_gain = digital_gain * (target/(float)global_mean); // digital gain we should set to get the targeted mean	

	if(new_digital_gain <= 256) // 256 is select as min because if the digital gain is < 256 the image will be grayed out
	{
		new_digital_gain=256;
		set_control("digital_gain", 256);
	}
	else if(new_digital_gain > get_control_max("digital_gain")) // set digital_gain  at max
	{
		new_digital_gain = get_control_max("digital_gain");
		set_control("digital_gain", get_control_max("digital_gain"));
	}
	else
	{
		set_control("digital_gain", new_digital_gain);
	}
	return global_mean*(((float)new_digital_gain)/digital_gain);
}

float algorithm_analog_gain(int target, float global_mean, int max_analog_gain)
{
	int analog_gain = get_control("analog_gain");
	//float new_analog_gain = (log10(target) + 0.07 * analog_gain - log10(global_mean)) / 0.07; // analog gain we should set to get the targeted mean
	float new_analog_gain = analog_gain+log(target/global_mean)/log(1.18);

	if(new_analog_gain < get_control_min("analog_gain"))// set analog gain to minimum and return expected mean (with a gain of 0)
	{
		new_analog_gain=get_control_min("analog_gain");
		set_control("analog_gain",(int) new_analog_gain);
	}
	else if(new_analog_gain > max_analog_gain) // set analog gain to maximum and return expected mean (with max gain)
	{
		new_analog_gain=get_control_max("analog_gain");
		set_control("analog_gain", max_analog_gain);
	}
	else // set analog gain to the calculated gain (expect global mean to be close to target)
	{
		set_control("analog_gain", (int)new_analog_gain);
	}
	return global_mean*pow(1.18,(int)new_analog_gain-analog_gain);
}

float algorithm_without_exposition(float global_mean, int latency, int target, int max_analog_gain, bool toggle_digital_gain, int tolerance)
{	
	int digital_gain = get_control("digital_gain");
float expected=global_mean;
	if (proc_once2 == 0)
	{
		if(target > global_mean) // if the targeted mean is greater than the global mean, we start to change the analogic gain (because he should produce less noise than digital gain) 
		{
			expected = algorithm_analog_gain(target,global_mean,max_analog_gain); 

			if((expected > target + tolerance || expected < target - tolerance) && toggle_digital_gain)
			{
				expected=algorithm_digital_gain(target,expected,digital_gain);
			}
		}
		else  // if the targeted mean is lower than the global mean, we start to change the digital gain (to minimize the noise)
		{
			if(toggle_digital_gain)
			{
				expected = algorithm_digital_gain(target,global_mean,digital_gain);
				if((expected > target + tolerance || expected < target - tolerance) || !toggle_digital_gain)
				{
					expected=algorithm_analog_gain(target,expected,max_analog_gain);
				}
			}
			else 
			{
				expected=algorithm_analog_gain(target,expected,max_analog_gain);
			}
			
		}
		proc_once2 = 1;
	}
	if (proc_once2 > 0)
	{
		proc_once2++;
	}

	if (proc_once2 > latency && (global_mean > target + tolerance || global_mean < target - tolerance)) // restart the autoexposition algorithm if the targeted mean is to far from the global min
	{
		proc_once2 = 0;
	}
	return expected;
}


float algorithm_exposition(int target, float global_mean, int max_exposition)
{
	int exposition = get_control("exposure");
	int new_exposition = exposition * (target/(float)global_mean); // exposition we should set to get the targeted mean

	if(new_exposition < get_control_min("exposure"))// set exposition to minimum and return expected mean (with a exposition of 0)
	{
		new_exposition=get_control_min("exposure");
		set_control("exposure", new_exposition);

	}
	else if(new_exposition > max_exposition) // set exposition to maximum and return expected mean (with max exposition)
	{
		new_exposition=max_exposition;
		set_control("exposure", new_exposition);

	}
	else // set exposition to the calculated exposition (expect global mean to be close to target)
	{
		set_control("exposure", new_exposition);
	}
	return new_exposition * (global_mean /(float) exposition); 
}


void algorithm_with_exposition(float global_mean, int latency, int target, int max_exposition, int max_analog_gain, bool toggle_digital_gain, int tolerance)
{
	
	if (proc_Once == 0)
	{	
		if(target > global_mean) // if the targeted mean is greater than the global mean, we start to change the analogic gain (because he should produce less noise than digital gain) 
		{
			float expected = algorithm_exposition(target,global_mean,max_exposition); 
			if((expected > target + tolerance || expected < target - tolerance))
			{	
				proc_once2=0;
				expected=algorithm_without_exposition(expected,latency,target, max_analog_gain, toggle_digital_gain);
			}
		}
		else  // if the targeted mean is lower than the global mean, we start to change the digital gain (to minimize the noise)
		{
			proc_once2=0;
			float expected = algorithm_without_exposition(global_mean,latency,target, max_analog_gain, toggle_digital_gain);
			if((expected > target + tolerance || expected < target - tolerance))
			{
				expected=algorithm_exposition(target,expected,max_exposition);
			}
		}
		proc_Once = 1;
	}
	if (proc_Once > 0)
	{
		proc_Once++;
	}

	if (proc_Once > latency && (global_mean > target + tolerance || global_mean < target - tolerance)) // restart the autoexposition algorithm if the targeted mean is to far from the global min
	{
		proc_Once = 0;
	}
}
