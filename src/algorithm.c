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

void algorithm_without_exposition_v2(float global_mean, int latency,int lowerbound, int upperbound)
{
	int gain = get_control("analog_gain");
	int max_gain = get_control_max("analog_gain");
	int newGain;
	int calc = (log10(70) + 0.10 * gain - log10(global_mean)) / 0.10;
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

	if (proc_once2 > latency && (global_mean > 110 || global_mean < 50))
	{
		proc_once2 = 0;
	}
}

void algorithm_with_exposition_v3(float global_mean, int maxExp, int latency,int lowerbound, int upperbound)
{
	int exp1 = get_control("exposure");
	float delta = maxExp / exp1;
	float x = 70 / global_mean;
	int gain = get_control("analog_gain");
	if (global_mean < 50 && (proc_Once == 0 || exp1 != maxExp))
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
			algorithm_without_exposition_v2(delta * global_mean, latency,lowerbound,upperbound);
		}
	}
	else if (global_mean < 50 && proc_Once > latency)
	{
		algorithm_without_exposition_v2(global_mean, latency,lowerbound,upperbound);
	}
	else if (global_mean > 110)
	{

		if (gain > 0)
		{
			algorithm_without_exposition_v2(global_mean, latency,lowerbound,upperbound);
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
	if (proc_Once > latency && gain == 0 && (global_mean > 110 || global_mean < 50))
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
		printf("%f\n\n\n", exp1 * exp((float)(gain - 3) * 0.1662) * 2.5837);
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
