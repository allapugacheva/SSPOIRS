#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#define LOG_INFO     "INFO"
#define LOG_ERROR    "ERROR"
#define LOG_CRITICAL "CRITICAL"

void log_message(FILE* f, const char* level, const char* message);
FILE* start_log(const char* fileName);

#endif