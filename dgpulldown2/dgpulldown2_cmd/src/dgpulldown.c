/* 
 *  DGPulldown Copyright (C) 2005-2007, Donald Graft
 *
 *  DGPulldown is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  DGPulldown is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

 /*------
 Version 1.5.1-C: vs2015 updates by Jason Stevens 2016-12-22.
                command line version
 Version 1.0.11: Fixed the broken drop frame option.
 Version 1.0.10: Fixed a bug in in-place operation that corrupted the last
                 32Kbytes of the file.
 Version 1.0.9 : Fixed a bug in the initialization of the destination file
                 edit box when drag-and-drop is used.
 Version 1.0.8 : Fixed 2GB limit problem of in-place operation,
                 made TFF/BFF selction user configurable
                 (because the previous automatic stream reading was broken),
                 and added a GUI configurable output path.
                 Changes by 'neuron2'.
 Version 1.0.7 : Added option for in-place output (input file is modified).
                 Changes by 'timecop'.
 Version 1.0.6 : Added CLI interface.
                 Changes by 'neuron2'/'timecop'.
 Version 1.0.5 : Added drag-and-drop to input file edit box.
                 Changes by 'timecop'.
 Version 1.0.4 : Repaired broken source frame rate edit box
                 (some valid input rates are rejected).
                 Changes by 'neuron2'.
 ------*/

#define _LARGEFILE64_SOURCE
#if defined(_WIN32)
#else
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include "argparse.h"
#include "av_log.h"
#include "dgpulldown_version.h"


 // these macros help with cross-platform
#ifdef __MINGW32__
#undef fseeko
#define fseeko(x, y, z) fseeko64(x, y, z)
#undef ftello
#define ftello(x)       ftello64(x)
#elif defined(_WIN32)
#undef fseeko
#define fseeko(x, y, z) _fseeki64(x, y, z)
#undef ftello
#define ftello(x)       _ftelli64(x)
#endif

static const char *const usage[] = {
        "dgpulldown [options]",
        NULL,
};

// functions
void KillThread(void);
size_t getMemorySize(void);  // define getMemorySize from getMemorySize.c
size_t getMemoryFree(void);  // define getMemoryFree from getMemorySize.c

// file variables
char *input_filename;
char *output_filename;
FILE *input_fp = NULL;
FILE *output_fp = NULL;

// stream detection variables
#define STREAM_TYPE_ES 0
#define STREAM_TYPE_PROGRAM 1

// input variables
#define CONVERT_NO_CHANGE       0
#define CONVERT_23976_TO_29970  1
#define CONVERT_24000_TO_29970  2
#define CONVERT_25000_TO_29970  3
#define CONVERT_CUSTOM          4

unsigned int Rate = CONVERT_23976_TO_29970;
char InputRate[255];
char OutputRate[255];
unsigned int TimeCodes = 1;
unsigned int DropFrames = 2;
unsigned int StartTime = 0;
char HH[255] = { 0 }, MM[255] = { 0 }, SS[255] = { 0 }, FF[255] = { 0 };
int tff = 1;

// io buffer variables
#define BUFFER_SIZE_1MB (int64_t) 1048576        //    1MB
#define BUFFER_SIZE_16MB (int64_t) 16777216      //   16MB
#define BUFFER_SIZE_64MB (int64_t) 67108864      //   64MB
#define BUFFER_SIZE_128MB (int64_t) 134217728    //  128MB
#define BUFFER_SIZE_256MB (int64_t) 268435456    //  256MB
#define BUFFER_SIZE_512MB (int64_t) 536870912    //  512MB
#define BUFFER_SIZE_1024MB (int64_t) 1073741824  // 1024MB
#define BUFFER_SIZE_1536MB (int64_t) 1610612736  // 1536MB
//#define BUFFER_SIZE_1800MB 1887436800  // 1800MB
#define BUFFER_SIZE_1800MB (int64_t) 3774873600  // 1800MB

size_t system_mem_total = 0;
size_t system_mem_free = 0;
size_t system_mem_usable = 0;
size_t max_buffer_size = 0;
int lowmem = 0;

// read buffer
size_t read_buffer_size = BUFFER_SIZE_1800MB;
unsigned char *read_buffer = NULL;
unsigned char *read_ptr = NULL;
uint64_t read_cnt = 0;
uint64_t read_used = 0;
uint64_t read_total = 0;
uint64_t read_timer = 0;

// write buffer
size_t write_buffer_size = BUFFER_SIZE_1800MB;
unsigned char *write_buffer = NULL;
unsigned char *write_ptr = NULL;
uint64_t write_used = 0;

