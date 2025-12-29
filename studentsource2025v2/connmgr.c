//
// Created by nilfer on 12/29/25.
//
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/select.h>
#include "connmgr.h"
#include "lib/tcpsock.h"
#include "config.h"


#ifndef TIMEOUT
#define TIMEOUT 5
#endif


extern int pipe_file_descriptors[2];
extern pthread_mutex_t pipe_write_lock;

typedef struct {
    tcpsock_t *client_socket_connection;
    sbuffer_t *pointer_to_shared_queue;
    int session_identifier;
} ActiveSession;


void *handler_for_client_session(void *thread_argument) {
    ActiveSession *current_session = (ActiveSession *)thread_argument;
    tcpsock_t *my_socket = current_session->client_socket_connection;
    sbuffer_t *my_queue = current_session->pointer_to_shared_queue;

    free(current_session);

    sensor_data_t received_packet;
    int bytes_to_read;
    int connection_is_active = 1;
    int first_packet_received = 0;
    sensor_id_t stored_sensor_id = 0;  // to store the sensor ID from first packet


    int socket_fd;
    if (tcp_get_sd(my_socket, &socket_fd) != TCP_NO_ERROR) {
        printf("Error getting socket descriptor\n");
        connection_is_active = 0;
    }

    fd_set read_file_descriptors;
    struct timeval timeout_timer;

    while (connection_is_active) {
        FD_ZERO(&read_file_descriptors);
        FD_SET(socket_fd, &read_file_descriptors);
        timeout_timer.tv_sec = TIMEOUT;
        timeout_timer.tv_usec = 0;

        int select_result = select(socket_fd + 1, &read_file_descriptors, NULL, NULL, &timeout_timer);

        if (select_result <= 0) {
            connection_is_active = 0;
            continue;
        }

        bytes_to_read = sizeof(received_packet.id);
        if (tcp_receive(my_socket, &received_packet.id, &bytes_to_read) != TCP_NO_ERROR) break;

        bytes_to_read = sizeof(received_packet.value);
        if (tcp_receive(my_socket, &received_packet.value, &bytes_to_read) != TCP_NO_ERROR) break;

        bytes_to_read = sizeof(received_packet.ts);
        if (tcp_receive(my_socket, &received_packet.ts, &bytes_to_read) != TCP_NO_ERROR) break;

        if (!first_packet_received) {
            stored_sensor_id = received_packet.id;  // to store the ID for later
            pthread_mutex_lock(&pipe_write_lock);
            dprintf(pipe_file_descriptors[1], "Sensor node %d has opened a new connection\n", stored_sensor_id);
            pthread_mutex_unlock(&pipe_write_lock);
            first_packet_received = 1;
        }

        sbuffer_insert(my_queue, &received_packet);
    }

    if (first_packet_received) {
        pthread_mutex_lock(&pipe_write_lock);
        dprintf(pipe_file_descriptors[1], "Sensor node %d has closed the connection\n", stored_sensor_id);
        pthread_mutex_unlock(&pipe_write_lock);
    }

    tcp_close(&my_socket);
    return NULL;
}

void connmgr_listen(int port_number, int max_clients_allowed, sbuffer_t *shared_queue_ptr) {
    tcpsock_t *main_listening_socket;
    tcpsock_t *incoming_client_socket;
    int clients_processed_count = 0;


    pthread_t *client_threads = malloc(max_clients_allowed * sizeof(pthread_t));
    if (client_threads == NULL) {
        exit(EXIT_FAILURE);
    }

    if (tcp_passive_open(&main_listening_socket, port_number) != TCP_NO_ERROR) {
        free(client_threads);
        exit(EXIT_FAILURE);
    }

    while (clients_processed_count < max_clients_allowed) {
        if (tcp_wait_for_connection(main_listening_socket, &incoming_client_socket) != TCP_NO_ERROR) {
            continue;
        }

        ActiveSession *new_session_info = malloc(sizeof(ActiveSession));
        new_session_info->client_socket_connection = incoming_client_socket;
        new_session_info->pointer_to_shared_queue = shared_queue_ptr;
        new_session_info->session_identifier = clients_processed_count;

        if (pthread_create(&client_threads[clients_processed_count], NULL, handler_for_client_session, new_session_info) != 0) {
            tcp_close(&incoming_client_socket);
            free(new_session_info);
        } else {
            clients_processed_count++;
        }
    }

    // to make sure all client threads to finish before closing
    for (int i = 0; i < clients_processed_count; i++) {
        pthread_join(client_threads[i], NULL);
    }

    free(client_threads);
    tcp_close(&main_listening_socket);
}

void connmgr_free() {
}