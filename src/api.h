#pragma once
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>

#include <fcntl.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <libv4l2.h>
#define CLEAR(x) memset(&(x), 0, sizeof(x))
struct Control
{
    char name[64];
    int id;
    char type[32];
    int64_t minimum;
    int64_t maximum;
    int step;
    int64_t default_value;
};

struct Control_List
{
    struct Control controls[32];
    int number_of_controls;
};

struct buffer
{
    void *start;
    size_t length;
};

int64_t get_control_min(char *name);
int64_t get_control_max(char *name);
int64_t get_control_default(char *name);
struct Control_List *get_control_list();
int get_control(char *name);
void set_control(char *name, int value);
int get_control_by_code(int code);
void set_control_by_code(int code, int value);
void print_control_list();
void close_driver_access();
void initialization(char *v4l2_device, int sensor_mode);
