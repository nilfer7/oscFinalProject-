//
// Created by nilfer on 12/29/25.
//

#ifndef OSCFINALPROJECT_SENSOR_DB_H
#define OSCFINALPROJECT_SENSOR_DB_H

#include "sbuffer.h"

/*
 * Reads from the buffer and writes to data.csv
 */
void storage_mgr_parse_sensor_data(sbuffer_t *shared_queue_ptr);

#endif //OSCFINALPROJECT_SENSOR_DB_H