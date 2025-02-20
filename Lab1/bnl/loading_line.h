#ifndef LOADING_LINE_H
#define LOADING_LINE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "color.h"

#define LEFT_BOUND   "["
#define RIGHT_BOUND  "]"

#define MOVE_LEFT(x)   printf("\033[%dD", x)
#define MOVE_RIGHT(x)  printf("\033[%dC", x)

typedef struct _LOADING_LINE {
    double prev_percent;
    char   prcnt_char;
    int    amount_stages;
    int    crnt_amount_stages;
    int    amount_inp_stages;
    int    duration;
    double output_frequency;
    int    clear_size;
    double fileSize;
} LLINE;

LLINE* init_lline                  (double percent, double fileSize);
void   show_lline                  (LLINE* lline);
int    get_amount_stages_by_percent(LLINE* lline, double percent);
int    refresh_lline               (LLINE* lline, double new_percent, double time);
char*  get_speed_stirng            (double bytes);
double get_speed                   (int fileSize, double percent_up, double time);
void   free_lline                  (LLINE* lline);

#endif