// field flags variables
#define MAX_PATTERN_LENGTH 2000000
unsigned char bff_flags[MAX_PATTERN_LENGTH];
unsigned char tff_flags[MAX_PATTERN_LENGTH];

// progress bar variables
uint64_t file_size;
uint64_t data_count;
unsigned int time_start, time_now, time_elapsed;
unsigned int time_last = 0;
unsigned int percent = 0;

// Defines for the start code detection state machine.
#define NEED_FIRST_0  0
#define NEED_SECOND_0 1
#define NEED_1        2

// process variables
static int state, found;
static int f, F, D;
static int ref;
float tfps;
int64_t current_num, current_den, target_num, target_den;
int rate;
int rounded_fps;
int field_count, pict, sec, minute, hour, drop_frame;
int set_tc;

static int detect_buffer_size(void)
{
    if (lowmem) {
        // use low memory buffers
        av_log(AV_LOG_INFO, "lowmem mode\n");
        read_buffer_size = BUFFER_SIZE_1MB;
        write_buffer_size = BUFFER_SIZE_1MB;
    } else {
        system_mem_total = getMemorySize();
        av_log(AV_LOG_INFO, "system memory total: %d MB\n", (system_mem_total / 1024 / 1024));
        system_mem_free = getMemoryFree();
        av_log(AV_LOG_INFO, "system memory free : %d MB\n", (system_mem_free / 1024 / 1024));

        // detect max buffer size based on free memory
        if (system_mem_free == 0) {
            // memory detect failed, use 1MB chunks
            max_buffer_size = BUFFER_SIZE_1MB;
        } else {
            // less than 1 GB of free memory, do not use more that 50% of free mem
            system_mem_usable = ((system_mem_free / 4) * 3);  // only use 3/4 of system free max
            max_buffer_size = system_mem_usable / 2;  // we need 2 buffers so divide by 2
            // make sure we use at least 1 MB buffer size
            if (max_buffer_size < BUFFER_SIZE_1MB)
                max_buffer_size = BUFFER_SIZE_1MB;
        }
        //max_buffer_size = BUFFER_SIZE_512MB;
        av_log(AV_LOG_INFO, "max buffer size: %d MB\n", (max_buffer_size / 1024 / 1024));

        if (file_size == 0 || max_buffer_size == 0) {
            av_log(AV_LOG_WARNING, "unable to determine optimal buffer size based on file size, using 1 MB buffers\n");
            read_buffer_size = BUFFER_SIZE_1MB;
            write_buffer_size = BUFFER_SIZE_1MB;
        } else if (file_size <= max_buffer_size) {
            av_log(AV_LOG_INFO, "using file size as buffer size\n");
            read_buffer_size = file_size;
            write_buffer_size = file_size;
        } else {
            av_log(AV_LOG_INFO, "file bigger than available memory, using chunked reading\n");
            read_buffer_size = max_buffer_size;
            write_buffer_size = max_buffer_size;
        }
    }

    av_log(AV_LOG_INFO, "read buffer size : %d MB\n", (read_buffer_size / 1024 / 1024));
    av_log(AV_LOG_INFO, "write buffer size: %d MB\n", (write_buffer_size / 1024 / 1024));

    return 0;
}

static int read_buffer_init(void)
{
    // allocate memory for read buffer
    read_buffer = malloc(read_buffer_size);
    if (!read_buffer) {
        av_log(AV_LOG_ERROR, "error allocating read buffer\n");
        KillThread();
    }

    // zero memory buffer
    memset(read_buffer, 0, read_buffer_size);
    // set read pointer to start of read buffer
    read_ptr = NULL;
    // reset read cnt
    read_cnt = 0;
    // reset read used
    read_used = 0;
    // reset read total
    read_total = 0;
    // reset read timer
    read_timer = 0;

    return 0;
}

static int read_buffer_load(void)
{
    // if read buffer not initialized, return error
    if (!read_buffer)
        return 1;

    if (read_total >= file_size && file_size != 0)
        KillThread();

    if (read_buffer_size >= BUFFER_SIZE_256MB)
        av_log(AV_LOG_INFO, "reading %d MB chunk from file\n", (read_buffer_size / 1024 / 1024));

    // read file into memory
    read_cnt = (int)fread(read_buffer, 1, read_buffer_size, input_fp);
    read_total += read_cnt;
    if (!read_cnt) {
        // at end of file, exit
        KillThread();
    }
    // reset read used
    read_used = 0;
    // reset read pointer to start of read buffer
    read_ptr = read_buffer;

    return 0;
}

static void read_buffer_free(void)
{
    read_ptr = NULL;
    if (read_buffer)
        free(read_buffer);
    read_buffer = NULL;
}

