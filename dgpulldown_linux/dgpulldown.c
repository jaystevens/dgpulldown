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
Version 1.0.11-L: Linux version by Eric Olson.
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
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

int check_options(void);
unsigned int timeGetTime(void);

char input_file[2048];
char output_file[2048];
#define ES 0
#define PROGRAM 1
int stream_type;

#define CONVERT_NO_CHANGE       0
#define CONVERT_23976_TO_29970  1
#define CONVERT_24000_TO_29970  2
#define CONVERT_25000_TO_29970  3
#define CONVERT_CUSTOM          4

int process(int n);
unsigned long pOldEditProc;

unsigned int CliActive = 0;
unsigned int Rate = CONVERT_23976_TO_29970;
char InputRate[255];
char OutputRate[255];
unsigned int TimeCodes = 1;
unsigned int DropFrames = 2;
unsigned int StartTime = 0;
char HH[255] = { 0 }, MM[255] = {0}, SS[255] = {0}, FF[255] = {0};

int tff = 1;
static int output_m2v = 1;
int interlaced = 0;


int main(int argc, char **argv)
{
	int i;

    // disable buffering on stdout and stderr
    setbuf(stderr, NULL);
    setbuf(stdout, NULL);

	if (argc > 1) {
		CliActive = 1;
		Rate = CONVERT_NO_CHANGE;
		strcpy(InputRate, "23.976");
		strcpy(OutputRate, "29.970");
		strncpy(input_file, argv[1], sizeof(input_file));
		// default output file
		strcpy(output_file, input_file);
		strcat(output_file, ".pulldown.m2v");
		for (i = 2; i < argc; i++) {
			if (strcmp(argv[i], "-srcfps") == 0) {
				Rate = CONVERT_CUSTOM;
				strcpy(InputRate, argv[i + 1]);
				// gobble up the argument
				i++;
			} else if (strcmp(argv[i], "-destfps") == 0) {
				Rate = CONVERT_CUSTOM;
				strcpy(OutputRate, argv[i + 1]);
				// gobble up the argument
				i++;
			} else if (strcmp(argv[i], "-nom2v") == 0) {
				output_m2v = 0;
			} else if (strcmp(argv[i], "-bff") == 0) {
				tff = 0;
			} else if (strcmp(argv[i], "-interlaced") == 0) {
				interlaced = 1;
			} else if (strcmp(argv[i], "-df") == 0) {
				DropFrames = 1;
			} else if (strcmp(argv[i], "-nodf") == 0) {
				DropFrames = 0;
			} else if (strcmp(argv[i], "-notc") == 0) {
				TimeCodes = 0;
			} else if (strcmp(argv[i], "-start") == 0) {
				StartTime = 1;
				strcpy(HH, argv[i + 1]);
				strcpy(MM, argv[i + 2]);
				strcpy(SS, argv[i + 3]);
				strcpy(FF, argv[i + 4]);
				// gobble up the arguments
				i += 4;
			} else if (strcmp(argv[i], "-o") == 0) {
				strcpy(output_file, argv[i + 1]);
				// gobble up the argument
				i++;
			}
		}
		fprintf(stderr, "dgpulldown for Linux, Unix and OS X\n"
			"This version based of Version 2.0.11 by David Graft\n"
			"Processing, please wait...\n");
		return process(0);
	}
	fprintf(stderr,
		"Usage: dgpulldown input.m2v [options]\n"
		"Options:\n"
		"\n"
		"-o filename\n"
		"    File name for output file.\n"
		"    If omitted, the name will be \"*.pulldown.m2v\".\n"
		"\n"
		"-srcfps rate\n"
		"    rate is any float fps value, e.g., \"23.976\" (default) or\n"
		"    a fraction, e.g., \"30000/1001\"\n"
		"\n"
		"-destfps rate\n"
		"    rate is any valid mpeg2 float "
			"fps value, e.g., \"29.97\" (default).\n"
		"\n"
		"If neither srcfps nor destfps is given, then "
			"no frame rate change\n"
		"is performed.  If srcfps is specified as equal "
			"to destfps, then all\n"
		"pulldown is removed and the stream is flagged "
			"as having a rate equal to\n"
		"that specified in destfps.\n"
		"\n"
		"-nom2v \n"
		"    Do not create an output m2v file.\n"
		"\n"
		"-dumptc\n"
		"    Dump timecodes to \"*.timecodes.txt\".\n"
		"\n"
		"-df \n"
		"    Force dropframes.\n"
		"\n"
		"-nodf\n"
		"    Force no dropframes.\n"
		"\n"
		"-start hh mm ss ff\n"
		"    Set start timecode.\n"
		"\n"
		"-notc\n"
		"    Don't set timecodes.\n"
		"\n"
		"-bff\n"
		"    Generate a BFF output stream. "
			"If absent, a TFF stream is generated.\n"
		"\n"
		"-interlaced\n"
		"    Turn off the progressive flag.\n"
		"\n"

		"Example:\n"
		"\n"
		"dgpulldown source.m2v -o flagged.m2v "
			"-srcfps 24000/1001 -destfps 29.97\n");
	exit(1);
}

