//
// Created by nilfer on 12/29/25.
//

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "datamgr.h"
#include "lib/dplist.h"
#include "config.h"

#define AVG_WINDOW_SIZE 5


#ifndef SET_MIN_TEMP
#define SET_MIN_TEMP 10
#endif
#ifndef SET_MAX_TEMP
#define SET_MAX_TEMP 20
#endif

extern int pipe_file_descriptors[2];
extern pthread_mutex_t pipe_write_lock;

typedef struct {
    uint16_t room_identifier;
    uint16_t sensor_identifier;
    double temperature_history[AVG_WINDOW_SIZE];
    int history_write_index;
    int number_of_readings_recorded;
    double current_average_temp;
} SensorNodeState;

dplist_t *sensor_registry_list = NULL;

void *copy_sensor_state(void *original) {
    SensorNodeState *copy = malloc(sizeof(SensorNodeState));
    *copy = *(SensorNodeState *)original;
    return copy;
}


void delete_sensor_state(void **element) {
    free(*element);
    *element = NULL;
}

int match_sensor_id(void *x, void *y) {
    SensorNodeState *a = (SensorNodeState *)x;
    SensorNodeState *b = (SensorNodeState *)y;
    if (a->sensor_identifier == b->sensor_identifier) return 0;
    return 1;
}

void datamgr_parse_sensor_files(FILE *map_file_handle, sbuffer_t *buffer_ptr) {
    // oder: Copy, Free, Compare
    sensor_registry_list = dpl_create(copy_sensor_state, delete_sensor_state, match_sensor_id);

    uint16_t read_room, read_sensor;
    while(fscanf(map_file_handle, "%hu %hu", &read_room, &read_sensor) == 2) {
        SensorNodeState new_node = {0};
        new_node.room_identifier = read_room;
        new_node.sensor_identifier = read_sensor;
        dpl_insert_at_index(sensor_registry_list, &new_node, 0, true);
    }

    sensor_data_t current_packet;
    while(1) {
        int status = sbuffer_remove(buffer_ptr, &current_packet, READER_TYPE_DATA_PROCESSOR);
        if (status != SBUFFER_SUCCESS) break;
        if (current_packet.id == 0) break;

        SensorNodeState search_key;
        search_key.sensor_identifier = current_packet.id;

        int index = dpl_get_index_of_element(sensor_registry_list, &search_key);

        pthread_mutex_lock(&pipe_write_lock);
        if (index == -1) {
            dprintf(pipe_file_descriptors[1], "Received sensor data with invalid sensor node ID %d\n", current_packet.id);
        } else {
            SensorNodeState *live_state = (SensorNodeState *)dpl_get_element_at_index(sensor_registry_list, index);

            live_state->temperature_history[live_state->history_write_index] = current_packet.value;
            live_state->history_write_index = (live_state->history_write_index + 1) % AVG_WINDOW_SIZE;

            if (live_state->number_of_readings_recorded < AVG_WINDOW_SIZE) {
                live_state->number_of_readings_recorded++;
            }

            double sum = 0;
            for(int i = 0; i < live_state->number_of_readings_recorded; i++) {
                sum += live_state->temperature_history[i];
            }
            live_state->current_average_temp = sum / live_state->number_of_readings_recorded;

            if (live_state->current_average_temp < SET_MIN_TEMP) {
                dprintf(pipe_file_descriptors[1], "Sensor node %d reports it’s too cold (avg temp = %.2f)\n", live_state->sensor_identifier, live_state->current_average_temp);
            } else if (live_state->current_average_temp > SET_MAX_TEMP) {
                dprintf(pipe_file_descriptors[1], "Sensor node %d reports it’s too hot (avg temp = %.2f)\n", live_state->sensor_identifier, live_state->current_average_temp);
            }
        }
        pthread_mutex_unlock(&pipe_write_lock);
    }
}

void datamgr_free() {
    if (sensor_registry_list) {
        dpl_free(&sensor_registry_list, true);
        sensor_registry_list = NULL;
    }
}