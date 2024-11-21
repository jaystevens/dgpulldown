#!/usr/bin/env python
# coding=utf-8

#  Copyright(c) 2016 Jason Stevens
#  All Rights Reserved
#
#  These coded instructions, statements, and computer programs
#  contain unpublished proprietary information written by
#  Jason Stevens, and are protected by Federal copyright law.  They may
#  not be disclosed to third parties or copied or duplicated
#  in any form, in whole or in part, without the prior written consent
#  of Jason Stevens
#
#  This script is a library to author video dvd.
#  Version 0.2 October 2, 2016
#  Version 0.5 October 7, 2016
#
import sys
import os
import io
import argparse
import math
import time
import traceback
import logging

class dgpulldown():
    # constants
    CONVERT_NO_CHANGE = 0
    CONVERT_23976_TO_29970 = 1
    CONVERT_24000_TO_29970 = 2
    CONVERT_25000_TO_29970 = 3
    CONVERT_CUSTOM = 4

    STREAMTYPE_ES = 0
    STREAMTYPE_PROGRAM = 1

    # start code state machine constants
    NEED_FIRST_0 = 0
    NEED_SECOND_0 = 1
    NEED_1 = 2


    # top level variables
    BUFFER_SIZE = (1024 * 1000) * 25  # 25M
    BUFFER_SIZE_OUT = (1024 * 1000) * 1000  # 1G
    #BUFFER_SIZE_OUT = (1024 * 1000)  # 1M
    input_file = None
    input_fd = None
    input_buffer = io.BytesIO()
    input_size = 0
    input_pos = 0
    input_eof = False
    output_file = None
    output_fd = None
    output_buffer = io.BytesIO()

    stream_type = STREAMTYPE_ES

    # input variables
    rate = -1  # rate from sequence header of source file.
    drop_frame = 0  # TODO fix init
    hour = 0  # TODO fix init
    minute = 0  # TODO fix init
    sec = 0  # TODO fix init
    pict = 0  # TODO fix init
    field_count = 0  # TODO fix init
    rate_mode = CONVERT_23976_TO_29970
    input_rate = None  # string i.e. 23.976
    output_reate = None  # string i.e. 29.970
    TimeCodes = 1  # -notc=0, default=1
    DropFrames = 2  # -df=1, -nodf=0, default=2
    HH = 0  # start, HH
    MM = 0  # start, MM
    SS = 0  # start, SS
    FF = 0  # start, FF
    tff = 1  # tff = 1, bff = 0
    output_m2v = 1
    current_num = None
    current_den = None
    target_num = None
    target_den = None

    MAX_PATTERN_LENGTH = 2000000
    #unsigned char bff_flags[MAX_PATTERN_LENGTH];
    #unsigned char tff_flags[MAX_PATTERN_LENGTH];
    bff_flags = {}
    tff_flags = {}

    def __init__(self, args=None):
        if args is not None:
            self.input_file = args.input
            self.output_file = args.output
        else:
            # for now must run from cmd_line
            print('no args passed in, exit.')
            sys.exit(0)

        # get launch path
        if hasattr(sys, "frozen"):
            self.__launchPath = os.path.dirname(os.path.realpath(sys.executable))
        else:
            self.__launchPath = os.path.dirname(os.path.realpath(__file__))

        # debug mode
        if os.path.exists(os.path.join(self.__launchPath, 'debug.txt')):
            self.__debugMode = True

    def __startLogger(self):
        logFn = 'dgpulldown.log'
        if sys.platform.startswith('win'):
            AllUsersProfile = os.environ['AllUsersProfile']
            logFolder = os.path.join(AllUsersProfile, 'TheFoundation')
        else:
            logFolder = '/tmp'

        if not os.path.exists(logFolder):
            os.makedirs(logFolder)
        logFn = os.path.join(logFolder, logFn)

        self.__logger = logging.getLogger('dgpulldown')
        self.__logger.setLevel(logging.DEBUG)

        # create file handler which logs even debug messages
        self.__fh = logging.FileHandler(logFn)
        self.__fh.setLevel(logging.INFO)

        # create console handler with a higher log level
        self.__ch = logging.StreamHandler()
        self.__ch.setLevel(logging.INFO)

        # create formatter and add it to the handlers
        formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
        self.__ch.setFormatter(formatter)
        self.__fh.setFormatter(formatter)
        # add the handlers to logger
        self.__logger.addHandler(self.__ch)
        self.__logger.addHandler(self.__fh)

        self.__debugToggle()

        self.__logger.info('startup - %s' % self.__version__)

    def __debugToggle(self):
        if self.__debugMode is True:
            self.__fh.setLevel(logging.DEBUG)
            self.__ch.setLevel(logging.DEBUG)
            self.__logger.info('dgpulldown - debugMode ON')

    def dummyexit(self):
        print('dummy exit')
        sys.exit(3)

    def main(self):
        # Open input file
        try:
            self.input_fd = io.open(self.input_file, mode='rb')
        except FileNotFoundError:
            print('Could not open the inpout file!')
            sys.exit(1)

        # determine stream type: ES or program
        self.__determine_stream_type()
        if self.stream_type == self.STREAMTYPE_PROGRAM:
            print('The input file must be an elementary stream, not a program stream.')
            sys.exit(1)
        print(self.stream_type)

        # reset file position
        self.__reset_input()

        # get size of input file for progress calc
        #self.input_size = os.stat(self.input_file).st_size
        self.input_size = os.path.getsize(self.input_file)

        # check options
        self.__check_options()
        # TODO - add check of option results...

        # generate the flags sequences
        # it is easier and cleaner to pre-calculate than
        # to try to work it out on the fly in encode order
        # pre-calcuation can work in display order
        flag_pre = time.time()
        self.__generate_flags()
        flag_post = time.time()
        print('generate flags took: %s' % (flag_post - flag_pre))

        # open output file
        if self.output_file is None:
            self.output_file = '{0}.pulldown.m2v'.format(self.input_file)  # TODO - parse out input file extension
        try:
            self.output_fd = io.open(self.output_file, mode='wb')
        except:
            print('Could not open the output file %s' % self.output_file)

        # start converting
        try:
            self.__video_parser()
        except KeyboardInterrupt:
            self.__flush_output()
        except:
            traceback.print_exc()

    def __reset_input(self):
        # truncate input_buffer
        self.input_buffer.seek(0)
        self.input_buffer.truncate(0)
        # move input_fd to 0
        self.input_fd.seek(0)

    def __get_byte(self):
        """
        get a byte from file, should probably implement read buffering.
        :return:
        """

        if self.input_buffer.getbuffer().nbytes == 0:
            print('input_buffer is empty, preloading')
            #print('input_fd pre-load: %s' % self.input_fd.tell())
            self.input_buffer.write(self.input_fd.read(self.BUFFER_SIZE))
            #print('input_fd post-load: %s' % self.input_fd_pos)
            self.input_buffer.seek(0)
        if self.input_buffer.tell() == self.input_buffer.getbuffer().nbytes:
            print('input_buffer used up, refilling buffer')
            self.input_buffer.seek(0)
            self.input_buffer.truncate(0)
            #print('input_fd pre-reload: %s' % self.input_fd.tell())
            self.input_buffer.write(self.input_fd.read(self.BUFFER_SIZE))
            #print('input_fd post-reload: %s' % self.input_fd_pos)
            self.input_buffer.seek(0)
        buf = self.input_buffer.read(1)
        self.input_pos = (self.input_fd.tell() - self.input_buffer.getbuffer().nbytes) + self.input_buffer.tell()

        if self.input_eof is False:
            if (self.input_fd.tell() == self.input_size) and (self.input_buffer.tell() == self.input_buffer.getbuffer().nbytes):
                print('__get_byte: EOF!')
                self.input_eof = True

        return buf

    def __put_byte(self, val_byte):
        """
        put a byte into file, should probably implement write buffering.
        :return:
        """
        self.output_buffer.write(val_byte)
        #print('output_buffer size: %s' % self.output_buffer.getbuffer().nbytes)
        if self.output_buffer.getbuffer().nbytes > self.BUFFER_SIZE_OUT:
            print('output buffer is full, flushinig to file')
            self.output_fd.write(self.output_buffer.getbuffer())
            self.output_buffer.seek(0)
            self.output_buffer.truncate(0)
        #self.output_fd.write(val_byte)

    def __flush_output(self):
        print('flushing output buffer')
        self.output_fd.write(self.output_buffer.getbuffer())
        self.output_buffer.seek(0)  # not sure if this is really needed, but it's probably good practice
        self.output_buffer.truncate(0)

    def __video_parser(self):
        val = None
        #tc[4] = None

        # inits
        state = self.NEED_FIRST_0
        found = False
        f = 0
        F = 0
        EOF_reached = 0
        data_count = 0

        last_progress_update = time.time()
        while True:
            val = self.__get_byte()
            self.__put_byte(val)

            if time.time() - last_progress_update > 1:
                #print('input_fd pos: %s' % self.input_fd.tell())
                #print('input_buffer size: %s' % self.input_buffer.getbuffer().nbytes)
                #print('input_buffer pos: %s' % self.input_buffer.tell())
                #input_pos = abs((self.input_fd.tell() - self.input_buffer.getbuffer().nbytes)) + self.input_buffer.tell()
                #input_pos = self.input_fd_po.tell() + self.input_buffer.tell()
                #print('input_pos: %s' % self.input_pos)
                print('progress: %s' % (int(((self.input_pos / self.input_size) * 100))))
                last_progress_update = time.time()

            # original: switch(state)
            if state == self.NEED_FIRST_0:
                if val == b'\x00':
                    state = self.NEED_SECOND_0
            elif state == self.NEED_SECOND_0:
                if val == b'\x00':
                    state = self.NEED_1
                else:
                    state = self.NEED_FIRST_0
            elif state == self.NEED_1:
                if val == b'\x01':
                    found = True
                    state = self.NEED_FIRST_0
                elif val != b'\x00':
                    state = self.NEED_FIRST_0

            if found is True:
                #print('VP: found')
                #print('VP: pos: %s' % self.input_pos)
                # found a start code
                found = False
                val = self.__get_byte()
                self.__put_byte(val)

                if val == b'\xb8':
                    #print('VP: GOP')
                    # GOP
                    # next byte is 5
                    # drop_frame flag: byte 4 bit 7
                    # hour (0-23): byte 4, bit 2-6
                    # minute (0-59): byte 5 bit 4-7, byte 4 bit 0-1
                    # byte 5, bit 3 = 1
                    # second (0-59): byte 6 bit 5-7, byte 5 bit 0-2
                    # frame (0-59): byte 7 bit 7, byte 6 bit 0-4
                    # closed GOP: byte 7 bit 6
                    # broken GOP: byte 7 bit 5
                    # padding: byte7 bit 0-4
                    # TODO - set dropframe bit if going to 29.97
                    F += f
                    f = 0
                    # just read timecode
                    val = self.__get_byte()
                    self.__put_byte(val)
                    val_int = int.from_bytes(val, byteorder='little')
                    drop_frame = (val_int & 0x80) >> 7
                    minute = (val_int & 0x03) << 4
                    hour = (val_int & 0x97c) >> 2
                    val = self.__get_byte()
                    self.__put_byte(val)
                    val_int = int.from_bytes(val, byteorder='little')
                    minute |= (val_int & 0xf0) >> 4
                    sec = (val_int & 0x07) << 3
                    val = self.__get_byte()
                    self.__put_byte(val)
                    val_int = int.from_bytes(val, byteorder='little')
                    sec |= (val_int & 0xe0) >> 5
                    pict = (val_int & 0x1f) << 1
                    val = self.__get_byte()
                    self.__put_byte(val)
                    val_int = int.from_bytes(val, byteorder='little')
                    pict |= (val_int & 0x80) >> 7
                elif val == b'\x00':
                    #print('VP: Picture')
                    # Picture
                    val = self.__get_byte()
                    self.__put_byte(val)
                    val_int = int.from_bytes(val, byteorder='little')
                    ref = (val_int << 2)
                    val = self.__get_byte()
                    self.__put_byte(val)
                    val_int = int.from_bytes(val, byteorder='little')
                    ref |= (val_int >> 6)
                    D = F + ref
                    f += 1
                    if D >= self.MAX_PATTERN_LENGTH - 1:
                        # an error goes here about max file length..
                        print('Maximum filelength exceeded, aborting!')
                        self.__flush_output()
                        break
                elif (self.rate != -1) and (val == b'\xB3'):
                    print('VP: SEQ')
                    # sequence header
                    #
                    val = self.__get_byte()
                    self.__put_byte(val)
                    val = self.__get_byte()
                    self.__put_byte(val)
                    val = self.__get_byte()
                    self.__put_byte(val)
                    val = self.__get_byte()
                    val_int = int.from_bytes(val, byteorder='little')
                    print('SEQ val int: {}'.format(val_int))
                    val_int = (val_int & 0xf0) | self.rate
                    print('SEQ val int mod: {}'.format(val_int))
                    val_byte = val_int.to_bytes(1, byteorder='little')
                    print('SEQ MOD, OLD: {}, NEW: {}'.format(val, val_byte))
                    self.__put_byte(val_byte)
                elif val == b'\xb5':
                    #print('VP: extension')
                    val = self.__get_byte()
                    self.__put_byte(val)
                    val_int = int.from_bytes(val, byteorder='little')
                    #print('VP: B5 int: %s' % val_int)
                    if (val_int & 0xf0) == 0x80:  # = 0xf80 TODO - check this, i think it is wrong
                        print('VP: B5, picture coding extension')
                        # picture coding extension
                        # val/val_int is currently byte 4
                        # next byte is 5
                        # PCE layout
                        # byte 4, bit 7: 1
                        # byte 4, bit 6: 0
                        # byte 4, bit 5: 0
                        # byte 4, bit 4: 0
                        # byte 4, bit 0-3: f_code[0][0] (forward horizontal)
                        # byte 5, bit 4-7: f_code[0][1] (forward vertical)
                        # byte 5, bit 0-3: f_code[1][0] (backward horizontal)
                        # byte 6, bit 0-1: picture_structure
                        # byte 6, bit 2-3: intra_DC_precision
                        # byte 6, bit 4-7: f_code[1][1] (backward vertical)
                        # byte 7, bit 0: chroma_420_type
                        # byte 7, bit 1: Repeat_First_Field
                        # byte 7, bit 2: alternate_scan
                        # byte 7, bit 3: intra_vlc_format
                        # byte 7, bit 4: q_scale_type
                        # byte 7, bit 5: concealment_motion_vectors
                        # byte 7, bit 6: frame_pred_frame_dct
                        # byte 7, bit 7: Top_Field_First
                        # byte 8, bit 7: progressive_frame

                        # byte 8, bit 6: 0=composite_display
                        # byte 8, bit 0-5: padding 0
                        # no byte 9 or byte 10

                        # byte 8, bit 6: 1
                        # byte 8, bit 5: v_axis
                        # byte 8, bit 2-4: field_sequence
                        # byte 8, bit 1: sub_carrier
                        # byte 8 bit 0, byte 9 bit 2-7: burst_amplitude
                        # byte 9 bit 0-1, byte 10 bit 2-7: sub_carrier_phase
                        # byte 10 bit 0-1: 0, 0

                        val = self.__get_byte()
                        self.__put_byte(val)
                        val = self.__get_byte()
                        self.__put_byte(val)
                        val = self.__get_byte()
                        val_int = int.from_bytes(val, byteorder='little')
                        # rewrite trf
                        #trf = tff ? tff_flags[D] :  bff_flags[D]
                        print('D value: {}'.format(D))
                        if self.tff == 1:
                            trf = self.tff_flags[D]
                        else:
                            trf = self.bff_flags[D]
                        #trf = self.tff_flags[D]
                        val_int &= 0x7d
                        val_int |= (trf & 2) << 6
                        val_int |= (trf & 1) << 1
                        self.field_count += 2 + (trf & 1)
                        val_byte = val_int.to_bytes(1, byteorder='little')
                        self.__put_byte(val_byte)
                        # set progressive frame. this is need for RFFs to work
                        val = self.__get_byte()
                        val_int = int.from_bytes(val, byteorder='little')
                        val_int = val_int | 0x80
                        val_byte = val_int.to_bytes(1, byteorder='little')
                        self.__put_byte(val_byte)
                    elif (val_int & 0xf0) == 0x10:  # TODO - check this, i think it is wrong
                        print('VP: B5, sequence extension')
                        # sequence extension
                        # clear progressive sequence. this is need to get RFFs to work field-based
                        # current val/val_int is byte 4
                        # next byte is 5
                        # byte 4, bit 7: 0
                        # byte 4, bit 6: 0
                        # byte 4, bit 5: 0
                        # byte 4, bit 4, 1
                        # profile and level: byte 5 bit 4-7, byte 4 bit 0-3
                        # progressive_sequence: byte 5 bit 3
                        # chroma_format: byte 5 bit 1-2
                        # horizontal size extension: byte 6 bit 7, byte 5 bit 0
                        # vertical size extension: byte 6 bit 5-6
                        # bit rate extension: byte 7 bit 1-7, byte 6 bit 0-4
                        # byte 7 bit 0 = 1
                        # vbv buffer size extension: byte 8 bit 0-7
                        # low delay: byte 9, bit 7
                        # frame rate extension n: byte 9 bit 5-6
                        # frame rate extension d: byte 9 bit 0-4

                        val = self.__get_byte()
                        val_int = int.from_bytes(val, byteorder='little')
                        val_int = val_int & ~0x08
                        val_byte = val_int.to_bytes(1, byteorder='little')
                        self.__put_byte(val_byte)
            if self.input_eof is True:
                print('VP: EOF!')
                self.__flush_output()
                break

    def __determine_stream_type(self):
        val = None
        tc = {0: 0, 1: 0, 2: 0, 3: 0}
        state = self.NEED_FIRST_0
        found = False

        # Look for start codes
        self.field_count = -1
        self.rate = -1
        while (self.field_count == -1) or (self.rate == -1):
            #print('position: %s' % ((self.input_fd_pos - self.input_buffer.getbuffer().nbytes) + self.input_buffer_pos))
            print('pos: %s' % self.input_pos)
            if self.input_pos > 25:
                sys.exit(1)
            val = self.__get_byte()
            print('val: %s' % val)
            #if self.input_fd.tell() > 10240:  # TODO - fix this after adding buffering
            #    print('Unable to find stream info after 10Mb')
            #    sys.exit(0)
            if state == self.NEED_FIRST_0:
                print('STATE: NEED_FIRST_0')
                if val == b'\x00':
                    print('found 0x00')
                    state = self.NEED_SECOND_0
            elif state == self.NEED_SECOND_0:
                print('STATE: NEED_SECOND_0')
                if val == b'\x00':
                    print('found 0x00')
                    state = self.NEED_1
                else:
                    print('missed 0x00')
                    state = self.NEED_FIRST_0
            elif state == self.NEED_1:
                print('STATE: NEED_1')
                if val == b'\x01':
                    print('found 0x01')
                    found = True
                    state = self.NEED_FIRST_0
                elif val != b'\x00':
                    print('not 0x00')
                    state = self.NEED_FIRST_0

            if found is True:
                print('FOUND IS TRUE!!!!!')
                print('pos: %s' % self.input_pos)
                # found a start code
                found = False
                val = self.__get_byte()
                print('val: %s' % val)
                if val == b'\xba':
                    print('FOUND: STREAM TYPE')
                    self.stream_type = self.STREAMTYPE_PROGRAM
                    break
                elif val == b'\xb8':
                    print('FOUND: GOP')
                    # GOP
                    if self.field_count == -1:
                        #for pict = 0; pict < 4; pict ++:
                        #    tc[pict] = self.__get_byte()
                        for pict in range(0, 4):
                            tc[pict] = int.from_bytes(self.__get_byte(), byteorder='little')
                        #print(tc)
                        # TODO - these should be class level i think
                        self.drop_frame = (tc[0] & 0x80) >> 7
                        self.hour = (tc[0] & 0x7c) >> 2
                        self.minute = (tc[0] & 0x03) << 4 | (tc[1] & 0xf0) >> 4
                        self.sec = (tc[1] & 0x07) << 3 | (tc[2] & 0xe0) >> 5
                        self.pict = (tc[2] & 0x1f) << 1 | (tc[3] & 0x80) >> 7
                        self.field_count = -2
                elif val == b'\xb3':
                    print('FOUND: SEQ')
                    # sequence header
                    if self.rate == -1:
                        self.__get_byte()
                        self.__get_byte()
                        self.__get_byte()
                        val_int = int.from_bytes(self.__get_byte(), byteorder='little')
                        self.rate = val_int & 0x0f

        print('stream_type: %s' % self.stream_type)

    def __generate_flags(self):
        # TODO - rename these variables...
        dfl = (self.target_num - self.current_num) << 1
        tfl = self.current_num >> 1

        # Generate BFF & TFF flags.
        trfp = 0
        for i in range(0, self.MAX_PATTERN_LENGTH):
            tfl += dfl
            if tfl >= self.current_num:
                tfl -= self.current_num
                self.bff_flags[i] = (trfp + 1)
                trfp = (trfp ^ 2)
                # original: ((trfp ^= 2) + 1)
                # TODO - check, should this be stored with +1 or not...
                self.tff_flags[i] = trfp + 1
            else:
                self.bff_flags[i] = trfp
                self.tff_flags[i] = (trfp ^ 2)

    def __check_options(self):
        float_rates = [0.0, 23.976, 24.0, 25.0, 29.97, 30.0, 50.0, 59.94, 60.0]
        if self.rate_mode == self.CONVERT_NO_CHANGE:
            self.tfps = float_rates[wtf]  # TODO - WTF?
        elif self.rate_mode == self.CONVERT_23976_TO_29970:
            self.tfps = 29.970
            self.cfps = 23.976
            self.current_num = 24000
            self.current_den = 1001
        elif self.rate_mode == self.CONVERT_24000_TO_29970:
            self.tfps = 29.970
            self.cfps = 24.000
            self.current_num = 24
            self.current_den = 1
        elif self.rate_mode == self.CONVERT_25000_TO_29970:
            self.tfps = 29.970
            self.cfps = 25.000
            self.current_num = 25
            self.current_den = 1
        elif self.rate_mode == self.CONVERT_CUSTOM:
            raise NotImplementedError

        if math.fabs(self.tfps - 23.976) < 0.01:
            self.rate = 1
            self.rounded_fps = 24
            self.drop_frame = 0x80
            self.target_num = 24000
            self.target_den = 1001
        elif math.fabs(self.tfps - 24.000) < 0.01:
            self.rate = 2
            self.rounded_fps = 24
            self.drop_frame = 0
            self.target_num = 24
            self.target_den = 1
        elif math.fabs(self.tfps - 25.000) < 0.01:
            self.rate = 3
            self.rounded_fps = 30
            self.drop_frame = 0
            self.target_num = 25
            self.target_den = 1
        elif math.fabs(self.tfps - 29.970) < 0.01:
            self.rate = 4
            self.rounded_fps = 30
            self.drop_frame = 0x80
            self.target_num = 30000
            self.target_den = 1001
        elif math.fabs(self.tfps - 30.000) < 0.01:
            self.rate = 5
            self.rounded_fps = 30
            self.drop_frame = 0
            self.target_num = 30
            self.target_den = 1
        elif math.fabs(self.tfps - 50.000) < 0.01:
            self.rate = 6
            self.rounded_fps = 50
            self.drop_frame = 0
            self.target_num = 50
            self.target_den = 1
        elif math.fabs(self.tfps - 59.940) < 0.01:
            self.rate = 7
            self.rounded_fps = 60
            self.drop_frame = 0x80
            self.target_num = 60000
            self.target_den = 1001
        elif math.fabs(self.tfps - 60.000) < 0.01:
            self.rate = 8
            self.rounded_fps = 60
            self.drop_frame = 0
            self.target_num = 60
            self.target_den = 1
        else:
            print('Target rate is not a legal MPEG2 rate')
            sys.exit(1)


        # make currrent fps = target fps for "No change"
        if self.rate_mode == self.CONVERT_NO_CHANGE:
            self.current_num = self.target_num
            self.current_den = self.target_den
            # no reason to reset rate
            self.rate = -1

        # equate denominators
        # TODO - WTF?!?!?
        #if current_den != target_den:
        #    if current_den == 1:
        #        current_num *= (current_den = target_den)
        #    elif target_den == 1:
        #        target_num *= (target_den = current_den)
        #    else:
        #        current_num *= target_den
        #        target_num *= current_den
        #        current_den = (target_num *= current_den)

        # make divisible by two
        # TODO - WTF?!?!?
        #if (current_num & 1) or (target_num & 1):
        #    current_num <<= 1
        #    target_num <<= 1
        #    current_den = (target_den <<= 1)

        if (self.target_num - self.current_num >> 1) > self.current_num:
            print('target rate/current rate must not be greater than 1.5')
            sys.exit(1)
        elif (self.target_num < self.current_num):
            print('target rate/current rate must not be less than 1.0')
            sys.exit(1)

        # TODO - finish implementing this
        # set up df and tc vars
        #if (TimeCodes):
        #    # if the user wants to screw up the timecode... why not
        #    if DropFrames == 0:
        #        drop_frame = 0
        #    elif DropFrames == 1:
        #        drop_frame = 0x80

        # get timecode start (only if set tc is checked too, though)


