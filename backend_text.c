
#include <stdlib.h>
#include <stdio.h>
#include <sys/param.h>
#include <string.h>
#include <sys/time.h>
#include <lo/lo.h>

#include "backend_text.h"
#include "command.h"

backend_text_options_t backend_text_options;

static FILE* output_file = 0;
static FILE* input_file = 0;

extern int num_msgs;
extern mapper_timetag_t start;

void text_defaults()
{
    memset(&backend_text_options, 0,
           sizeof(backend_text_options));
    backend_text_options.file_path = 0;
}

int text_start()
{
    mapper_timetag_now(&start);

    if (!backend_text_options.file_path) {
        printf("No output filename specified.\n");
        return 1;
    }

    output_file = fopen(backend_text_options.file_path, "a");

    if (!output_file) {
        printf("Error opening file `%s' for writing.\n",
               backend_text_options.file_path);
        return 1;
    }

    return 0;
}

void text_stop()
{
    if (output_file)
    {
        fclose(output_file);
        output_file = 0;
    }

    if (input_file)
    {
        fclose(input_file);
        input_file = 0;
    }
}

int text_poll()
{
    return 0;
}

/* TODO: Bundle messages together that happen in the same call to poll(). */
void text_write_value(mapper_signal sig, const void *v, mapper_timetag_t *tt)
{
    int i;
    char str[1024], *path = str;
    mapper_device dev = mapper_signal_device(sig);
    snprintf(path, 1024, "%s/%s", mapper_device_name(dev),
             mapper_signal_name(sig));
    char type = mapper_signal_type(sig);
    int length = mapper_signal_length(sig);

    lo_timetag now;
    lo_timetag_now(&now);

    if (!tt || !tt->sec)
        fprintf(output_file, "%u %u %s %c ", now.sec, now.frac, path, type);
    else
        fprintf(output_file, "%u %u %s %c ", tt->sec, tt->frac, path, type);

    if (type == 'i') {
        for (i = 0; i < length; i++)
            fprintf(output_file, " %d", ((int*)v)[i]);
    }
    else if (type == 'f') {
        for (i = 0; i < length; i++)
            fprintf(output_file, " %g", ((float*)v)[i]);
    }

    fprintf(output_file, "\n");
    fflush(output_file);

    printf("\rRecording: %f seconds, %d messages.",
           mapper_timetag_difference(*tt, start), num_msgs++);
    fflush(stdout);
}

int text_seek_start()
{
    if (!input_file)
    {
        if (!backend_text_options.file_path) {
            printf("No input filename specified.\n");
            return 1;
        }

        input_file = fopen(backend_text_options.file_path, "r");

        if (!input_file) {
            printf("Error opening file `%s' for reading.\n",
                   backend_text_options.file_path);
            return 1;
        }
    }

    fseek(input_file, 0, SEEK_SET);

    return 0;
}

int text_read(char **_path, lo_message *_m, lo_timetag *_tt)
{
    const char *delim = " \r\n";
    char str[1024];
    if (!fgets(str, 1024, input_file))
        return 1;

    char *t, *p, *s = strtok_r(str, delim, &p);
    lo_timetag tt;
    lo_message m = lo_message_new();
    char *path;

    if (s) { tt.sec = atoi(s); }
    s = strtok_r(0, delim, &p);
    if (s) { tt.frac = atoi(s); }
    s = strtok_r(0, delim, &p);
    if (s) { path = s; }
    s = strtok_r(0, delim, &p);
    if (s) { t = s; }

    while ((s = strtok_r(0, delim, &p))) {
        switch (*t) {
        case 'i':
            lo_message_add_int32(m, atoi(s));
            break;
        case 'f':
            lo_message_add_float(m, atof(s));
            break;
        case 's':
            lo_message_add_string(m, s);
            break;
        }
        if (*(t+1))
            t++;
    }

    _tt->sec = tt.sec;
    _tt->frac = tt.frac;

    *_m = m;
    *_path = strdup(path);

    return 0;
}
