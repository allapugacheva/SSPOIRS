#include "voice_logger.h"

void play_message(const char* path) {
    pid_t pid = fork();

    if (pid == 0) {
        freopen("/dev/null", "w", stdout);  
        freopen("/dev/null", "w", stderr); 

        execlp("mpg123", "mpg123", path, NULL);
        perror("execlp"); 
        exit(1);
    } 
}

void vl_server_die() {
    play_message(SERVER_DIE);
}

void vl_server_critical_error() {
    play_message(CRITICAL_ERROR);
}

void vl_input_message() {
    play_message(INPUT_MESSAGE);
}

void vl_sent_message_success() {
    play_message(SENT_MESSAGE_SUCCESS);
}

void vl_system_error() {
    play_message(SYSTEM_ERROR);
}

void vl_task_success() {
    play_message(TASK_SUCCESS);
}

void vl_server_start() {
    play_message(SERVER_START);
}