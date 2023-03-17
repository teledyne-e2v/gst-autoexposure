#include "algorithm.h"
#include "math.h"


int algorithm_digital_gain(int target, int global_mean, int digital_gain)
{
	int new_digital_gain = digital_gain * (target/(float)global_mean);
	if(new_digital_gain < 256) //256 is select as min because if the digital gain is < 256 the image will be grayed out
	{
		new_digital_gain=256;
		set_control("digital_gain", 256);
		return 256 * (global_mean /(float) digital_gain); // expected value of global mean with the gain set to 256
	}
	else if(new_digital_gain > get_control_max("digital_gain"))
	{
		set_control("digital_gain", get_control_max("digital_gain"));
	}
	else
	{
		set_control("digital_gain", new_digital_gain);
	}
	return 0;
}

int algorithm_analog_gain(int target, int global_mean, int analog_gain)
{
	int new_analog_gain = (log10(target) + 0.09 * analog_gain - log10(global_mean)) / 0.07;
	printf("analog : %d\n",new_analog_gain);
	if(new_analog_gain < get_control_min("analog_gain"))
	{
		new_analog_gain=get_control_min("analog_gain");
		set_control("analog_gain", new_analog_gain);
		return pow(10, 0.07 * new_analog_gain - 0.09 * analog_gain + log10(global_mean)); // expected value of global mean with the gain set to 256
	}
	else if(new_analog_gain > get_control_max("analog_gain"))
	{
		new_analog_gain = get_control_max("analog_gain");
		set_control("analog_gain", new_analog_gain);
		return pow(10, 0.07 * new_analog_gain - 0.09 * analog_gain + log10(global_mean));
	}
	else
	{
		set_control("analog_gain", new_analog_gain);
	}
	return 0;
}

void algorithm_without_exposition(int global_mean, int latency, int target)
{

	int analog_gain = get_control("analog_gain");
	int digital_gain = get_control("digital_gain");
	if (proc_once2 == 0)
	{
	printf(" target : %d, global : %d\n",target,global_mean);
		if(target > global_mean)
		{
			int expected = algorithm_analog_gain(target,global_mean,analog_gain);

			if(expected != 0)
			{
				algorithm_digital_gain(target,global_mean,digital_gain);
			}
		}
		else
		{
			int expected = algorithm_digital_gain(target,global_mean,digital_gain);
			if(expected != 0)
			{
				algorithm_analog_gain(target,global_mean,analog_gain);
			}
		}
		proc_once2 = 1;
	}
	if (proc_once2 > 0)
	{
		proc_once2++;
	}

	if (proc_once2 > latency && (global_mean > target + 5 || global_mean < target - 5))
	{
		proc_once2 = 0;
	}
}



void algorithm_with_exposition(float global_mean, int maxExp, int latency, int target)
{
	int exp1 = get_control("exposure");
	float delta = maxExp / exp1;
	float x = 70 / global_mean;
	int gain = get_control("analog_gain");
	if (global_mean < lowerbound && (proc_Once == 0 || exp1 != maxExp))
	{
		proc_Once = 1;
		printf("exp1 : %d maxExp: %d x: %f delta: %f\n", exp1, maxExp, x, delta);
		if (x < delta)
		{
			exp1 = x * exp1;
			set_control("exposure", exp1);
		}
		else
		{
			set_control("exposure", maxExp);
			algorithm_without_exposition(delta * global_mean, latency, (upperbound + lowerbound)/2 );
		}
	}
	else if (global_mean < lowerbound && proc_Once > latency)
	{
		algorithm_without_exposition(delta * global_mean, latency, (upperbound + lowerbound)/2 );
	}
	else if (global_mean > upperbound)
	{

		if (gain > 0)
		{
			algorithm_without_exposition(delta * global_mean, latency, (upperbound + lowerbound)/2 );
		}
		else if (proc_Once == 0)
		{
			set_control("exposure", exp1 * x);
			proc_Once = 1;
		}
	}

	if (proc_Once > 0)
	{
		proc_Once++;
	}
	if (proc_Once > latency && gain == 0 && (global_mean > upperbound || global_mean < lowerbound))
	{
		proc_Once = 0;
	}
}