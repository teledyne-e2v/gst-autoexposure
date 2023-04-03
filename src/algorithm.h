#include "api.h"
#pragma once
#include <stdbool.h>

float algorithm_without_exposition(float global_mean, int latency, int target, int max_analog_gain, bool toggle_digital_gain);
void algorithm_with_exposition(float global_mean, int latency, int target, int max_exposition, int max_analog_gain, bool toggle_digital_gain);
static int proc_Once = 1;
static int proc_once2 = 1;
