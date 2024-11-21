# DGPulldown2 - python

This is a pure python port of DGPulldown algorithm.  
This is a work in progress.

### Description

dgpulldown2_python will:

-  Apply a pulldown pattern to a progressive source by modifying the RFF/TFF flags in accordance with [MPEG2Video/H.262][1].  Although the latest MPEG2Video/H.262 Specification is not available publicly from an official source, the ITU does make an older version of the specification freely available.  For example, when converting from 23.976fps to 29.970fps, dgpulldown2 will apply 2:3 (Classic) pulldown.
-  The input must be an MPEG2Video elementary stream.  The output will also be an MPEG2Video elementary stream.  The common file extension for a MPEG2Video elementary stream is `*.m2v`.  MPEG2Video elementary streams are sometime referred to as 'raw MPEG2 Video'.
- Modify the `frame_rate_code` in the MPEG2Video Sequence Header.  For example, when converting from 23.976fps to 29.970fps, dgpulldown2 will modify the frame_rate_code from `"1" (24000/1001)` to `"4" (30000/1001)`


### Usage

Generate an MPEG2video/H.262 elementary stream with FFmpeg at 23.976fps

```shell
$ ffmpeg -hide_banner \
  -f 'lavfi' -i "testsrc2=size='ntsc':rate='ntsc-film',setdar=ratio='(4/3)',trim=end_frame=12" \
  -map '0:v:0' -codec:v 'mpeg2video' -g 12 \
  -bf:v 2 -b_strategy 0 \
  -q:v 2 -maxrate:v 8000000 -minrate:v 0 -bufsize:v 1835008 \
  -f 'mpeg2video' "./test.m2v" -y
```
Use dgpulldown2_python to modify the RFF/TFF flags.

```shell
$ python3 dgpulldown2.py -i "./test.m2v" -o "./test.pulldown.m2v" -srcfps 24000/1001 -destfps 29.97
```


```text
$ python3 ./dgpulldown2.py -h
# or...
$ pypy3 ./dgpulldown2.py -h

usage: dgpulldown2.py [-h] -i INPUT [-o OUTPUT] [-srcfps SRCFPS] [-destfps DESTFPS] [-df DF] [-nodf NODF] [-bff BFF]

dgpulldown(python)

options:
  -h, --help        show this help message and exit
  -i INPUT          File name for input file.
  -o OUTPUT         File name for output file, if omitted the name will be "*.pulldown.m2v".
  -srcfps SRCFPS    Rate is any float fps value, e.g., "23.976" (default) or a fraction, e.g., "30000/1001"
  -destfps DESTFPS  Rate is any valid mpeg2 float fps value, e.g., "29.97" (default). Valid rates: 23.976, 24, 25,
                    29.97, 30, 50, 59.94, 60"
  -df DF            Force dropframes.
  -nodf NODF        Force no dropframes.
  -bff BFF          Generate a BFF (Bottom Field First) output stream. If absent, a TFF (Top Field First) stream is
                    generated. NTSC is BFF, PAL is TFF
```

## pypy3 vs python3

Using dgpulldown2 with `pypy3` appears runs about three times faster than using dgpulldown2 with `python3`.

## Validation

Output can be validated with:

```shell
$ mediainfo "./test.pulldown.m2v" | grep -e "Original\ frame\ rate" -e "Scan\ type" -e "Scan\ order"
```

```shell
$ ffprobe -hide_banner "./test.pulldown.m2v" -select_streams 'v:0' -show_entries frame='key_frame,pts,pict_type,interlaced_frame,top_field_first,repeat_pict' -print_format 'compact'
```

MPEG2Video (m2v) Headers can be inspected with [VTCLab Media Analyzer](https://media-analyzer.pro/app)
- MPEGVideo Sequence Header
- MPEGVideo Sequence Extension
- MPEGVideo Group Of Pictures Header
- MPEGVideo Picture Header
- MPEGVideo Picture Coding Extension

### Copyright & Credit

Repository: [https://github.com/jaystevens/dgpulldown](https://github.com/jaystevens/dgpulldown)
Copyright(c): 2016 Jason Stevens

[1]: https://www.itu.int/rec/T-REC-H.262-200002-S/en

