#include "api.h"
#pragma once

void algorithm_without_exposition(float global_mean);
void algorithm_without_exposition_v2(float global_mean, int latency, int lowerbound, int upperbound);
void algorithm_with_exposition(float global_mean, int minfps);
void algorithm_with_exposition_v2(float global_mean, int minfps);
void algorithm_with_exposition_v3(float global_mean, int minfps, int latency, int lowerbound, int upperbound);
static int proc_Once = 0;
static int proc_once2 = 0;