// Defines for the start code detection state machine.
#define NEED_FIRST_0  0
#define NEED_SECOND_0 1
#define NEED_1        2

FILE *fp, *wfp;
int fd, wfd;

#define BUFFER_SIZE 32768
unsigned char buffer[BUFFER_SIZE];
unsigned char *Rdptr;
int Read;

void determine_stream_type(void);
void video_parser(void);
void pack_parser(void);
void generate_flags(void);

#define MAX_PATTERN_LENGTH 2000000
unsigned char bff_flags[MAX_PATTERN_LENGTH];
unsigned char tff_flags[MAX_PATTERN_LENGTH];

// For progress bar.
long int size;
long int data_count;

static int state, found;
static int f, F, D;
static int ref;
int EOF_reached;
float tfps, cfps;
long int current_num, current_den, target_num, target_den;
int rate;
int rounded_fps;

int field_count, pict, sec, minute, hour, drop_frame;
int set_tc;

char stats[255];
char done[255];
unsigned int Start, Now, Elapsed;
unsigned int LastProgress = 0;
unsigned int percent = 0;

int process(int notused)
{
	F = 0;
	field_count = 0;
	pict = 0;
	sec = 0;
	minute = 0;
	hour = 0;
	drop_frame = 0;
    Start = timeGetTime();

	// Open the input file.

	if (input_file[0] == 0 || (fp = fopen(input_file, "rb")) == NULL) {
		fprintf(stderr, "Could not open the input file!\n");
		exit(2);
	}
	setvbuf(fp, NULL, _IOFBF, 0);

	// Determine the stream type: ES or program.
	determine_stream_type();
	if (stream_type == PROGRAM) {
		fprintf(stderr, "The input file must be an elementary\nstream, "
			"not a program stream.\n");
		exit(3);
	}
	fclose(fp);
	fp = fopen(input_file, "rb");
	if (fp == NULL) {
		fprintf(stderr, "Could not open the input file!\n");
		exit(4);
	}
	setvbuf(fp, NULL, _IOFBF, 0);

	// Get the file size.
	fd = open(input_file, O_RDONLY);
	size = lseek64(fd, 0, SEEK_END);
	close(fd);

	// Make sure all the options are ok
	if (!check_options()) {
		exit(5);
	}
	// Open the output file.
	if (output_m2v) {
		wfp = fopen(output_file, "wb");
		if (wfp == NULL) {
			char buf[80];
			fprintf(stderr, "Could not open the output file\n%s!\n",
				output_file);
			exit(6);
		}
		setvbuf(wfp, NULL, _IOFBF, 0);
	}
	// Generate the flags sequences.
	// It's easier and cleaner to pre-calculate than
	// to try to work it out on the fly in encode order.
	// Pre-calculation can work in display order.
	generate_flags();

	// Start converting.
	video_parser();

	return 0;
}

// emulate windows timeGetTime sort of, returns seconds
unsigned int timeGetTime(void)
{
    time_t s;
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    s = spec.tv_sec;
    return s;
}

// Write a value in the file at an offset relative
// to the last read character. This allows for
// read ahead (write behind) by using negative offsets.
// Currently, it's only used for the 4-byte timecode.
// The offset is ignored when not doing in-place operation.
void put_byte(int offset, unsigned char val)
{
	long int save, backup;
	fwrite(&val, 1, 1, wfp);
}

