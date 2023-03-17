#include "api.h"
#pragma once

void algorithm_without_exposition(float global_mean, int latency, int target);
void algorithm_with_exposition(float global_mean, int minfps, int latency, int target);

static int proc_Once = 0;
static int proc_once2 = 0;
