#include "algorithm.h"
#include "math.h"


int algorithm_digital_gain(int target, float global_mean, int digital_gain)
{
	int new_digital_gain = digital_gain * (target/(float)global_mean); // digital gain we should set to get the targeted mean	
	if(new_digital_gain < 256) // 256 is select as min because if the digital gain is < 256 the image will be grayed out
	{
		new_digital_gain=256;
		set_control("digital_gain", 256);
		return 256 * (global_mean /(float) digital_gain); // expected value of global mean with the gain set to 256
	}
	else if(new_digital_gain > get_control_max("digital_gain")) // set digital_gain  at max
	{
		set_control("digital_gain", get_control_max("digital_gain"));
	}
	else
	{
		set_control("digital_gain", new_digital_gain);
	}
	return global_mean;
}

int algorithm_analog_gain(int target, float global_mean, int max_analog_gain)
{
	int analog_gain = get_control("analog_gain");
	int new_analog_gain = (log10(target) + 0.09 * analog_gain - log10(global_mean)) / 0.07; // analog gain we should set to get the targeted mean

	if(new_analog_gain < get_control_min("analog_gain"))// set analog gain to minimum and return expected mean (with a gain of 0)
	{
		new_analog_gain=get_control_min("analog_gain");
		set_control("analog_gain", new_analog_gain);
		return pow(10, 0.07 * new_analog_gain - 0.09 * analog_gain + log10(global_mean)); 
	}
	else if(new_analog_gain > max_analog_gain) // set analog gain to maximum and return expected mean (with max gain)
	{
		set_control("analog_gain", max_analog_gain);
		return pow(10, 0.07 * max_analog_gain - 0.09 * analog_gain + log10(global_mean));
	}
	else // set analog gain to the calculated gain (expect global mean to be close to target)
	{
		set_control("analog_gain", new_analog_gain);
	}
	return global_mean;
}

int algorithm_without_exposition(float global_mean, int latency, int target, int max_analog_gain, bool toggle_digital_gain)
{	
	int digital_gain = get_control("digital_gain");
int expected=global_mean;
	if (proc_once2 == 0)
	{
		if(target > global_mean) // if the targeted mean is greater than the global mean, we start to change the analogic gain (because he should produce less noise than digital gain) 
		{
			 expected = algorithm_analog_gain(target,global_mean,max_analog_gain); 

			if(expected != global_mean && toggle_digital_gain)
			{
				expected=algorithm_digital_gain(target,expected,digital_gain);
			}
		}
		else  // if the targeted mean is lower than the global mean, we start to change the digital gain (to minimize the noise)
		{
			if(toggle_digital_gain)
			{
				expected = algorithm_digital_gain(target,global_mean,digital_gain);
				if(expected != global_mean || !toggle_digital_gain)
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

	if (proc_once2 > latency && (global_mean > target + 5 || global_mean < target - 5)) // restart the autoexposition algorithm if the targeted mean is to far from the global min
	{
		proc_once2 = 0;
	}
	return expected;
}


int algorithm_exposition(int target, float global_mean, int max_exposition)
{
	int exposition = get_control("exposure");
	int new_exposition = exposition * (target/(float)global_mean); // exposition we should set to get the targeted mean

	if(new_exposition < get_control_min("exposure"))// set exposition to minimum and return expected mean (with a exposition of 0)
	{
		new_exposition=get_control_min("exposure");
		set_control("exposure", new_exposition);
		return new_exposition * (global_mean /(float) exposition); 
	}
	else if(new_exposition > max_exposition) // set exposition to maximum and return expected mean (with max exposition)
	{
		set_control("exposure", max_exposition);
		return max_exposition * (global_mean /(float) exposition); 
	}
	else // set exposition to the calculated exposition (expect global mean to be close to target)
	{
		set_control("exposure", new_exposition);
	}
	return global_mean;
}


void algorithm_with_exposition(float global_mean, int latency, int target, int max_exposition, int max_analog_gain, bool toggle_digital_gain)
{
	
	if (proc_Once == 0)
	{
		if(target > global_mean) // if the targeted mean is greater than the global mean, we start to change the analogic gain (because he should produce less noise than digital gain) 
		{
			int expected = algorithm_exposition(target,global_mean,max_exposition); 

			if(expected != global_mean)
			{
				expected=algorithm_without_exposition(expected,latency,target, max_analog_gain, toggle_digital_gain);
			}
		}
		else  // if the targeted mean is lower than the global mean, we start to change the digital gain (to minimize the noise)
		{
			int expected = algorithm_without_exposition(global_mean,latency,target, max_analog_gain, toggle_digital_gain);
			printf("expected deacresed : %d\n",expected);
			if(expected != global_mean)
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

	if (proc_Once > latency && (global_mean > target + 5 || global_mean < target - 5)) // restart the autoexposition algorithm if the targeted mean is to far from the global min
	{
		proc_Once = 0;
	}
}
/*
void algorithm_with_exposition2(float global_mean, int maxExp, int latency, int target)
{
	int exposition = get_control("exposition");
	float delta = maxExp / exposition;
	float x = 70 / global_mean;
	int gain = get_control("analog_gain");
	if (global_mean < target - 5 && (proc_Once == 0 || exposition != maxExp))
	{
		if (x < delta)
		{
			exposition = x * exposition;
			set_control("exposition", exposition);
		}
		else
		{
			set_control("exposition", maxExp);
			algorithm_without_exposition(delta * global_mean, latency, target );
		}
			proc_Once = 1;

	}
	else if (global_mean < target - 5 && proc_Once > latency)
	{
		algorithm_without_exposition(delta * global_mean, latency, target);
	}
	else if (global_mean > target + 5)
	{

		if (gain > 0)
		{
			algorithm_without_exposition(delta * global_mean, latency, target);
		}
		else if (proc_Once == 0)
		{
			set_control("exposition", exposition * x);
			proc_Once = 1;
		}
	}

	if (proc_Once > 0)
	{
		proc_Once++;
	}
	if (proc_Once > latency && gain == 0 && (global_mean > target + 5 || global_mean < target -5)) // restart the autoexposition algorithm if the targeted mean is to far from the global min
	{
		proc_Once = 0;
	}
}*/
