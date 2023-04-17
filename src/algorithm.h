#include <topaz_api.h>
#pragma once
#include <stdbool.h>

float algorithm_without_exposition(float global_mean, int latency, int target, int max_analog_gain, bool toggle_digital_gain, int min_digital_gain, int max_digital_gain, int tolerance);
void algorithm_with_exposition(float global_mean, int latency, int target, int max_exposition, int max_analog_gain, bool toggle_digital_gain, int min_digital_gain, int max_digital_gain, int tolerance);
__attribute__((unused)) static int proc_Once = 1;

__attribute__((unused)) static int proc_once2 = 1;