static int write_buffer_init(void)
{
    // allocate memory for write buffer
    write_buffer = malloc(write_buffer_size);
    if (!write_buffer) {
        av_log(AV_LOG_ERROR, "error allocating write buffer\n");
        KillThread();
    }

    // zero memory buffer
    memset(write_buffer, 0, (size_t)write_buffer_size);
    // reset write pointer to start of write buffer
    write_ptr = write_buffer;
    // reset write used
    write_used = 0;

    return 0;
}

static int write_buffer_flush(void)
{
    // if write buffer not initialized, return error
    if (!write_buffer)
        return 1;

    // check if write buffer is empty
    if (write_used == 0)
        return 0;

    if (write_buffer_size >= BUFFER_SIZE_256MB)
        av_log(AV_LOG_INFO, "writing %d MB chunk to file\n", (write_used / 1024 / 1024));

    // write buffer to file
    if (fwrite(write_buffer, (size_t)write_used, 1, output_fp) != 1) {
        av_log(AV_LOG_ERROR, "error flushing write buffer to file\n");
        return 1;
    }
    // reset write used counter
    write_used = 0;
    // reset write pointer to start of write buffer
    write_ptr = write_buffer;

    return 0;
}

static void write_buffer_free(void)
{
    write_ptr = NULL;
    if (write_buffer)
        free(write_buffer);
    write_buffer = NULL;
}

static unsigned int get_time_seconds(void)
{
#if defined(_WIN32)  // windows
    struct timespec spec;
    timespec_get(&spec, TIME_UTC);
    return (unsigned int) spec.tv_sec;
#else  // linux and MacOS
    struct timeval spec;
    gettimeofday(&spec, NULL);
    return (unsigned int)spec.tv_sec;
#endif
}

void KillThread(void)
{
    // free + close vars
    read_buffer_free();
    write_buffer_flush();
    write_buffer_free();

    if (input_fp) {
        fclose(input_fp);
        input_fp = NULL;
    }

    if (output_fp) {
        fclose(output_fp);
        output_fp = NULL;
    }

    time_now = get_time_seconds();
    av_log(AV_LOG_INFO, "Done - took: %d seconds\n", (time_now - time_start));
    exit(0);
}

inline static void put_byte(unsigned char val)
{
    if (!write_ptr) {
        av_log(AV_LOG_ERROR, "tried to write to buffer memory that is not initialized\n");
        KillThread();
    }
    // copy byte to write buffer
    memcpy(write_ptr, &val, 1);
    // add 1 to write used
    write_used++;
    // if at end of write buffer, flush the write buffer
    if (write_used >= write_buffer_size) {
        if (write_buffer_flush())
            KillThread();
    } else {
        // advance write pointer if we did not flush
        write_ptr++;
    }
}

inline static unsigned char get_byte(void)
{
    unsigned char val;

    if (!read_buffer) {
        av_log(AV_LOG_ERROR, "tried to read from buffer memory that is not initialized\n");
        KillThread();
    }
    // load first chunk
    if (!read_ptr) {
        read_buffer_load();
    }
    // get value from read pointer
    val = *read_ptr;
    // add 1 to read used
    read_used++;
    // if at end of read buffer, load another chunk into read buffer
    if (read_used > read_cnt) {
        if (read_buffer_load())
            KillThread();
    } else {
        // advance read pointer if we did not load a new chunk
        read_ptr++;
    }

    // progress output, update only once per second
    data_count++;
    // increment read timer
    read_timer++;
    // only run progress code every 512K [52488]
    if (read_timer > 524288) {
        read_timer = 0;
        time_now = get_time_seconds();
        if (((time_now - time_last) >= 1) && (file_size > 0)) {
            if (time_now >= time_start)
                time_elapsed = time_now - time_start;
            else
#if defined(_WIN32)
                time_elapsed = (time_now - time_start) + 1;
#else
                time_elapsed = (unsigned int) (((4294967295 - time_start) + time_now + 1) / 1000);
#endif
            time_last = time_now;
            percent = (unsigned int) (data_count * 100 / file_size);
            av_log(AV_LOG_INFO, "%02d%% %7d input frames : output time %02d:%02d:%02d%c%02d @ %6.3f [elapsed %d sec]\n",
                   percent, F, hour, minute, sec, (drop_frame ? ',' : '.'), pict, tfps, time_elapsed);
        }
    }

    return val;
}

