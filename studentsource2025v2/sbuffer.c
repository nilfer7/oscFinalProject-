//
// Created by nilfer on 12/29/25.
//
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "sbuffer.h"


typedef struct sbuffer_node {
    struct sbuffer_node *next_node_in_chain;
    sensor_data_t stored_sensor_payload;
} sbuffer_node_t;

struct sbuffer {
    sbuffer_node_t *first_node_head;
    sbuffer_node_t *last_node_tail;

    /* Literal cursor to follow the reader positions */
    sbuffer_node_t *cursor_for_data_mgr;
    sbuffer_node_t *cursor_for_storage_mgr;

    pthread_mutex_t queue_access_lock;
    pthread_cond_t new_data_signal;
};

int sbuffer_init(sbuffer_t **pointer_to_buffer) {
    if (pointer_to_buffer == NULL) return SBUFFER_FAILURE;

    *pointer_to_buffer = malloc(sizeof(sbuffer_t));
    if (*pointer_to_buffer == NULL) return SBUFFER_FAILURE;

    sbuffer_t *queue = *pointer_to_buffer;

    queue->first_node_head = NULL;
    queue->last_node_tail = NULL;
    queue->cursor_for_data_mgr = NULL;
    queue->cursor_for_storage_mgr = NULL;

    pthread_mutex_init(&queue->queue_access_lock, NULL);
    pthread_cond_init(&queue->new_data_signal, NULL);

    return SBUFFER_SUCCESS;
}

int sbuffer_free(sbuffer_t **pointer_to_buffer) {
    if (pointer_to_buffer == NULL || *pointer_to_buffer == NULL) return SBUFFER_FAILURE;

    sbuffer_t *queue = *pointer_to_buffer;

    pthread_mutex_lock(&queue->queue_access_lock);

    sbuffer_node_t *current_cleanup_node = queue->first_node_head;
    while (current_cleanup_node != NULL) {
        sbuffer_node_t *node_to_delete = current_cleanup_node;
        current_cleanup_node = current_cleanup_node->next_node_in_chain;
        free(node_to_delete);
    }

    pthread_mutex_unlock(&queue->queue_access_lock);

    pthread_mutex_destroy(&queue->queue_access_lock);
    pthread_cond_destroy(&queue->new_data_signal);

    free(queue);
    *pointer_to_buffer = NULL;
    return SBUFFER_SUCCESS;
}

int sbuffer_insert(sbuffer_t *buffer_instance, sensor_data_t *incoming_data_packet) {
    if (buffer_instance == NULL || incoming_data_packet == NULL) return SBUFFER_FAILURE;

    sbuffer_node_t *newly_created_node = malloc(sizeof(sbuffer_node_t));
    if (newly_created_node == NULL) return SBUFFER_FAILURE;

    newly_created_node->stored_sensor_payload = *incoming_data_packet;
    newly_created_node->next_node_in_chain = NULL;

    pthread_mutex_lock(&buffer_instance->queue_access_lock);

    if (buffer_instance->last_node_tail == NULL) {

        buffer_instance->first_node_head = newly_created_node;
        buffer_instance->last_node_tail = newly_created_node;


        buffer_instance->cursor_for_data_mgr = newly_created_node;
        buffer_instance->cursor_for_storage_mgr = newly_created_node;
    } else {

        buffer_instance->last_node_tail->next_node_in_chain = newly_created_node;
        buffer_instance->last_node_tail = newly_created_node;


        if (buffer_instance->cursor_for_data_mgr == NULL) {
            buffer_instance->cursor_for_data_mgr = newly_created_node;
        }
        if (buffer_instance->cursor_for_storage_mgr == NULL) {
            buffer_instance->cursor_for_storage_mgr = newly_created_node;
        }
    }

    pthread_cond_broadcast(&buffer_instance->new_data_signal);
    pthread_mutex_unlock(&buffer_instance->queue_access_lock);

    return SBUFFER_SUCCESS;
}

int sbuffer_remove(sbuffer_t *buffer_instance, sensor_data_t *destination_data_container, ReaderType literal_reader_id) {
    if (buffer_instance == NULL || destination_data_container == NULL) return SBUFFER_FAILURE;

    pthread_mutex_lock(&buffer_instance->queue_access_lock);


    sbuffer_node_t **active_reader_cursor;
    if (literal_reader_id == READER_TYPE_DATA_PROCESSOR) {
        active_reader_cursor = &buffer_instance->cursor_for_data_mgr;
    } else {
        active_reader_cursor = &buffer_instance->cursor_for_storage_mgr;
    }

    /* Waits for data */
    while (*active_reader_cursor == NULL) {
        pthread_cond_wait(&buffer_instance->new_data_signal, &buffer_instance->queue_access_lock);
    }

    /* Copies data */
    *destination_data_container = (*active_reader_cursor)->stored_sensor_payload;

    /* Stops if id=0 is found */
    if (destination_data_container->id == 0) {
        pthread_mutex_unlock(&buffer_instance->queue_access_lock);
        return SBUFFER_NO_DATA;
    }


    *active_reader_cursor = (*active_reader_cursor)->next_node_in_chain;


    while (buffer_instance->first_node_head != NULL) {
        int data_mgr_needs_head = (buffer_instance->cursor_for_data_mgr == buffer_instance->first_node_head);
        int storage_mgr_needs_head = (buffer_instance->cursor_for_storage_mgr == buffer_instance->first_node_head);

        if (!data_mgr_needs_head && !storage_mgr_needs_head) {
            sbuffer_node_t *trash_node = buffer_instance->first_node_head;
            buffer_instance->first_node_head = buffer_instance->first_node_head->next_node_in_chain;

            if (buffer_instance->first_node_head == NULL) {
                buffer_instance->last_node_tail = NULL;
            }
            free(trash_node);
        } else {
            break;
        }
    }

    pthread_mutex_unlock(&buffer_instance->queue_access_lock);
    return SBUFFER_SUCCESS;
}