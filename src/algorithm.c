#include "algorithm.h"

void algorithm_without_exposition(float global_mean)
{
    if(global_mean<50)
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
else if(global_mean>100)
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

void algorithm_with_exposition(float global_mean, int minfps)
{

    if(global_mean<50)
{
    int exp = get_control("exposure");
    int gain = get_control("analog_gain");
    int gain_d=get_control("digital_gain");
    if(1000000/((float)exp)>minfps && get_control_max("exposure")!=exp)
    {
        set_control("exposure",gain_d+1000);
    }
    else if(gain!= get_control_max("analog_gain"))
	{
        set_control("analog_gain",gain+1);
	}
	else
	{
		set_control("digital_gain",gain_d+20);
	}
	
}
else if(global_mean>100)
{	
	int gain=get_control("digital_gain");
    int exp = get_control("exposure");
    int gain_a=get_control("analog_gain");
	if(gain != get_control_min("digital_gain"))
	{
        set_control("digital_gain",gain-20);
	}
	else if(gain_a!= get_control_min("analog_gain"))
	{
		set_control("analog_gain",gain_a-1);
	}
    else
    {
        set_control("exposure",exp-1000);
    }
}
}