static void video_parser(void)
{
    unsigned char val, tc[4];
    int trf;

    // Inits.
    state = NEED_FIRST_0;
    found = 0;
    f = F = 0;
    data_count = 0;  // progress bar

    // Let's go!
    while(1)
    {
        // Parse for start codes.
        val = get_byte();
        put_byte(val);

        switch (state) {
            case NEED_FIRST_0:
                if (val == 0)
                    state = NEED_SECOND_0;
                break;
            case NEED_SECOND_0:
                if (val == 0)
                    state = NEED_1;
                else
                    state = NEED_FIRST_0;
                break;
            case NEED_1:
                if (val == 1)
                {
                    found = 1;
                    state = NEED_FIRST_0;
                }
                else if (val != 0)
                    state = NEED_FIRST_0;
                break;
        }
        if (found == 1) {
            // Found a start code.
            found = 0;
            val = get_byte();
            put_byte(val);

            if (val == 0xb8) {
                // GOP.
                F += f;
                f = 0;
                if (set_tc) {
                    // Timecode and drop frame
                    for(pict=0; pict<4; pict++) 
                        tc[pict] = get_byte();

                    //determine frame->tc
                    pict = field_count >> 1;
                    if (drop_frame) pict += 18*(pict/17982) + 2*((pict%17982 - 2) / 1798);
                    pict -= (sec = pict / rounded_fps) * rounded_fps; 
                    sec -= (minute = sec / 60) * 60; 
                    minute -= (hour = minute / 60) * 60;
                    hour %= 24;
                    //now write timecode
                    val = drop_frame | (hour << 2) | ((minute & 0x30) >> 4);
                    put_byte(val);
                    val = ((minute & 0x0f) << 4) | 0x8 | ((sec & 0x38) >> 3);
                    put_byte(val);
                    val = ((sec & 0x07) << 5) | ((pict & 0x1e) >> 1);
                    put_byte(val);
                    val = (tc[3] & 0x7f) | ((pict & 0x1) << 7);
                    put_byte(val);
                } else {
                    //just read timecode
                    val = get_byte();
                    put_byte(val);
                    drop_frame = (val & 0x80) >> 7;
                    minute = (val & 0x03) << 4;
                    hour = (val & 0x7c) >> 2;
                    val = get_byte();
                    put_byte(val);
                    minute |= (val & 0xf0) >> 4;
                    sec = (val & 0x07) << 3;
                    val = get_byte();
                    put_byte(val);
                    sec |= (val & 0xe0) >> 5;
                    pict = (val & 0x1f) << 1;
                    val = get_byte();
                    put_byte(val);
                    pict |= (val & 0x80) >> 7;
                }
            }
            else if (val == 0x00) {
                // Picture.
                val = get_byte();
                put_byte(val);
                ref = (val << 2);
                val = get_byte();
                put_byte(val);
                ref |= (val >> 6);
                D = F + ref;
                f++;
                if (D >= MAX_PATTERN_LENGTH - 1) {
                    av_log(AV_LOG_ERROR, "Maximum filelength exceeded, aborting!");
                    KillThread();
                }
            }
            else if ((rate != -1) && (val == 0xB3)) {
                // Sequence header.
                val = get_byte();
                put_byte(val);
                val = get_byte();
                put_byte(val);
                val = get_byte();
                put_byte(val);
                // set frame rate
                val = (get_byte() & 0xf0) | rate;
                put_byte(val);
            }
            else if (val == 0xB5) {
                val = get_byte();
                put_byte(val);
                if ((val & 0xf0) == 0x80) {
                    // Picture coding extension.
                    val = get_byte();
                    put_byte(val);
                    val = get_byte();
                    put_byte(val);
                    //rewrite trf
                    val = get_byte();
                    trf = tff ? tff_flags[D] : bff_flags[D];
                    val &= 0x7d;
                    val |= (trf & 2) << 6;
                    val |= (trf & 1) << 1;
                    field_count += 2 + (trf & 1);
                    put_byte(val);
                    // Set progressive frame. This is needed for RFFs to work.
                    val = get_byte() | 0x80;
                    put_byte(val);
                }
                else if ((val & 0xf0) == 0x10) {
                    // Sequence extension
                    // Clear progressive sequence. This is needed to
                    // get RFFs to work field-based.
                    val = get_byte() & ~0x08;
                    put_byte(val);
                }
            }
        }
    }
}

