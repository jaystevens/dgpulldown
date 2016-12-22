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

#include <windows.h>
#include <stdio.h> 
#include <io.h>
#include <fcntl.h>
#include <commctrl.h>
#include <math.h>
#include "stdafx.h"

BOOL check_options(void);

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

HWND hWnd;
HANDLE hThread;
DWORD threadId;
DWORD WINAPI process(LPVOID n);
unsigned long pOldEditProc;

unsigned int CliActive = 0;
unsigned int Rate = CONVERT_23976_TO_29970;
char InputRate[255];
char OutputRate[255];
unsigned int TimeCodes = 1;
unsigned int DropFrames = 2;
unsigned int StartTime = 0;
char HH[255] = { 0 }, MM[255] = { 0 }, SS[255] = { 0 }, FF[255] = { 0 };
unsigned int Debug = 0;
int inplace = 0;
int tff = 1;
static int output_m2v = 1;
void helpText(void);

int main()
{
	int argc = __argc;
	char **argv = __argv;
	int i;

	// disable buffering on stdout and stderr
	setbuf(stderr, NULL);
	setbuf(stdout, NULL);

	if (__argc > 1)
	{
		// help check
		if (strcmp(argv[1], "-h") == 0)
		{
			helpText();
			return 1;
		}
		if (strcmp(argv[1], "-help") == 0)
		{
			helpText();
			return 1;
		}
		if (strcmp(argv[1], "--help") == 0)
		{
			helpText();
			return 1;
		}

		CliActive = 1;
		Rate = CONVERT_NO_CHANGE;
		strcpy(InputRate, "23.976");
		strcpy(OutputRate, "29.970");
		strncpy(input_file, __argv[1], sizeof(input_file));
		// default output file
		strcpy(output_file, input_file);
		strcat(output_file, ".pulldown.m2v");
		for (i = 2; i < argc; i++)
		{
			if (strcmp(argv[i], "-srcfps") == 0)
			{
				Rate = CONVERT_CUSTOM;
				strcpy(InputRate, argv[i+1]);
				// gobble up the argument
				i++;
			}
			else if (strcmp(argv[i], "-destfps") == 0)
			{
				Rate = CONVERT_CUSTOM;
				strcpy(OutputRate, argv[i+1]);
				// gobble up the argument
				i++;
			}
			else if (strcmp(argv[i], "-dumptc") == 0)
			{
				Debug = 1;
			}
			else if (strcmp(argv[i], "-nom2v") == 0)
			{
				output_m2v = 0;
			}
			else if (strcmp(argv[i], "-inplace") == 0)
			{
				inplace = 1;
			}
			else if (strcmp(argv[i], "-bff") == 0)
			{
				tff = 0;
			}
			else if (strcmp(argv[i], "-df") == 0)
			{
				DropFrames = 1;
			}
			else if (strcmp(argv[i], "-nodf") == 0)
			{
				DropFrames = 0;
			}
			else if (strcmp(argv[i], "-notc") == 0)
			{
				TimeCodes = 0;
			}
			else if (strcmp(argv[i], "-start") == 0)
			{
				StartTime = 1;
				strcpy(HH, argv[i+1]);
				strcpy(MM, argv[i+2]);
				strcpy(SS, argv[i+3]);
				strcpy(FF, argv[i+4]);
				// gobble up the arguments
				i += 4;
			}
			else if (strcmp(argv[i], "-o") == 0)
			{
				strcpy(output_file, argv[i+1]);
				// gobble up the argument
				i++;
			}
		}

		//freopen("CONIN$", "rb", stdin);
		//freopen("CONOUT$", "wb", stdout);
		//freopen("CONOUT$", "wb", stderr);

		fprintf(stderr, 
			"dgpulldown_cmdonly Version 1.5.1-C\n"
			"This version based of Version 2.0.11 by Donald A. Graft/Jetlag/timecop\n"
			"Processing, please wait...\n");
		hThread = CreateThread(NULL, 32000, process, 0, 0, &threadId);
		WaitForSingleObject(hThread, INFINITE);
		return 0;
	}
	else
	{
		// no options passed in, print help text and exit.
		helpText();
		return 1;
	}
}