// Get a byte from the stream. We do our own buffering
// for in-place operation, otherwise we rely on the
// operating system.
unsigned char get_byte(void)
{
	unsigned char val;
	{
		if (fread(&val, 1, 1, fp) != 1) {
			fprintf(stderr, "Done.\n");
			fclose(fp);
			if (output_m2v)
				fclose(wfp);
			exit(7);
		}
	}
    // progress output, only once per second
    data_count++;
    Now = timeGetTime();
	if (((Now - LastProgress) >= 1) && (size > 0))
	{
		if (Now >= Start)
			Elapsed = Now - Start;
		else
			Elapsed = (4294967295 - Start) + Now + 1;
		LastProgress = Now;
		percent = data_count * 100 / size;
		//sprintf(stats, "%02d %7d input frames : output time %02d:%02d:%02d%c%02d @ %6.3f [elapsed %d sec]\n",
		//	percent, "%", F, hour, minute, sec, (drop_frame ? ',' : '.'), pict, tfps, Elapsed / 1000);
		fprintf(stderr, "%02d%% %7d input frames : output time %02d:%02d:%02d%c%02d @ %6.3f [elapsed %d sec]\n",
			percent, F, hour, minute, sec, (drop_frame ? ',' : '.'), pict, tfps, Elapsed);
	}

	return val;
}

void video_parser(void)
{
	unsigned char val, tc[4];
	int trf;

	// Inits.
	state = NEED_FIRST_0;
	found = 0;
	f = F = 0;
	EOF_reached = 0;
	data_count = 0;

	// Let's go!
	while (1) {
		// Parse for start codes.
		val = get_byte();

		if (output_m2v)
			put_byte(0, val);

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
			if (val == 1) {
				found = 1;
				state = NEED_FIRST_0;
			} else if (val != 0)
				state = NEED_FIRST_0;
			break;
		}
		if (found == 1) {
			// Found a start code.
			found = 0;
			val = get_byte();
			if (output_m2v)
				put_byte(0, val);

			if (val == 0xb8) {
				// GOP.
				F += f;
				f = 0;
				if (set_tc) {
					// Timecode and drop frame
					for (pict = 0; pict < 4; pict++)
						tc[pict] = get_byte();

					//determine frame->tc
					pict = field_count >> 1;
					if (drop_frame)
						pict +=
							18 * (pict / 17982) + 2 * ((pict % 17982 -
								2) / 1798);
					pict -= (sec = pict / rounded_fps) * rounded_fps;
					sec -= (minute = sec / 60) * 60;
					minute -= (hour = minute / 60) * 60;
					hour %= 24;
					//now write timecode
					val =
						drop_frame | (hour << 2) | ((minute & 0x30) >> 4);
					if (output_m2v)
						put_byte(-3, val);
					val =
						((minute & 0x0f) << 4) | 0x8 | ((sec & 0x38) >> 3);
					if (output_m2v)
						put_byte(-2, val);
					val = ((sec & 0x07) << 5) | ((pict & 0x1e) >> 1);
					if (output_m2v)
						put_byte(-1, val);
					val = (tc[3] & 0x7f) | ((pict & 0x1) << 7);
					if (output_m2v)
						put_byte(0, val);
				} else {
					//just read timecode
					val = get_byte();
					if (output_m2v)
						put_byte(0, val);
					drop_frame = (val & 0x80) >> 7;
					minute = (val & 0x03) << 4;
					hour = (val & 0x7c) >> 2;
					val = get_byte();
					if (output_m2v)
						put_byte(0, val);
					minute |= (val & 0xf0) >> 4;
					sec = (val & 0x07) << 3;
					val = get_byte();
					if (output_m2v)
						put_byte(0, val);
					sec |= (val & 0xe0) >> 5;
					pict = (val & 0x1f) << 1;
					val = get_byte();
					if (output_m2v)
						put_byte(0, val);
					pict |= (val & 0x80) >> 7;
				}
			} else if (val == 0x00) {
				// Picture.
				val = get_byte();
				if (output_m2v)
					put_byte(0, val);
				ref = (val << 2);
				val = get_byte();
				if (output_m2v)
					put_byte(0, val);
				ref |= (val >> 6);
				D = F + ref;
				f++;
				if (D >= MAX_PATTERN_LENGTH - 1) {
					fclose(fp);
					if (output_m2v)
						fclose(wfp);
					fprintf(stderr,
						"Maximum filelength exceeded, aborting!\n");
					exit(8);
				}
			} else if ((rate != -1) && (val == 0xB3)) {
				// Sequence header.
				val = get_byte();
				if (output_m2v)
					put_byte(0, val);
				val = get_byte();
				if (output_m2v)
					put_byte(0, val);
				val = get_byte();
				if (output_m2v)
					put_byte(0, val);
				val = (get_byte() & 0xf0) | rate;
				if (output_m2v)
					put_byte(0, val);
			} else if (val == 0xB5) {
				val = get_byte();
				if (output_m2v)
					put_byte(0, val);
				if ((val & 0xf0) == 0x80) {
					// Picture coding extension.
					val = get_byte();
					if (output_m2v)
						put_byte(0, val);
					val = get_byte();
					if (output_m2v)
						put_byte(0, val);
					val = get_byte();
					//rewrite trf
					trf = tff ? tff_flags[D] : bff_flags[D];
					val &= 0x7d;
					val |= (trf & 2) << 6;
					val |= (trf & 1) << 1;
					field_count += 2 + (trf & 1);
					if (output_m2v)
						put_byte(0, val);
					// Set progressive frame. This is needed for RFFs to work.
					if(interlaced) {
						val = get_byte() & ~0x08;
					} else {
						val = get_byte() | 0x80;
					}
					if (output_m2v)
						put_byte(0, val);
				} else if ((val & 0xf0) == 0x10) {
					// Sequence extension
					// Clear progressive sequence. This is needed to
					// get RFFs to work field-based.
					val = get_byte() & ~0x08;
					if (output_m2v)
						put_byte(0, val);
				}
			}
		}
	}
	return;
}