static int determine_stream_type(void)
{
    //int i;
    unsigned char val, tc[4];
    int state = 0, found = 0;
    int stream_type = STREAM_TYPE_ES;

    if (fseeko(input_fp, 0, SEEK_SET)) {
        av_log(AV_LOG_ERROR, "error seeking to start of file to determine stream type\n");
        KillThread();
    }

    // Look for start codes.
    state = NEED_FIRST_0;
    // Read timecode, and rate from stream
    field_count = -1;
    rate = -1;
    while ((field_count==-1) || (rate==-1)) {
        val = get_byte();
        switch (state) {
            case NEED_FIRST_0:
                if (val == 0)
                    state = NEED_SECOND_0;
                break;
            case NEED_SECOND_0:
                if (val == 0)
                    state = NEED_1;
                else
                    state = NEED_FIRST_0;
                break;
            case NEED_1:
                if (val == 1)
                {
                    found = 1;
                    state = NEED_FIRST_0;
                }
                else if (val != 0)
                    state = NEED_FIRST_0;
                break;
        }
        if (found == 1) {
            // Found a start code.
            found = 0;
            val = get_byte();
            if (val == 0xba) {
                stream_type = STREAM_TYPE_PROGRAM;
                break;
            }
            else if (val == 0xb8) {
                // GOP.
                if (field_count == -1) {
                    for(pict=0; pict<4; pict++) tc[pict] = get_byte();
                    drop_frame = (tc[0] & 0x80) >> 7;
                    hour = (tc[0] & 0x7c) >> 2;
                    minute = (tc[0] & 0x03) << 4 | (tc[1] & 0xf0) >> 4;
                    sec = (tc[1] & 0x07) << 3 | (tc[2] & 0xe0) >> 5;
                    pict = (tc[2] & 0x1f) << 1 | (tc[3] & 0x80) >> 7;
                    field_count = -2;
                }
            }
            else if (val == 0xB3) {
                // Sequence header.
                if (rate == -1) {
                    get_byte();
                    get_byte();
                    get_byte();
                    rate = get_byte() & 0x0f;
                }
            }
        }
    }
    // seek back to start of file
    if (fseeko(input_fp, 0, SEEK_SET)) {
        av_log(AV_LOG_ERROR, "error seeking back to start of file during stream type parsing\n");
        KillThread();
    }

    return stream_type;
}

