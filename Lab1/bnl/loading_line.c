#include "loading_line.h"

LLINE* init_lline(double percent, double fileSize) {
    LLINE* lline = (LLINE*)malloc(sizeof(LLINE));
    lline->prcnt_char = '#';
    lline->prev_percent = percent;
    lline->amount_stages = 20;
    lline->amount_inp_stages = lline->crnt_amount_stages = get_amount_stages_by_percent(lline, percent);
    lline->duration = 0;
    lline->output_frequency = 1.0;
    lline->clear_size = 0;
    lline->fileSize = fileSize;
    return lline;
}

void show_lline(LLINE* lline) {
    printf(LEFT_BOUND);
    for(int i = 0; i < lline->crnt_amount_stages; i++) {
        printf("%c", lline->prcnt_char);
    }

    int temp = lline->amount_stages - lline->crnt_amount_stages;
    MOVE_RIGHT(temp);
    printf(RIGHT_BOUND);
    printf("%7.2lf", lline->prev_percent);
    printf("%%");
    MOVE_LEFT(temp + 9);
}

int get_amount_stages_by_percent(LLINE* lline, double percent) {
    return (double)(percent / 100.0) * lline->amount_stages;
}

int refresh_lline(LLINE* lline, double new_percent, double time) {
    double diff = new_percent - lline->prev_percent;
    if(diff < lline->output_frequency) {
        return 0;
    }

    lline->crnt_amount_stages = get_amount_stages_by_percent(lline, new_percent);
    int temp = lline->crnt_amount_stages - lline->amount_inp_stages - 1;
    const char* crnt_color;
    for(int i = 0; i < temp + 1; i++) {
        lline->amount_inp_stages++;
        double crnt_percent = ((double)lline->amount_inp_stages / lline->amount_stages) * 100.0;
        if(crnt_percent > 67.0) {
            crnt_color = GREEN;
        } else if (crnt_percent > 34.0) {
            crnt_color = BOLD_YELLOW;
        } else {
            crnt_color = RED;
        }
        usleep(lline->duration);

        printf("%s%c%s", crnt_color, lline->prcnt_char, RESET);

        fflush(stdout);
    }

    temp = lline->amount_stages - lline->amount_inp_stages;
    MOVE_RIGHT(temp + 1);
    printf("%7.2lf", new_percent);
    printf("%%");
    char* speedString = get_speed_stirng(get_speed(lline->fileSize, new_percent - lline->prev_percent, time));
    printf(" %s", speedString);
    temp += 8 + strlen(speedString) + 2;
    MOVE_LEFT(temp);
    fflush(stdout);
    lline->prev_percent = new_percent;

    lline->clear_size = 3 + lline->amount_stages + temp;

    free(speedString);

    return 1;
}

double get_speed(int fileSize, double percent_up, double time) {
    return (percent_up * fileSize) / time / 100.0;
}

char* get_speed_stirng(double bytes) {
    char buf[100];
    double speed = bytes / 1024;
    if(bytes < 100) {
        sprintf(buf, "%.2lf/Bs", bytes);
    } else if(speed < 100) {
        sprintf(buf, "%.2lf/KBs", speed);
    } else if((speed /= 1024) < 100) {
        sprintf(buf, "%.2lf/MBs", speed);
    } else {
        speed /= 1024;
        sprintf(buf, "%.2lf/GBs", speed);
    }

    return strdup(buf);
}

void free_lline(LLINE* lline) {
    printf("\r");
    for(int i = 0; i < lline->clear_size; i++) {
        printf(" ");
    }

    free(lline);
}