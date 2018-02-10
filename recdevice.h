
#ifndef __RECDEVICE_H__
#define __RECDEVICE_H__

#include <mapper/mapper.h>

int recdevice_start();
void recdevice_poll(int block_ms);
void recdevice_stop();

struct stringlist
{
    const char *string;
    struct stringlist *next;
};

extern struct stringlist *device_strings;
extern struct stringlist *signal_strings;
extern int n_device_strings;
extern int n_signal_strings;

int recdevice_add_device_string(const char *str);
int recdevice_add_signal_string(const char *str);

void recdevice_add_input(const char *devname, const char *signame,
                         char type, int length);

void db_device_handler(mapper_database db, mapper_device dev,
                       mapper_record_event event, const void *user);

void db_signal_handler(mapper_database db, mapper_signal sig,
                       mapper_record_event event, const void *user);

extern mapper_device recdev;

#endif // __RECDEVICE_H__
