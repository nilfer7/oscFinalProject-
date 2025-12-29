//
// Created by nilfer on 12/29/25.
//

#ifndef OSCFINALPROJECT_CONNMGR_H
#define OSCFINALPROJECT_CONNMGR_H

#include "sbuffer.h"

/*
 * Starts the server on the chosen port.
 * Blocks until 'max_clients_allowed' have connected and disconnected.
 */
void connmgr_listen(int port_number, int max_clients_allowed, sbuffer_t *shared_queue_ptr);

/*
 * Cleanup resources used by connection manager
 */
void connmgr_free();

#endif //OSCFINALPROJECT_CONNMGR_H