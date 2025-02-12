#ifndef VOICE_LOGGER_H
#define VOICE_LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#define SERVER_DIE "../voice_logger/messages/die.mp3"
#define CRITICAL_ERROR "../voice_logger/messages/critical_error.mp3"
#define INPUT_MESSAGE "../voice_logger/messages/input_message.mp3"
#define SENT_MESSAGE_SUCCESS "../voice_logger/messages/sent_success.mp3"
#define SYSTEM_ERROR "../voice_logger/messages/system_error.mp3"
#define TASK_SUCCESS "../voice_logger/messages/task_success.mp3"
#define SERVER_START "../voice_logger/messages/server_start.mp3"

void play_message(const char* path);
void vl_server_die();
void vl_server_critical_error();
void vl_input_message();
void vl_sent_message_success();
void vl_system_error();
void vl_task_success();
void vl_server_start();

#endif