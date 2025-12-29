//
// Created by nilfer on 12/29/25.
//

#ifndef OSCFINALPROJECT_SBUFFER_H
#define OSCFINALPROJECT_SBUFFER_H

#include "config.h"

#define SBUFFER_FAILURE -1
#define SBUFFER_SUCCESS 0
#define SBUFFER_NO_DATA 1


typedef enum {
    READER_TYPE_DATA_PROCESSOR = 0,
    READER_TYPE_STORAGE_WRITER = 1
} ReaderType;

typedef struct sbuffer sbuffer_t;

/* Allocates and initializes the shared queue */
int sbuffer_init(sbuffer_t **pointer_to_buffer);

/* Cleans the queue and frees memory */
int sbuffer_free(sbuffer_t **pointer_to_buffer);

/* * Removes the next item.
 * Returns SBUFFER_NO_DATA if the queue is empty AND the writer has finished
 */
int sbuffer_remove(sbuffer_t *buffer_instance, sensor_data_t *destination_data_container, ReaderType literal_reader_id);

/* Inserts a new measurement */
int sbuffer_insert(sbuffer_t *buffer_instance, sensor_data_t *incoming_data_packet);

#endif //OSCFINALPROJECT_SBUFFER_H