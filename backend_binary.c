
#include <stdlib.h>
#include <stdio.h>
#include <sys/param.h>
#include <string.h>
#include <sys/time.h>
#include <lo/lo.h>

#include "backend_binary.h"
#include "command.h"

backend_binary_options_t backend_binary_options;

static FILE* output_file = 0;

extern int num_msgs;
extern mapper_timetag_t start;

void binary_defaults()
{
    memset(&backend_binary_options, 0, sizeof(backend_binary_options));
    backend_binary_options.file_path = 0;
}

int binary_start()
{
    mapper_timetag_now(&start);

    if (!backend_binary_options.file_path) {
        printf("No output filename specified.\n");
        return 1;
    }

    output_file = fopen(backend_binary_options.file_path, "ab");

    if (!output_file) {
        printf("Error opening file `%s' for writing.\n",
               backend_binary_options.file_path);
        return 1;
    }

    return 0;
}

void binary_stop()
{
    if (output_file) {
        fclose(output_file);
        output_file = 0;
    }
}

int binary_poll()
{
    return 0;
}

/* TODO: Bundle messages together that happen in the same call to poll(). */
void binary_write_value(mapper_signal sig, const void *v, mapper_timetag_t *tt)
{
    char str[1024], *path = str;

    lo_timetag now;
    lo_timetag_now(&now);

    if (!tt || !tt->sec)
        fwrite(&now, sizeof(lo_timetag), 1, output_file);
    else
        fwrite(tt, sizeof(lo_timetag), 1, output_file);

    mapper_device dev = mapper_signal_device(sig);
    snprintf(path, 1024, "%s/%s", mapper_device_name(dev),
             mapper_signal_name(sig));
    int len = strlen(path), wrote=len, i;
    len = (len / 4 + 1) * 4;
    int wlen = lo_htoo32(len);
    fwrite(&wlen, 4, 1, output_file);
    fwrite(path, wrote, 1, output_file);
    while (wrote < len) {
        fwrite("", 1, 1, output_file);
        wrote ++;
    }

    char type = mapper_signal_type(sig);
    int length = mapper_signal_length(sig);

    fwrite(&type, 1, 1, output_file);
    wlen = lo_htoo32(length);
    fwrite(&wlen, 4, 1, output_file);

    if (type == 'i' || type == 'f') {
        for (i = 0; i < length; i++) {
            int wi = lo_htoo32(((uint32_t*)v)[i]);
            fwrite(&wi, 4, 1, output_file);
        }
    }

    fflush(output_file);

    printf("\rRecording: %f seconds, %d messages.",
           mapper_timetag_difference(*tt, start), num_msgs++);
    fflush(stdout);
}
