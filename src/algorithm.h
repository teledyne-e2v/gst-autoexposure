#include "api.h"
#pragma once

void algorithm_without_exposition(float global_mean);
void algorithm_without_exposition_v2(float global_mean,int latency);
void algorithm_with_exposition(float global_mean,int minfps);
void algorithm_with_exposition_v2(float global_mean, int minfps);
void algorithm_with_exposition_v3(float global_mean, int minfps, int latency);
static int proc_Once=0;
static int proc_once2=0;