static int check_options(void)
{
    char buf[100];

    float float_rates[9] = { 0.0, (float)23.976, 24.0, 25.0, (float)29.97, 30.0, 50.0, (float)59.94, 60.0 };
    
    if (Rate == CONVERT_NO_CHANGE) {
        tfps = float_rates[rate];
    }
    else if (Rate == CONVERT_23976_TO_29970) {
        tfps = (float) 29.970;
        current_num = 24000;
        current_den = 1001;
    }
    else if (Rate == CONVERT_24000_TO_29970) {
        tfps = (float) 29.970;
        current_num = 24;
        current_den = 1;
    }
    else if (Rate == CONVERT_25000_TO_29970) {
        tfps = (float) 29.970;
        current_num = 25;
        current_den = 1;
    }
    else if (Rate == CONVERT_CUSTOM) {
        if (strchr(InputRate, '/') != NULL) {
            // Have a fraction specified.
            char *p;

            p = InputRate;
            sscanf(p, "%I64Ld", &current_num);
            while (*p++ != '/');
            sscanf(p, "%I64Ld", &current_den);
        }
        else {
            // Have a decimal specified.
            float f;

            sscanf(InputRate, "%f", &f);
            current_num = (int64_t) (f * 1000000.0);
            current_den = 1000000;
        }

        sscanf(OutputRate, "%f", &tfps);
    }
    av_log(AV_LOG_INFO, "current_num: %d\n", current_num);
    av_log(AV_LOG_INFO, "current_den: %d\n", current_den);
    if (fabs(tfps - 23.976) < 0.01) // <-- we'll let people cheat a little here (ie 23.98)
    {
        av_log(AV_LOG_INFO, "MPEG2 Rate: 1 [23.976]\n");
        rate = 1;
        rounded_fps = 24;
        drop_frame = 0x80;
        target_num = 24000; target_den = 1001;
    }
    else if (fabs(tfps - 24.000) < 0.001) {
        av_log(AV_LOG_INFO, "MPEG2 Rate: 2 [24.000]\n");
        rate = 2;
        rounded_fps = 24;
        drop_frame = 0;
        target_num = 24; target_den = 1;
    }
    else if (fabs(tfps - 25.000) < 0.001) {
        av_log(AV_LOG_INFO, "MPEG2 Rate: 3 [25.000]\n");
        rate = 3;
        rounded_fps = 25;
        drop_frame = 0;
        target_num = 25; target_den = 1;
    }
    else if (fabs(tfps - 29.970) < 0.001) {
        av_log(AV_LOG_INFO, "MPEG2 Rate: 4 [29.970]\n");
        rate = 4;
        rounded_fps = 30;
        drop_frame = 0x80;
        target_num = 30000; target_den = 1001;
    }
    else if (fabs(tfps - 30.000) < 0.001) {
        av_log(AV_LOG_INFO, "MPEG2 Rate: 5 [30.000]\n");
        rate = 5;
        rounded_fps = 30;
        drop_frame = 0;
        target_num = 30; target_den = 1;
    }
    else if (fabs(tfps - 50.000) < 0.001) {
        av_log(AV_LOG_INFO, "MPEG2 Rate: 6 [50.000]\n");
        rate = 6;
        rounded_fps = 50;
        drop_frame = 0;
        target_num = 50; target_den = 1;
    }
    else if (fabs(tfps - 59.940) < 0.001) {
        av_log(AV_LOG_INFO, "MPEG2 Rate: 7 [59.940]\n");
        rate = 7;
        rounded_fps = 60;
        drop_frame = 0x80;
        target_num = 60000; target_den = 1001;
    }
    else if (fabs(tfps - 60.000) < 0.001) {
        av_log(AV_LOG_INFO, "MPEG2 Rate: 8 [60.000]\n");
        rate = 8;
        rounded_fps = 60;
        drop_frame = 0;
        target_num = 60; target_den = 1;
    }
    else {
        av_log(AV_LOG_ERROR, "Target rate is not a legal MPEG2 rate");
        return 0;
    }

    // Make current fps = target fps for "No change"
    if (Rate == CONVERT_NO_CHANGE) {
        current_num = target_num;
        current_den = target_den;
        // no reason to reset rate
        rate = -1;
    }

    // equate denominators
    if (current_den != target_den) {
        if (current_den == 1)
            current_num *= (current_den = target_den);
        else if (target_den == 1)
            target_num *= (target_den = current_den);
        else {
            current_num *= target_den;
            target_num *= current_den;
            current_den = (target_den *= current_den);
        }
    }
    // make divisible by two
    if ((current_num & 1) || (target_num & 1)) {
        current_num <<= 1;
        target_num <<= 1;
        current_den = (target_den <<= 1);
    }

    if (((target_num - current_num) >> 1) > current_num) {
        av_log(AV_LOG_ERROR, "target rate/current rate must not be greater than 1.5");
        return 0;
    }
    else if (target_num < current_num) {
        av_log(AV_LOG_ERROR, "target rate/current rate must not be less than 1.0");
        return 0;
    }

    // set up df and tc vars
    if (TimeCodes) {
        // if the user wants to screw up the timecode... why not
        if (DropFrames == 0) {
            drop_frame = 0;
        }
        else if (DropFrames == 1) {
            drop_frame = 0x80;
        }
        // get timecode start (only if set tc is checked too, though)
        if (StartTime) {
            if (sscanf(HH, "%d", &hour) < 1) hour = 0;
            else if (hour < 0) hour = 0;    
            else if (hour > 23) hour = 23;
            if (sscanf(MM, "%d", &minute) < 1) minute = 0;
            else if (minute < 0) minute = 0;
            else if (minute > 59) minute = 59;
            if (sscanf(SS, "%d", &sec) < 1) sec = 0;
            else if (sec < 0) sec = 0;
            else if (sec > 59) sec = 59;
            if (sscanf(FF, "%d", &pict) < 1) pict = 0;
            else if (pict < 0) pict = 0;
            else if (pict >= rounded_fps) pict = rounded_fps - 1;
        }
        set_tc = 1;
    } else set_tc = 0;

    // Determine field_count for timecode start
    pict = (((hour*60)+minute)*60+sec)*rounded_fps+pict;
    if (drop_frame) pict -= 2 * (pict/1800 - pict/18000);
    field_count = pict << 1;

    // Recalc timecode and rewrite boxes
    if (drop_frame) pict += 18*(pict/17982) + 2*((pict%17982 - 2) / 1798);
    pict -= (sec = pict / rounded_fps) * rounded_fps; 
    sec -= (minute = sec / 60) * 60; 
    minute -= (hour = minute / 60) * 60;
    hour %= 24;

    sprintf(buf, "%02d",hour);
    strcpy(HH, buf);
    sprintf(buf, "%02d",minute);
    strcpy(MM, buf);
    sprintf(buf, "%02d",sec);
    strcpy(SS, buf);
    sprintf(buf, "%02d",pict);
    strcpy(FF, buf);
    return 1;
}

