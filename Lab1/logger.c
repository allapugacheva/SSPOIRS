#include "logger.h"

void log_message(FILE* f, const char* level, const char* message) {

    time_t now = time(NULL);
    struct tm* timeInfo = localtime(&now);

    fprintf(f, "[%02d.%02d.%4d %2d:%2d:%2d] [%8s] [%s]\n", 
        timeInfo->tm_mday, timeInfo->tm_mon + 1, timeInfo->tm_year + 1900,
        timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec,
        level, message);
}

FILE* start_log(const char* fileName) {

    FILE* f = fopen(fileName, "w");
    if (f == NULL) {
        fclose(f);
        perror("Error opening log file.\n");
        exit(errno);
    }
    return f;
}