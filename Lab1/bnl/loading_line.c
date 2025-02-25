#include "loading_line.h"

LLINE* init_lline(double percent, double fileSize) {

    LLINE* lline = (LLINE*)malloc(sizeof(LLINE));
    lline->prcnt_char = L'#';
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

    wprintf(LEFT_BOUND);
    for (int i = 0; i < lline->crnt_amount_stages; i++)
        wprintf(L"%lc", lline->prcnt_char);

    int temp = lline->amount_stages - lline->crnt_amount_stages;
    MOVE_RIGHT(temp);
    wprintf(RIGHT_BOUND);
    wprintf(L"%7.2lf", lline->prev_percent);
    wprintf(L"%%");
    MOVE_LEFT(temp + 9);
}

int get_amount_stages_by_percent(LLINE* lline, double percent) {
    return (double)(percent / 100.0) * lline->amount_stages;
}

int refresh_lline(LLINE* lline, double new_percent, double time) {

    double diff = new_percent - lline->prev_percent;
    if (diff < lline->output_frequency)
        return 0;

    lline->crnt_amount_stages = get_amount_stages_by_percent(lline, new_percent);
    int temp = lline->crnt_amount_stages - lline->amount_inp_stages - 1;
    const wchar_t* crnt_color;
    for (int i = 0; i < temp + 1; i++) {
        lline->amount_inp_stages++;
        double crnt_percent = ((double)lline->amount_inp_stages / lline->amount_stages) * 100.0;
        if (crnt_percent > 67.0)
            crnt_color = CYAN;
        else if (crnt_percent > 34.0)
            crnt_color = MAGENTA;
        else
            crnt_color = GREEN;
        usleep(lline->duration);

        wprintf(L"%ls%lc%ls", crnt_color, lline->prcnt_char, RESET);

        fflush(stdout);
    }

    temp = lline->amount_stages - lline->amount_inp_stages;
    MOVE_RIGHT(temp + 1);
    wprintf(L"%7.2lf", new_percent);
    wprintf(L"%%");
    wchar_t* speedString = get_speed_stirng(get_speed(lline->fileSize, new_percent - lline->prev_percent, time));
    wprintf(L" %ls", speedString);
    temp += 8 + wcslen(speedString) + 2;
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

wchar_t* get_speed_stirng(double bytes) {

    wchar_t buf[100];
    double speed = bytes / 1024;
    if (bytes < 100)
        swprintf(buf, 100, L"%.2lf б/с", bytes);
    else if (speed < 100)
        swprintf(buf, 100, L"%.2lf Кб/с", speed);
    else if ((speed /= 1024) < 100)
        swprintf(buf, 100, L"%.2lf Мб/с", speed);
    else {
        speed /= 1024;
        swprintf(buf, 100, L"%.2lf Гб/с", speed);
    }

    return wcsdup(buf);
}

void free_lline(LLINE* lline) {
    
    wprintf(L"\r");
    for (int i = 0; i < lline->clear_size; i++)
        wprintf(L" ");

    free(lline);
}