static void generate_flags(void)
{
    // Yay for integer math
    unsigned char *p, *q;
    unsigned int i,trfp;
    int64_t dfl,tfl;

    dfl = (target_num - current_num) << 1;
    tfl = current_num >> 1;

    // Generate BFF & TFF flags.
    p = bff_flags;
    q = tff_flags;
    trfp = 0;
    for (i = 0; i < MAX_PATTERN_LENGTH; i++)
    {
        tfl += dfl;
        if (tfl >= current_num)
        { 
            tfl -= current_num; 
            *p++ = (trfp + 1);
            *q++ = ((trfp ^= 2) + 1);
        }
        else
        {
            *p++ = trfp;
            *q++ = (trfp ^ 2);
        }
    }
}

static int process(void)
{
    size_t tmp_buffer_size;
    F = 0;
    field_count = 0;
    pict = 0;
    sec = 0;
    minute = 0;
    hour = 0;
    drop_frame = 0;
    time_start = get_time_seconds();

    // Open the input file.
    input_fp = fopen(input_filename, "rb");
    if (!input_fp) {
        av_log(AV_LOG_ERROR, "Could not open the input file!");
        KillThread();
    }

    // load a 1MB chunk into memory to parse stream type
    tmp_buffer_size = read_buffer_size;
    read_buffer_size = BUFFER_SIZE_1MB;
    read_buffer_init();
    read_buffer_load();

    // Determine the stream type: ES or program
    // also setups some values needed by check_options()
    if (determine_stream_type() == STREAM_TYPE_PROGRAM) {
        av_log(AV_LOG_ERROR, "The input file must be an elementary stream, not a program stream.");
        KillThread();
    }

    // reset read buffer after parsing stream type
    read_buffer_free();
    // reset read buffer size to 1G chunk
    read_buffer_size = tmp_buffer_size;

    // Get file size - used for progress
    if (fseeko(input_fp, 0, SEEK_END)) {
        av_log(AV_LOG_ERROR, "error seeking to end of file\n");
        KillThread();
    }
    file_size = ftello(input_fp);
    if (fseeko(input_fp, 0, SEEK_SET)) {
        av_log(AV_LOG_ERROR, "error seeking to start of file\n");
        KillThread();
    }

    // Make sure all the options are ok - this must be after determine_stream_type()
    if (!check_options()) {
        KillThread();
    }

    // Open the output file.
    output_fp = fopen(output_filename, "wb");
    if (output_fp == NULL)
    {
        av_log(AV_LOG_ERROR, "Could not open the output file\n%s!", output_filename);
        KillThread();
    }

    // init read and write buffers
    detect_buffer_size();
    read_buffer_init();
    write_buffer_init();

    // Generate the flags sequences.
    // It's easier and cleaner to pre-calculate than
    // to try to work it out on the fly in encode order.
    // Pre-calculation can work in display order.
    generate_flags();

    // Start converting.
    video_parser();

    return 0;
}

