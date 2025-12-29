//
// Created by nilfer on 12/29/25.
//

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "sensor_db.h"
#include "config.h"

extern int pipe_file_descriptors[2];
extern pthread_mutex_t pipe_write_lock;

void storage_mgr_parse_sensor_data(sbuffer_t *shared_queue_ptr) {
    FILE *csv_output_file = fopen("data.csv", "w");
    if (csv_output_file == NULL) return;

    pthread_mutex_lock(&pipe_write_lock);
    dprintf(pipe_file_descriptors[1], "A new data.csv file has been created.\n");
    pthread_mutex_unlock(&pipe_write_lock);

    sensor_data_t data_container;

    while(1) {
        int result = sbuffer_remove(shared_queue_ptr, &data_container, READER_TYPE_STORAGE_WRITER);
        if (result == SBUFFER_NO_DATA) break;

        fprintf(csv_output_file, "%d,%.4f,%ld\n",
                data_container.id,
                data_container.value,
                (long)data_container.ts);
        fflush(csv_output_file);

        pthread_mutex_lock(&pipe_write_lock);
        dprintf(pipe_file_descriptors[1], "Data insertion from sensor %d succeeded.\n", data_container.id);
        pthread_mutex_unlock(&pipe_write_lock);
    }

    fclose(csv_output_file);

    pthread_mutex_lock(&pipe_write_lock);
    dprintf(pipe_file_descriptors[1], "The data.csv file has been closed.\n");
    pthread_mutex_unlock(&pipe_write_lock);
}