void determine_stream_type(void)
{
	//int i;
	unsigned char val, tc[4];
	int state = 0, found = 0;

	stream_type = ES;

	// Look for start codes.
	state = NEED_FIRST_0;
	// Read timecode, and rate from stream
	field_count = -1;
	rate = -1;
	while ((field_count == -1) || (rate == -1)) {
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
			if (val == 1) {
				found = 1;
				state = NEED_FIRST_0;
			} else if (val != 0)
				state = NEED_FIRST_0;
			break;
		}
		if (found == 1) {
			// Found a start code.
			found = 0;
			val = get_byte();
			if (val == 0xba) {
				stream_type = PROGRAM;
				break;
			} else if (val == 0xb8) {
				// GOP.
				if (field_count == -1) {
					for (pict = 0; pict < 4; pict++)
						tc[pict] = get_byte();
					drop_frame = (tc[0] & 0x80) >> 7;
					hour = (tc[0] & 0x7c) >> 2;
					minute = (tc[0] & 0x03) << 4 | (tc[1] & 0xf0) >> 4;
					sec = (tc[1] & 0x07) << 3 | (tc[2] & 0xe0) >> 5;
					pict = (tc[2] & 0x1f) << 1 | (tc[3] & 0x80) >> 7;
					field_count = -2;
				}
			} else if (val == 0xB3) {
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
}

int check_options(void)
{
	char buf[100];

	float float_rates[9] =
		{ 0.0, (float) 23.976, 24.0, 25.0, (float) 29.97, 30.0, 50.0,
		(float) 59.94, 60.0
	};

	if (Rate == CONVERT_NO_CHANGE) {
		tfps = float_rates[rate];
	} else if (Rate == CONVERT_23976_TO_29970) {
		tfps = (float) 29.970;
		cfps = (float) 23.976;
		current_num = 24000;
		current_den = 1001;
	} else if (Rate == CONVERT_24000_TO_29970) {
		tfps = (float) 29.970;
		cfps = (float) 24.000;
		current_num = 24;
		current_den = 1;
	} else if (Rate == CONVERT_25000_TO_29970) {
		tfps = (float) 29.970;
		cfps = (float) 25.000;
		current_num = 25;
		current_den = 1;
	} else if (Rate == CONVERT_CUSTOM) {
		if (strchr(InputRate, '/') != NULL) {
			// Have a fraction specified.
			char *p;

			p = InputRate;
			sscanf(p, "%I64Ld", &current_num);
			while (*p++ != '/');
			sscanf(p, "%I64Ld", &current_den);
		} else {
			// Have a decimal specified.
			float f;

			sscanf(InputRate, "%f", &f);
			current_num = (long int) (f * 1000000.0);
			current_den = 1000000;
		}
		sscanf(OutputRate, "%f", &tfps);
	}
	if (fabs(tfps - 23.976) < 0.01)	// <-- we'll let people cheat a little here (ie 23.98)
	{
		rate = 1;
		rounded_fps = 24;
		drop_frame = 0x80;
		target_num = 24000;
		target_den = 1001;
	} else if (fabs(tfps - 24.000) < 0.001) {
		rate = 2;
		rounded_fps = 24;
		drop_frame = 0;
		target_num = 24;
		target_den = 1;
	} else if (fabs(tfps - 25.000) < 0.001) {
		rate = 3;
		rounded_fps = 25;
		drop_frame = 0;
		target_num = 25;
		target_den = 1;
	} else if (fabs(tfps - 29.970) < 0.001) {
		rate = 4;
		rounded_fps = 30;
		drop_frame = 0x80;
		target_num = 30000;
		target_den = 1001;
	} else if (fabs(tfps - 30.000) < 0.001) {
		rate = 5;
		rounded_fps = 30;
		drop_frame = 0;
		target_num = 30;
		target_den = 1;
	} else if (fabs(tfps - 50.000) < 0.001) {
		rate = 6;
		rounded_fps = 50;
		drop_frame = 0;
		target_num = 50;
		target_den = 1;
	} else if (fabs(tfps - 59.940) < 0.001) {
		rate = 7;
		rounded_fps = 60;
		drop_frame = 0x80;
		target_num = 60000;
		target_den = 1001;
	} else if (fabs(tfps - 60.000) < 0.001) {
		rate = 8;
		rounded_fps = 60;
		drop_frame = 0;
		target_num = 60;
		target_den = 1;
	} else {
		fprintf(stderr, "Target rate is not a legal MPEG2 rate\n");
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
		fprintf(stderr,
			"target rate/current rate\nmust not be greater than 1.5\n");
		return 0;
	} else if (target_num < current_num) {
		fprintf(stderr,
			"target rate/current rate\nmust not be less than 1.0\n");
		return 0;
	}
	// set up df and tc vars
	if (TimeCodes) {
		// if the user wants to screw up the timecode... why not
		if (DropFrames == 0) {
			drop_frame = 0;
		} else if (DropFrames == 1) {
			drop_frame = 0x80;
		}
		// get timecode start (only if set tc is checked too, though)
		if (StartTime) {
			if (sscanf(HH, "%d", &hour) < 1)
				hour = 0;
			else if (hour < 0)
				hour = 0;
			else if (hour > 23)
				hour = 23;
			if (sscanf(MM, "%d", &minute) < 1)
				minute = 0;
			else if (minute < 0)
				minute = 0;
			else if (minute > 59)
				minute = 59;
			if (sscanf(SS, "%d", &sec) < 1)
				sec = 0;
			else if (sec < 0)
				sec = 0;
			else if (sec > 59)
				sec = 59;
			if (sscanf(FF, "%d", &pict) < 1)
				pict = 0;
			else if (pict < 0)
				pict = 0;
			else if (pict >= rounded_fps)
				pict = rounded_fps - 1;
		}
		set_tc = 1;
	} else
		set_tc = 0;

	// Determine field_count for timecode start
	pict = (((hour * 60) + minute) * 60 + sec) * rounded_fps + pict;
	if (drop_frame)
		pict -= 2 * (pict / 1800 - pict / 18000);
	field_count = pict << 1;

	// Recalc timecode and rewrite boxes
	if (drop_frame)
		pict += 18 * (pict / 17982) + 2 * ((pict % 17982 - 2) / 1798);
	pict -= (sec = pict / rounded_fps) * rounded_fps;
	sec -= (minute = sec / 60) * 60;
	minute -= (hour = minute / 60) * 60;
	hour %= 24;
	return 1;
}

void generate_flags(void)
{
	// Yay for integer math
	unsigned char *p, *q;
	unsigned int i, trfp;
	long int dfl, tfl;

	dfl = (target_num - current_num) << 1;
	tfl = current_num >> 1;

	// Generate BFF & TFF flags.
	p = bff_flags;
	q = tff_flags;
	trfp = 0;
	for (i = 0; i < MAX_PATTERN_LENGTH; i++) {
		tfl += dfl;
		if (tfl >= current_num) {
			tfl -= current_num;
			*p++ = (trfp + 1);
			*q++ = ((trfp ^= 2) + 1);
		} else {
			*p++ = trfp;
			*q++ = (trfp ^ 2);
		}
	}
}
