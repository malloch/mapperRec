
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mapper/mapper.h>

#include "backend.h"
#include "recdevice.h"
#include "mapperRec.h"

struct stringlist *device_strings = 0;
struct stringlist *signal_strings = 0;
int n_device_strings = 0;
int n_signal_strings = 0;

mapper_device recdev;
mapper_database db;
int ready = 0;

int recdevice_start()
{
    recdev = mapper_device_new("mapperRec", 0, 0);
    return recdev == 0;
}

void recdevice_poll(int block_ms)
{
    mapper_device_poll(recdev, block_ms);
    if (!ready && mapper_device_ready(recdev)) {
        ready = 1;
        // subscribe to information about devices on the network
        db = mapper_database_new(mapper_device_network(recdev),
                                 MAPPER_OBJ_DEVICES);
        mapper_database_add_device_callback(db, db_device_handler, 0);
        mapper_database_add_signal_callback(db, db_signal_handler, 0);
        printf("looking for devices named: \n");
        struct stringlist **node = &device_strings;
        while (*node) {
            printf("'%s', ", (*node)->string);
            node = &(*node)->next;
        }
        printf("\b\b\n");
    }
}

void recdevice_stop()
{
    if (recdev)
        mapper_device_free(recdev);
}

void input_handler(mapper_signal sig, mapper_id instance, const void *value,
                   int count, mapper_timetag_t *timetag)
{
    if (value)
        backend_write_value(sig, value, timetag);
}

void db_device_handler(mapper_database db, mapper_device dev,
                       mapper_record_event event, const void *user)
{
    if (mapper_device_is_local(dev))
        return;

    if (event == MAPPER_ADDED) {
        // check if device matches device name argument, if so, get signals
        struct stringlist **node = &device_strings;
        const char *dev_name = mapper_device_name(dev);
        printf("Found device named '%s'", dev_name);
        while (*node) {
            if (strstr(dev_name, (*node)->string) != 0) {
                printf(" - subscribing to device signals...");
                mapper_database_subscribe(db, dev, MAPPER_OBJ_SIGNALS, -1);
                break;
            }
            node = &(*node)->next;
        }
        printf("\n");
    }
}

void add_sig_and_map(mapper_signal sig)
{
    char fullname[256];
    snprintf(fullname, 256, "%s/%s",
             mapper_device_name(mapper_signal_device(sig)),
             mapper_signal_name(sig));
    mapper_signal recsig = mapper_device_add_input_signal(recdev, fullname,
                                                          mapper_signal_length(sig),
                                                          mapper_signal_type(sig),
                                                          0, 0, 0,
                                                          input_handler, 0);
    if (!sig)
        return;
    printf("Connecting to signal '%s'\n", fullname);
    mapper_map map = mapper_map_new(1, &sig, 1, &recsig);
    mapper_map_push(map);
}

void db_signal_handler(mapper_database db, mapper_signal sig,
                       mapper_record_event event, const void *user)
{
    if (mapper_signal_is_local(sig))
        return;

    if (event == MAPPER_ADDED) {
        struct stringlist **signode = &signal_strings;
        if (!*signode) {
            add_sig_and_map(sig);
            return;
        }
        while (*signode) {
            if (strcmp(mapper_signal_name(sig), (*signode)->string) == 0) {
                add_sig_and_map(sig);
                return;
            }
            signode = &(*signode)->next;
        }
    }
}

int recdevice_add_device_string(const char *str)
{
    struct stringlist **node = &device_strings;
    struct stringlist *prevnode = device_strings;
    *node = malloc(sizeof(struct stringlist));
    (*node)->string = strdup(str);
    (*node)->next = prevnode;
    return ++n_device_strings;
}

int recdevice_add_signal_string(const char *str)
{
    struct stringlist **node = &signal_strings;
    struct stringlist *prevnode = signal_strings;
    *node = malloc(sizeof(struct stringlist));
    (*node)->string = strdup(str);
    (*node)->next = prevnode;
    return ++n_signal_strings;
}
