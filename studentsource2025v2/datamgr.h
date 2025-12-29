//
// Created by nilfer on 12/29/25.
//

#ifndef OSCFINALPROJECT_DATAMGR_H
#define OSCFINALPROJECT_DATAMGR_H

#include <stdio.h>
#include "sbuffer.h"

/*
 * Reads the room map to build the sensor registry, then enters the main processing lope to pull and analyze data from the shared queue.
 */
void datamgr_parse_sensor_files(FILE *map_file_handle, sbuffer_t *buffer_ptr);

/*
 * Frees the sensor registry list.
 */
void datamgr_free();

#endif //OSCFINALPROJECT_DATAMGR_H