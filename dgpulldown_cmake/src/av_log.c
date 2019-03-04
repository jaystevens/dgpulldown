//
// Created by jaystevens on 12/1/2018.
//

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "av_log.h"

static int av_log_level = AV_LOG_INFO;

/*
//#if defined(_WIN32) && HAVE_SETCONSOLETEXTATTRIBUTE
#if defined(_WIN32)
#include <windows.h>
static const uint8_t color[8] = {
    [AV_LOG_PANIC  /8] = 12,
    [AV_LOG_FATAL  /8] = 12,
    [AV_LOG_ERROR  /8] = 12,
    [AV_LOG_WARNING/8] = 14,
    [AV_LOG_INFO   /8] =  7,
    [AV_LOG_VERBOSE/8] = 10,
    [AV_LOG_DEBUG  /8] = 10,
    [AV_LOG_TRACE  /8] = 8,
};

static int16_t background, attr_orig;
static HANDLE con;
#else

static const uint32_t color[8] = {
        [AV_LOG_PANIC  /8] =  52 << 16 | 196 << 8 | 0x41,
        [AV_LOG_FATAL  /8] = 208 <<  8 | 0x41,
        [AV_LOG_ERROR  /8] = 196 <<  8 | 0x11,
        [AV_LOG_WARNING/8] = 226 <<  8 | 0x03,
        [AV_LOG_INFO   /8] = 253 <<  8 | 0x09,
        [AV_LOG_VERBOSE/8] =  40 <<  8 | 0x02,
        [AV_LOG_DEBUG  /8] =  34 <<  8 | 0x02,
        [AV_LOG_TRACE  /8] =  34 <<  8 | 0x07,
};

#endif
static int use_color = -1;

static void check_color_terminal(void)
{
//#if defined(_WIN32) && HAVE_SETCONSOLETEXTATTRIBUTE
#if defined(_WIN32)
    CONSOLE_SCREEN_BUFFER_INFO con_info;
    con = GetStdHandle(STD_ERROR_HANDLE);
    use_color = (con != INVALID_HANDLE_VALUE) && !getenv("NO_COLOR") &&
                !getenv("AV_LOG_FORCE_NOCOLOR");
    if (use_color) {
        GetConsoleScreenBufferInfo(con, &con_info);
        attr_orig  = con_info.wAttributes;
        background = attr_orig & 0xF0;
    }
#elif HAVE_ISATTY
    char *term = getenv("TERM");
    use_color = !getenv("NO_COLOR") && !getenv("AV_LOG_FORCE_NOCOLOR") &&
                (getenv("TERM") && isatty(2) || getenv("AV_LOG_FORCE_COLOR"));
    if (   getenv("AV_LOG_FORCE_256COLOR")
        || (term && strstr(term, "256color")))
        use_color *= 256;
#else
    use_color = getenv("AV_LOG_FORCE_COLOR") && !getenv("NO_COLOR") &&
                !getenv("AV_LOG_FORCE_NOCOLOR");
#endif
}

static void colored_fputs(int level, int tint, const char *str)
{
    int local_use_color;
    if (!*str)
        return;

    if (use_color < 0)
        check_color_terminal();

    if (level == AV_LOG_INFO/8) local_use_color = 0;
    else                        local_use_color = use_color;

//#if defined(_WIN32) && HAVE_SETCONSOLETEXTATTRIBUTE
#if defined(_WIN32)
    if (local_use_color)
        SetConsoleTextAttribute(con, background | color[level]);
    fputs(str, stderr);
    if (local_use_color)
        SetConsoleTextAttribute(con, attr_orig);
#else
    if (local_use_color == 1) {
        fprintf(stderr,
                "\033[%"PRIu32";3%"PRIu32"m%s\033[0m",
                (color[level] >> 4) & 15,
                color[level] & 15,
                str);
    } else if (tint && use_color == 256) {
        fprintf(stderr,
                "\033[48;5;%"PRIu32"m\033[38;5;%dm%s\033[0m",
                (color[level] >> 16) & 0xff,
                tint,
                str);
    } else if (local_use_color == 256) {
        fprintf(stderr,
                "\033[48;5;%"PRIu32"m\033[38;5;%"PRIu32"m%s\033[0m",
                (color[level] >> 16) & 0xff,
                (color[level] >> 8) & 0xff,
                str);
    } else
        fputs(str, stderr);
#endif

}
*/

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
    //colored_fputs(level, 0, line);
    va_end(vl);
}

void av_log_newline(void)
{
    if (av_log_level >= AV_LOG_INFO)
        printf("\n");
}