int main(int argc, char *argv[])
{
    // variables
    int init_exit = 0;

    // argparse vars
    int param_convert23 = 0;
    int param_convert24 = 0;
    int param_convert25 = 0;
    char *param_srcfps = NULL;
    char *param_destfps = NULL;
    int param_bff = 0;
    int param_tff = 0;
    int param_df = 0;
    int param_nodf = 0;
    int param_notc = 0;
    char *param_start;

    struct argparse_option options[] = {
            OPT_HELP(),
            OPT_STRING('i', "input", &input_filename, "input m2v file", NULL, 0, 0),
            OPT_STRING('o', "output", &output_filename, "output m2v file", NULL, 0, 0),
            //OPT_STRING('l', "loglevel", &loglevel, "loglevel [quiet, error, warning, info, verbose, debug]", NULL, 0, 0),
            OPT_GROUP("conversion presets"),
            OPT_BOOLEAN(0, "convert23", &param_convert23, "convert 23.976 to 29.970", NULL, 0, 0),
            OPT_BOOLEAN(0, "convert24", &param_convert24, "convert 24.000 to 29.970", NULL, 0, 0),
            OPT_BOOLEAN(0, "convert25", &param_convert25, "convert 25.000 to 29.970", NULL, 0, 0),
            OPT_GROUP("fps"),
            OPT_STRING(0, "srcfps", &param_srcfps, "rate is any float fps value, e.g., \\\"23.976\\\" (default) or a fraction, e.g., \\\"30000/1001\\\"", NULL, 0, 0),
            OPT_STRING(0, "destfps", &param_destfps, "rate is any valid mpeg2 float fps value, e.g., \"29.97\" (default).", NULL, 0, 0),
            // "Valid rates: 23.976, 24, 25, 29.97, 30, 50, 59.94, 60"
            OPT_GROUP("params"),
            OPT_BOOLEAN(0, "bff", &param_bff, "bff", NULL, 0, 0),
            OPT_BOOLEAN(0, "tff", &param_tff, "tff", NULL, 0, 0),
            OPT_BOOLEAN(0, "df", &param_df, "force drop frame", NULL, 0, 0),
            OPT_BOOLEAN(0, "nodf", &param_nodf, "force non drop frame", NULL, 0, 0),
            OPT_BOOLEAN(0, "notc", &param_notc, "do not set timecodes", NULL, 0, 0),
            OPT_STRING(0, "start", &param_start, "set start timecode: hh mm ss ff", NULL, 0, 0),
            OPT_BOOLEAN(0, "lowmem", &lowmem, "use 1 MB buffers", NULL, 0, 0),
            OPT_END(),
    };
    struct argparse argparse;

    // disable buffering on stdout and stderr
    //setbuf(stderr, NULL);
    //setbuf(stdout, NULL);

    printf("dgpulldown2 by Jason Stevens\n");
    printf("This version based off Version 1.0.11 by Donald A. Graft/Jetlag/timecop\n");
    printf("Version: %s\n", DGPULLDOWN_VERSION_FULL);
    printf("Compiler: %s %s\n", COMPILER_NAME, COMPILER_VER);
    printf("compiled on: %s @ %s\n", __TIMESTAMP__, DGPULLDOWN_BUILD_HOST);
    printf("\n");

    // init vars
    strcpy(InputRate, "23.976");
    strcpy(OutputRate, "29.970");
    //Rate = CONVERT_NO_CHANGE;
    Rate = CONVERT_CUSTOM;

    // parse args
    argparse_init(&argparse, options, usage, 0);
    argparse_describe(&argparse, "\ndgpulldown2", "\nutility set repeat field flag in mpeg2");
    argparse_parse(&argparse, argc, (const char **) argv);  // returns argc

    if (param_srcfps) {
        Rate = CONVERT_CUSTOM;
        strcpy(InputRate, param_srcfps);
    }
    if (param_destfps) {
        Rate = CONVERT_CUSTOM;
        strcpy(OutputRate, param_destfps);
    }

    if (param_convert23)
        Rate = CONVERT_23976_TO_29970;
    if (param_convert24)
        Rate = CONVERT_24000_TO_29970;
    if (param_convert25)
        Rate = CONVERT_25000_TO_29970;

    // check input
    if (input_filename == NULL) {
        init_exit = 1;
        av_log(AV_LOG_ERROR, "no input file specified [ -i input.m2v ]\n");
    }

    if (output_filename == NULL) {
        init_exit = 1;
        av_log(AV_LOG_ERROR, "no output file specified [ -o output.m2v ]\n");
    }

    // BFF / TFF
    if (param_bff)
        tff = 0;
    if (param_tff)
        tff = 1;

    // DF / NDF
    if (param_df)
        DropFrames = 1;
    if (param_nodf)
        DropFrames = 0;

    // timecode
    if (param_notc)
        TimeCodes = 0;

    av_log_newline();
    av_log(AV_LOG_INFO, "Params:\n");
    switch (Rate) {
        case CONVERT_NO_CHANGE:
            av_log(AV_LOG_INFO, "Rate: no change\n");
            break;
        case CONVERT_23976_TO_29970:
            av_log(AV_LOG_INFO, "Rate: 23.976 to 29.970\n");
            break;
        case CONVERT_24000_TO_29970:
            av_log(AV_LOG_INFO, "Rate: 24.000 to 29.970\n");
            break;
        case CONVERT_25000_TO_29970:
            av_log(AV_LOG_INFO, "Rate: 25.000 to 29.970\n");
            break;
        case CONVERT_CUSTOM:
            av_log(AV_LOG_INFO, "Rate: Custom\n");
            av_log(AV_LOG_INFO, "InputRate: %s\n", InputRate);
            av_log(AV_LOG_INFO, "OutputRate: %s\n", OutputRate);
            break;
        default:
            av_log(AV_LOG_INFO, "Rate: UNK\n");
    }

    av_log(AV_LOG_INFO, "TFF: %d\n", tff);
    av_log(AV_LOG_INFO, "DropFrames: %d\n", DropFrames);
    av_log(AV_LOG_INFO, "Timecodes: %d\n", TimeCodes);
    av_log_newline();


    /*  TODO - start
     *             else if (strcmp(argv[i], "-start") == 0)
            {
                StartTime = 1;
                strcpy(HH, argv[i+1]);
                strcpy(MM, argv[i+2]);
                strcpy(SS, argv[i+3]);
                strcpy(FF, argv[i+4]);
                // gobble up the arguments
                i += 4;
            }
    */

    if (init_exit) {
        return 1;
    }

     av_log(AV_LOG_INFO, "Processing, please wait...\n");
    process();
    return 0;
}