void helpText(void)
{
	// TODO - fix this, it was copied from linux version
	fprintf(stderr,
		"Usage: dgpulldown input.m2v [options]\n"
		"Options:\n"
		"-o filename            File name for output file, if omitted, the name will be \"*.pulldown.m2v\".\n"
		"-srcfps rate           Rate is any float fps value, e.g., \"23.976\" (default) or a fraction, e.g., \"30000/1001\"\n"
		"-destfps rate          Rate is any valid mpeg2 float fps value, e.g., \"29.97\" (default).\n"
		"                       Valid rates: 23.976, 24, 25, 23.97, 30, 50, 59.94, 60\n"
		"-nom2v                 Do not create an output m2v file.\n"
		"-inplace               Modify the input file instead of creating an output file. [WINDOWS ONLY]\n"
		"-dumptc                Dump timecodes to \"*.timecodes.txt\". [WINDOWS ONLY]\n"
		"-df                    Force dropframes.\n"
		"-nodf                  Force no dropframes.\n"
		"-start hh mm ss ff     Set start timecode.\n"
		"-notc                  Do not set timecodes.\n"
		"-bff                   Generate a BFF (Bottom Field First) output stream.\n"
		"                       If absent, a TFF (Top Field First) stream is generated.\n"
		"                       NTSC is BFF, PAL is TFF\n"
		//"-interlaced            Turn off the progressive flag. [LINUX/MAC ONLY]\n"
		//"                       Used to change BFF/TFF interlaced mode of output file.\n"
		//"                       DO NOT USE WITH PULLDOWN OPTIONS\n"
		"-h|-help|--help        Print this help message.\n"
		"\n"
		"If neither srcfps nor destfps is given, then no frame rate change is performed.\n"
		"If srcfps is specified as equal to destfps, then all pulldown is removed and the\n"
		"stream is flagged as having a rate equal to that specified in destfps.\n"
		"\n"
		"Example:\n"
		"dgpulldown source.m2v -o flagged.m2v -srcfps 23.976 -destfps 29.97\n");
}

// Defines for the start code detection state machine.
#define NEED_FIRST_0  0
#define NEED_SECOND_0 1
#define NEED_1        2

FILE *fp,*wfp,*dfp;
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
__int64 size;
__int64 data_count;

static int state, found;
static int f, F, D;
static int ref;
int EOF_reached;
float tfps, cfps;
__int64 current_num, current_den, target_num, target_den;
int rate;
int rounded_fps;

int field_count, pict, sec, minute, hour, drop_frame;
int set_tc;

char stats[255];
char done[255];
unsigned int Start, Now, Elapsed;
unsigned int LastProgress = 0;
unsigned int percent = 0;

void KillThread(void)
{
	if (CliActive)
		fprintf(stderr, "Done.\n");
	ExitThread(0);
}