if __name__ == '__main__':
    bff_help = ''
    bff_help += 'Generate a BFF (Bottom Field First) output stream. '
    bff_help += 'If absent, a TFF (Top Field First) stream is generated. '
    bff_help += 'NTSC is BFF, PAL is TFF'
    parser = argparse.ArgumentParser(description='dgpulldown(python)')
    parser.add_argument('-i', dest='input', default=None, required=True, help='File name for input file.')
    parser.add_argument('-o', dest='output', default=None, help='File name for output file, if omitted the name will be "*.pulldown.m2v".')
    parser.add_argument('-srcfps', dest='srcfps', default=None, help='Rate is any float fps value, e.g., \"23.976\" (default) or a fraction, e.g., \"30000/1001\"')
    parser.add_argument('-destfps', dest='destfps', default=None, help='Rate is any valid mpeg2 float fps value, e.g., \"29.97\" (default). Valid rates: 23.976, 24, 25, 29.97, 30, 50, 59.94, 60"')
    parser.add_argument('-df', dest='df', default=None, help='Force dropframes.')
    parser.add_argument('-nodf', dest='nodf', default=None, help='Force no dropframes.')
    parser.add_argument('-bff', dest='bff', default=None, help=bff_help)
    args = parser.parse_args()

    obj = dgpulldown(args=args)
    obj.main()
