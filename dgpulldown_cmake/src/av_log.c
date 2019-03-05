//
// Created by jaystevens on 12/1/2018.
//

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "av_log.h"

static int av_log_level = AV_LOG_INFO;

static const char *get_level_str(int level)
{
     switch (level) {
         case AV_LOG_QUIET:
             return "quiet";
         case AV_LOG_PANIC:
             return "panic";
         case AV_LOG_FATAL:
             return "fatal";
         case AV_LOG_ERROR:
             return "error";
         case AV_LOG_WARNING:
             return "warning";
         case AV_LOG_INFO:
             return "info";
         case AV_LOG_VERBOSE:
             return "verbose";
         case AV_LOG_DEBUG:
             return "debug";
         case AV_LOG_TRACE:
             return "trace";
         default:
             return "";
     }
}

static const char *get_level_prefix(int level)
{
    switch (level) {
        case AV_LOG_QUIET:
            return "";
        case AV_LOG_PANIC:
            return "PANIC  : ";
        case AV_LOG_FATAL:
            return "FATAL  : ";
        case AV_LOG_ERROR:
            return "ERROR  : ";
        case AV_LOG_WARNING:
            return "WARNING: ";
        case AV_LOG_INFO:
            return "INFO   : ";
        case AV_LOG_VERBOSE:
            return "VERBOSE: ";
        case AV_LOG_DEBUG:
            return "DEBUG  : ";
        case AV_LOG_TRACE:
            return "TRACE  : ";
        default:
            return "";
    }
}

int av_log_set_level_str(const char *level_str) {
    const char *level_name = NULL;
    int level = 100;
    int i;

    for (i = 56; i > -9; i -= 8) {

        level_name = get_level_str(i);
        //printf("level_name: %d, \"%s\"\n", i, level_name);

        if (strcmp(level_str, level_name) == 0) {
            level = i;
            if (level > AV_LOG_VERBOSE)
                printf("loglevel: %s\n", level_name);
            break;
        }
    }

    if (level == 100) {
        printf("Invalid loglevel \"%s\". Possible levels are numbers or:\n", level_str);
        printf("\"quiet\"\n");
        printf("\"panic\"\n");
        printf("\"fatal\"\n");
        printf("\"error\"\n");
        printf("\"warning\"\n");
        printf("\"info\"\n");
        printf("\"verbose\"\n");
        printf("\"debug\"\n");
        printf("\"trace\"\n");
        return 1;
    }

    av_log_set_level(level);
    return 0;
}

int av_log_get_level(void)
{
    return av_log_level;
}

void av_log_set_level(int level)
{
    av_log_level = level;
}

void av_log(int level, const char *fmt, ...)
{
    va_list vl;
    char line[1024];
    const char *level_prefix;

    if (level > av_log_level)
        return;

    level_prefix = get_level_prefix(level);

    va_start(vl, fmt);
    vsprintf(line, fmt, vl);
    if (av_log_level == AV_LOG_INFO ||av_log_level == AV_LOG_VERBOSE) {
        if (level == AV_LOG_INFO || level == AV_LOG_VERBOSE) {
            level_prefix = "";
        }
    }
    printf("%s%s", level_prefix, line);
    va_end(vl);
}

void av_log_newline(void)
{
    if (av_log_level >= AV_LOG_INFO)
        printf("\n");
}