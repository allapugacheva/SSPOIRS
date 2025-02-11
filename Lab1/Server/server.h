#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>
#include "../logger.h"
#include "../check_connection.h"
#include "load_info.h"
#include "../settings.h"

#include "../bnl/loading_line.h"
#include "../voice_logger/voice_logger.h"

#define LOG_FILE "server_log.txt"
#define BUFFER_SIZE 80

extern SETTINGS* settings;

void run();
int start_server(int* sfd);
int process_client(int* cfd);
void receive_data(int* cfd, const char* file);
void send_data(int* cfd, const char* file);
void echo();
void server_time();
void settings_command(char* command);

#endif
