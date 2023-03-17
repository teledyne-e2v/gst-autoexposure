#include "algorithm.h"
#include "math.h"
void algorithm_without_exposition(float global_mean)
{

	if (global_mean < 50)
	{
		int gain = get_control("analog_gain");
		if (gain == get_control_max("analog_gain"))
		{
			int gain_d = get_control("digital_gain");
			set_control("digital_gain", gain_d + 20);
		}
		else
		{
			set_control("analog_gain", gain + 1);
		}
	}
	else if (global_mean > 110)
	{

		int gain = get_control("digital_gain");
		if (gain == get_control_default("digital_gain"))
		{

			int gain_a = get_control("analog_gain");
			set_control("analog_gain", gain_a - 1);
		}
		else
		{
			set_control("digital_gain", gain - 20);
		}
	}
}

void algorithm_without_exposition_v2(float global_mean, int latency, int lowerbound, int upperbound)
{
	int gain = get_control("analog_gain");
	int max_gain = get_control_max("analog_gain");
	int newGain;
	int interbound = (upperbound + lowerbound) / 2;
	int calc = (log10(interbound) + 0.09 * gain - log10(global_mean)) / 0.07;
	printf(" calc : %d\nproc = %d\n", calc, proc_once2);
	if (proc_once2 == 0)
	{
		printf("im ininnn\n");
		if (calc < max_gain)
		{
			newGain = calc;
		}
		else
		{
			newGain = max_gain;
		}
		set_control("analog_gain", newGain);
		proc_once2 = 1;
	}
	if (proc_once2 > 0)
	{
		proc_once2++;
	}

	if (proc_once2 > latency && (global_mean > upperbound || global_mean < lowerbound))
	{
		proc_once2 = 0;
	}
}



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

void algorithm_without_exposition_v3(int global_mean, int latency, int target)
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



void algorithm_with_exposition_v3(float global_mean, int maxExp, int latency, int lowerbound, int upperbound)
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
			algorithm_without_exposition_v3(delta * global_mean, latency, (upperbound + lowerbound)/2 );
		}
	}
	else if (global_mean < lowerbound && proc_Once > latency)
	{
		algorithm_without_exposition_v3(delta * global_mean, latency, (upperbound + lowerbound)/2 );
	}
	else if (global_mean > upperbound)
	{

		if (gain > 0)
		{
			algorithm_without_exposition_v3(delta * global_mean, latency, (upperbound + lowerbound)/2 );
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

void algorithm_with_exposition_v2(float global_mean, int maxExp)
{

	if (global_mean < 50)
	{
		int gain = get_control("analog_gain");
		if (gain == get_control_max("analog_gain"))
		{
			int gain_d = get_control("digital_gain");
			set_control("digital_gain", gain_d + 20);
		}
		else
		{
			set_control("analog_gain", gain + 1);
		}
	}
	else if (global_mean > 110)
	{
		int gain = get_control("digital_gain");
		int exp1 = get_control("exposure");
		int gain_a = get_control("analog_gain");
		if (gain != get_control_default("digital_gain"))
		{
			set_control("digital_gain", gain - 20);
		}
		else if (gain_a != get_control_min("analog_gain"))
		{
			set_control("analog_gain", gain_a - 1);
		}
		else
		{
			set_control("exposure", exp1 - 1000);
		}
	}
	else if (proc_Once == 0)
	{
		proc_Once = 1;
		int exp1 = get_control("exposure");
		int exp2 = maxExp;
		int delta = exp2 - exp1;
		int gain = get_control("analog_gain");
		int newExp;
		int newGain;
		printf("%f\n", exp1 * exp((float)(gain - 3) * 0.1662) * 2.5837);
		if (exp1 * exp((float)(gain - 3) * 0.1662) * 2.5837 > delta)
		{

			newExp = exp2;
			newGain = gain - 3 - log(delta / (2.5837 * exp1)) / 0.1662; //(float(gain)*0.1662)
		}
		else
		{
			newExp = exp1 * exp((float)(gain - 3) * 0.1662) * 2.5837 - exp1;
			newGain = 0;
		}
		set_control("exposure", newExp);
		set_control("analog_gain", newGain - 1);
	}

	if (proc_Once != 0)
	{
		proc_Once += 1;
	}

	if (proc_Once >= 10 && (global_mean < 50 /*|| global_mean>110*/))
	{
		proc_Once = 0;
	}
}

void algorithm_with_exposition(float global_mean, int maxExp)
{

	if (global_mean < 50)
	{
		int exp1 = get_control("exposure");
		int gain = get_control("analog_gain");
		int gain_d = get_control("digital_gain");
		if (exp1 > maxExp && get_control_max("exposure") != exp1)
		{
			printf("exp %d\n", exp1);
			set_control("exposure", exp1 + 5000);
		}
		else if (gain != get_control_max("analog_gain"))
		{
			set_control("analog_gain", gain + 1);
		}
		else
		{
			set_control("digital_gain", gain_d + 20);
		}
	}
	else if (global_mean > 110)
	{
		int gain = get_control("digital_gain");
		int exp1 = get_control("exposure");
		int gain_a = get_control("analog_gain");
		if (gain != get_control_default("digital_gain"))
		{
			set_control("digital_gain", gain - 20);
		}
		else if (gain_a != get_control_min("analog_gain"))
		{
			set_control("analog_gain", gain_a - 1);
		}
		else
		{
			set_control("exposure", exp1 - 1000);
		}
	}
}