DWORD WINAPI process(LPVOID n)
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
	if (inplace)
	{
		if (input_file[0] == 0 || (wfd = _open(input_file, _O_RANDOM | _O_BINARY | _O_RDWR)) == -1)
		{
			fprintf(stderr, "Could not open the input file!");
			KillThread();
		}
		// Read the first chunk of data.
		Read = _read(wfd, buffer, BUFFER_SIZE);
		Rdptr = buffer - 1;
	}
	else
	{
		if (input_file[0] == 0 || (fp = fopen(input_file, "rb")) == NULL)
		{
			fprintf(stderr, "Could not open the input file!");
			KillThread();
		}
		setvbuf(fp, NULL, _IOFBF, BUFSIZ);
	}

	// Determine the stream type: ES or program.
	determine_stream_type();
	if (stream_type == PROGRAM)
	{
		printf(stderr, "The input file must be an elementary\nstream, not a program stream.");
		KillThread();
	}

	// Re-open the input file.
	if (inplace)
	{
		_close(wfd);
		wfd = _open(input_file, _O_RANDOM | _O_BINARY | _O_RDWR);
		if (wfd == -1)
		{
			fprintf(stderr, "Could not open the input file!");
			KillThread();
		}
		Read = _read(wfd, buffer, BUFFER_SIZE);
		Rdptr = buffer - 1;
	}
	else
	{
		fclose(fp);
		fp = fopen(input_file, "rb");
		if (fp == NULL)
		{
			fprintf(stderr, "Could not open the input file!");
			KillThread();
		}
		setvbuf(fp, NULL, _IOFBF, BUFSIZ);
	}

	// Get the file size.
	fd = _open(input_file, _O_RDONLY | _O_BINARY | _O_SEQUENTIAL);
	size = _lseeki64(fd, 0, SEEK_END);
	_close(fd);

	// Make sure all the options are ok
	if (!check_options()) 
	{
		KillThread();
	}

	// Open the output file.
	if (output_m2v && !inplace)
	{
		wfp = fopen(output_file, "wb");
		if (wfp == NULL)
		{
			fprintf(stderr, "Could not open the output file\n%s!", output_file);
			KillThread();
		}
		setvbuf(wfp, NULL, _IOFBF, BUFSIZ);
	}

	if (Debug) {
		if (CliActive)
		{
			strcat(output_file, ".timecode.txt");
			dfp = fopen(output_file, "w");
		}
		else
		{
			strcpy(output_file, input_file);
			strcat(output_file, ".pulldown.m2v.timecode.txt");
			dfp = fopen(output_file, "w");
		}
		if (dfp == NULL)
		{
			fprintf(stderr, "Could not open the timecode file!");
			KillThread();
		}
		fprintf(dfp,"   field   frame  HH:MM:SSdFF\n");
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

// Write a value in the file at an offset relative
// to the last read character. This allows for
// read ahead (write behind) by using negative offsets.
// Currently, it's only used for the 4-byte timecode.
// The offset is ignored when not doing in-place operation.
_inline void put_byte(int offset, unsigned char val)
{
	__int64 save, backup;

	if (inplace)
	{
		// Save the file position.
		save = _lseeki64(wfd, 0, SEEK_CUR);
		// Calculate the amount to seek back to write this value.
		// offset is relative to the position of the last read value.
		backup = buffer + Read - Rdptr - offset;
		// Seek back.
		_lseeki64(wfd, -backup, SEEK_CUR);
		// Write the value.
		_write(wfd, &val, 1);
		// Restore the file position to be ready for the next
		// read buffer refill.
		_lseeki64(wfd, save, SEEK_SET);
	}
	else
	{
		fwrite(&val, 1, 1, wfp);
	}
}

// Get a byte from the stream. We do our own buffering
// for in-place operation, otherwise we rely on the
// operating system.
_inline unsigned char get_byte(void)
{
	unsigned char val;

	if (inplace)
	{
		if (Rdptr > &buffer[Read-2])
		{
			// Incrementing to the next byte will take us outside the buffer,
			// so we need to refill it.
			Read = _read(wfd, buffer, BUFFER_SIZE);
			Rdptr = buffer;
			if (Read < 1)
			{
				// No more data. Finish up.
				_close(wfd);
				if (Debug) fclose(dfp);
				KillThread();
			}
			// We return the first byte of the buffer now.
			val = *Rdptr;
		}
		else
		{
			// Increment the buffer pointer and return the pointed to byte.
			val = *++Rdptr;
		}
	}
	else
	{
		if (fread(&val, 1, 1, fp) != 1)
		{
			fclose(fp);
			if (output_m2v && !inplace) fclose(wfp);
			if (Debug) fclose(dfp);
			KillThread();
		}
	}
	// progress output, update only once per second
	data_count++;
	Now = timeGetTime();
	//if (!(data_count++ % 524288) && (size > 0))
	if (((Now - LastProgress) >= 1000) && (size > 0))
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
			percent, F, hour, minute, sec, (drop_frame ? ',' : '.'), pict, tfps, Elapsed / 1000);
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
	while(1)
	{
		// Parse for start codes.
		val = get_byte();

	    if (output_m2v && !inplace) put_byte(0, val);

		switch (state)
		{
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
		if (found == 1)
		{
			// Found a start code.
			found = 0;
			val = get_byte();
			if (output_m2v && !inplace) put_byte(0, val);

			if (val == 0xb8)
			{
				// GOP.
				F += f;
				f = 0;
				if (set_tc)
				{
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
					if (output_m2v) put_byte(-3, val);
					val = ((minute & 0x0f) << 4) | 0x8 | ((sec & 0x38) >> 3);
					if (output_m2v) put_byte(-2, val);
					val = ((sec & 0x07) << 5) | ((pict & 0x1e) >> 1);
					if (output_m2v) put_byte(-1, val);
					val = (tc[3] & 0x7f) | ((pict & 0x1) << 7);
					if (output_m2v) put_byte(0, val);
				}
				else
				{
					//just read timecode
					val = get_byte();
					if (output_m2v && !inplace) put_byte(0, val);
					drop_frame = (val & 0x80) >> 7;
					minute = (val & 0x03) << 4;
					hour = (val & 0x7c) >> 2;
					val = get_byte();
					if (output_m2v && !inplace) put_byte(0, val);
					minute |= (val & 0xf0) >> 4;
					sec = (val & 0x07) << 3;
					val = get_byte();
					if (output_m2v && !inplace) put_byte(0, val);
					sec |= (val & 0xe0) >> 5;
					pict = (val & 0x1f) << 1;
					val = get_byte();
					if (output_m2v && !inplace) put_byte(0, val);
					pict |= (val & 0x80) >> 7;
				}
				if (Debug) fprintf(dfp,"%7d  %02d:%02d:%02d%c%02d\n",F,hour,minute,sec,(drop_frame?',':'.'),pict);
			}
			else if (val == 0x00)
			{
				// Picture.
				val = get_byte();
				if (output_m2v && !inplace) put_byte(0, val);
				ref = (val << 2);
				val = get_byte();
				if (output_m2v && !inplace) put_byte(0, val);
				ref |= (val >> 6);
				D = F + ref;
				f++;
				if (D >= MAX_PATTERN_LENGTH - 1)
				{
					if (inplace)
						_close(wfd);
					else
					{
						fclose(fp);
						if (output_m2v)
							fclose(wfp);
					}
					fprintf(stderr, "Maximum filelength exceeded, aborting!");
					KillThread();
				}
			}
			else if ((rate != -1) && (val == 0xB3))
			{
				// Sequence header.
				val = get_byte();
				if (output_m2v && !inplace) put_byte(0, val);
				val = get_byte();
				if (output_m2v && !inplace) put_byte(0, val);
				val = get_byte();
				if (output_m2v && !inplace) put_byte(0, val);
				val = (get_byte() & 0xf0) | rate;
				if (output_m2v) put_byte(0, val);
			}
			else if (val == 0xB5)
			{
				val = get_byte();
				if (output_m2v && !inplace) put_byte(0, val);
				if ((val & 0xf0) == 0x80)
				{
					// Picture coding extension.
					val = get_byte();
					if (output_m2v && !inplace) put_byte(0, val);
					val = get_byte();
					if (output_m2v && !inplace) put_byte(0, val);
					val = get_byte();
					//rewrite trf
					trf = tff ? tff_flags[D] : bff_flags[D];
					val &= 0x7d;
					val |= (trf & 2) << 6;
					val |= (trf & 1) << 1;
					field_count += 2 + (trf & 1);
					if (output_m2v) put_byte(0, val);
					// Set progressive frame. This is needed for RFFs to work.
					val = get_byte() | 0x80;
					if (output_m2v) put_byte(0, val);
				}
				else if ((val & 0xf0) == 0x10)
				{
					// Sequence extension
					// Clear progressive sequence. This is needed to
					// get RFFs to work field-based.
					val = get_byte() & ~0x08;
					if (output_m2v) put_byte(0, val);
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
	while ((field_count==-1) || (rate==-1))
	{
		val = get_byte();
		switch (state)
		{
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
		if (found == 1)
		{
			// Found a start code.
			found = 0;
			val = get_byte();
			if (val == 0xba)
			{
				stream_type = PROGRAM;
				break;
			}
			else if (val == 0xb8)
			{
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
			else if (val == 0xB3)
			{
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

BOOL check_options(void)
{
    char buf[100];

	float float_rates[9] = { 0.0, (float)23.976, 24.0, 25.0, (float)29.97, 30.0, 50.0, (float)59.94, 60.0 };
	
	if (Rate == CONVERT_NO_CHANGE)
	{
		tfps = float_rates[rate];
	}
	else if (Rate == CONVERT_23976_TO_29970)
	{
		tfps = (float) 29.970;
		cfps = (float) 23.976;
		current_num = 24000;
		current_den = 1001;
	}
	else if (Rate == CONVERT_24000_TO_29970)
	{
		tfps = (float) 29.970;
		cfps = (float) 24.000;
		current_num = 24;
		current_den = 1;
	}
	else if (Rate == CONVERT_25000_TO_29970)
	{
		tfps = (float) 29.970;
		cfps = (float) 25.000;
		current_num = 25;
		current_den = 1;
	}
	else if (Rate == CONVERT_CUSTOM)
	{
		if (strchr(InputRate, '/') != NULL)
		{
			// Have a fraction specified.
			char *p;

			p = InputRate;
			sscanf(p, "%I64Ld", &current_num);
			while (*p++ != '/');
			sscanf(p, "%I64Ld", &current_den);
		}
		else
		{
			// Have a decimal specified.
			float f;

			sscanf(InputRate, "%f", &f);
			current_num = (__int64) (f * 1000000.0);
			current_den = 1000000;
		}

		if(0) { //debug
			sprintf(buf, "%I64Ld", current_num);
			fprintf(stderr, buf, "Current fps numerator");
			sprintf(buf, "%I64Ld", current_den);
			fprintf(stderr, buf, "Current fps denominator");
			return FALSE;
		}

		sscanf(OutputRate, "%f", &tfps);
	}
	if (fabs(tfps - 23.976) < 0.01) // <-- we'll let people cheat a little here (ie 23.98)
	{
		rate = 1;
		rounded_fps = 24;
		drop_frame = 0x80;
		target_num = 24000; target_den = 1001;
	}
	else if (fabs(tfps - 24.000) < 0.001)
	{
		rate = 2;
		rounded_fps = 24;
		drop_frame = 0;
		target_num = 24; target_den = 1;
	}
	else if (fabs(tfps - 25.000) < 0.001)
	{
		rate = 3;
		rounded_fps = 25;
		drop_frame = 0;
		target_num = 25; target_den = 1;
	}
	else if (fabs(tfps - 29.970) < 0.001)
	{
		rate = 4;
		rounded_fps = 30;
		drop_frame = 0x80;
		target_num = 30000; target_den = 1001;
	}
	else if (fabs(tfps - 30.000) < 0.001)
	{
		rate = 5;
		rounded_fps = 30;
		drop_frame = 0;
		target_num = 30; target_den = 1;
	}
	else if (fabs(tfps - 50.000) < 0.001)
	{
		rate = 6;
		rounded_fps = 50;
		drop_frame = 0;
		target_num = 50; target_den = 1;
	}
	else if (fabs(tfps - 59.940) < 0.001)
	{
		rate = 7;
		rounded_fps = 60;
		drop_frame = 0x80;
		target_num = 60000; target_den = 1001;
	}
	else if (fabs(tfps - 60.000) < 0.001)
	{
		rate = 8;
		rounded_fps = 60;
		drop_frame = 0;
		target_num = 60; target_den = 1;
	}
	else
	{
		fprintf(stderr, "Target rate is not a legal MPEG2 rate");
		return FALSE;
	}

	// Make current fps = target fps for "No change"
	if (Rate == CONVERT_NO_CHANGE)
	{
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

	if(0) { //debug
		sprintf(buf, "%I64Ld", current_num);
		fprintf(stderr, buf, "Current fps numerator");
		sprintf(buf, "%I64Ld", current_den);
		fprintf(stderr, buf, "Current fps denominator");
		sprintf(buf, "%I64Ld", target_num);
		fprintf(stderr, buf, "Target fps numerator");
		sprintf(buf, "%I64Ld", target_den);
		fprintf(stderr, buf, "Target fps denominator");
		return FALSE;
	}

	if (((target_num - current_num) >> 1) > current_num)
	{
		fprintf(stderr, "target rate/current rate\nmust not be greater than 1.5");
		return FALSE;
	}
	else if (target_num < current_num)
	{
		fprintf(stderr, "target rate/current rate\nmust not be less than 1.0");
		return FALSE;
	}

	// set up df and tc vars
	if (TimeCodes)
	{
		// if the user wants to screw up the timecode... why not
		if (DropFrames == 0)
		{
			drop_frame = 0;
		}
		else if (DropFrames == 1)
		{
			drop_frame = 0x80;
		}
		// get timecode start (only if set tc is checked too, though)
		if (StartTime)
		{
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
	return TRUE;
}

void generate_flags(void)
{
	// Yay for integer math
	unsigned char *p, *q;
	unsigned int i,trfp;
	__int64 dfl,tfl;

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
