//
// Created by nilfer on 12/29/25.
//

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include "connmgr.h"
#include "datamgr.h"
#include "sensor_db.h"
#include "sbuffer.h"

int pipe_file_descriptors[2];
pthread_mutex_t pipe_write_lock = PTHREAD_MUTEX_INITIALIZER;

void run_logger_process() {
    close(pipe_file_descriptors[1]);

    FILE *log_output_file = fopen("gateway.log", "w");
    if (!log_output_file) exit(EXIT_FAILURE);

    FILE *pipe_stream = fdopen(pipe_file_descriptors[0], "r");
    char read_buffer[256];
    int log_sequence_number = 0;

    while (fgets(read_buffer, sizeof(read_buffer), pipe_stream) != NULL) {
        read_buffer[strcspn(read_buffer, "\n")] = 0;

        time_t current_time = time(NULL);
        char *timestamp_string = ctime(&current_time);
        timestamp_string[strcspn(timestamp_string, "\n")] = 0;

        fprintf(log_output_file, "%d %s %s\n", log_sequence_number++, timestamp_string, read_buffer);
        fflush(log_output_file);
    }

    fclose(pipe_stream);
    fclose(log_output_file);
    exit(EXIT_SUCCESS);
}

void *thread_entry_datamgr(void *arg) {
    sbuffer_t *buf = (sbuffer_t *)arg;
    FILE *map_fp = fopen("room_sensor.map", "r");
    if (!map_fp) exit(EXIT_FAILURE);

    datamgr_parse_sensor_files(map_fp, buf);
    fclose(map_fp);
    return NULL;
}

void *thread_entry_storagemgr(void *arg) {
    sbuffer_t *buf = (sbuffer_t *)arg;
    storage_mgr_parse_sensor_data(buf);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <port> <max_clients>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_port = atoi(argv[1]);
    int max_clients_limit = atoi(argv[2]);

    if (pipe(pipe_file_descriptors) == -1) exit(EXIT_FAILURE);

    pid_t process_id = fork();
    if (process_id == 0) {
        run_logger_process();
    }

    close(pipe_file_descriptors[0]);

    sbuffer_t *main_event_buffer;
    if (sbuffer_init(&main_event_buffer) != SBUFFER_SUCCESS) exit(EXIT_FAILURE);

    pthread_t thread_datamgr, thread_storagemgr;
    pthread_create(&thread_datamgr, NULL, thread_entry_datamgr, main_event_buffer);
    pthread_create(&thread_storagemgr, NULL, thread_entry_storagemgr, main_event_buffer);

    connmgr_listen(server_port, max_clients_limit, main_event_buffer);

    sensor_data_t shutdown_signal = { .id = 0, .value = 0, .ts = 0 };
    sbuffer_insert(main_event_buffer, &shutdown_signal);

    pthread_join(thread_datamgr, NULL);
    pthread_join(thread_storagemgr, NULL);

    sbuffer_free(&main_event_buffer);
    datamgr_free();

    close(pipe_file_descriptors[1]);
    wait(NULL);

    return 0;
}