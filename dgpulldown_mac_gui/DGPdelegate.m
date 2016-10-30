#import "DGPdelegate.h"

@implementation DGPdelegate
- (IBAction)OnConvert:(id)sender {
   // [txtOutput setIntValue:current_rb];
    //NLog(@"Selected cell is %d",current_rb);
    NSString *tmpStr;

    
//    switch (current_rb) {
//        case CONVERT_NO_CHANGE:
//            NSLog(@"Selected cell is NO CHANGE [%d]",current_rb);
//            break;
//        case CONVERT_23976_TO_29970:
//            NSLog(@"Selected cell is 23.976->29.97 [%d]",current_rb);
//            break;
//        case CONVERT_24000_TO_29970:
//            NSLog(@"Selected cell is 24->29.97 [%d]",current_rb);
//            break;
//        case CONVERT_25000_TO_29970:
//            NSLog(@"Selected cell is 25->29.97 [%d]",current_rb);
//            break;
//        case CONVERT_CUSTOM:
//            NSLog(@"Selected cell is CUSTOM [%d]",current_rb);
//            break;
//            
//        default:
//            break;
//    }
    Rate = current_rb;

    // SOURCE FILE
    tmpStr =  [txtSource stringValue];
    snprintf(input_file, [tmpStr length]+1, [tmpStr cStringUsingEncoding:NSASCIIStringEncoding]);
    
    // DESTINATION FILE
    tmpStr = [txtDest stringValue];
    snprintf(output_file, [tmpStr length]+1, [tmpStr cStringUsingEncoding:NSASCIIStringEncoding]);
    
    // CHECKBOXES
    tff = [chkTopFieldFirst state];
    inplace = [chkModifyInPlace state];
    TimeCodes = [chkTimecodes state];
    DropFrames = [chkDropFrames state];
    output_m2v = ![chkNoOutput state];
    
    if ([chkStartTime state] == YES) {
        tmpStr = [txtHH stringValue];
        snprintf(HH, [tmpStr length]+1, [tmpStr cStringUsingEncoding:NSASCIIStringEncoding]);
        tmpStr = [txtMM stringValue];
        snprintf(MM, [tmpStr length]+1, [tmpStr cStringUsingEncoding:NSASCIIStringEncoding]);   
        tmpStr = [txtSS stringValue];
        snprintf(SS, [tmpStr length]+1, [tmpStr cStringUsingEncoding:NSASCIIStringEncoding]);
        tmpStr = [txtFF stringValue];
        snprintf(FF, [tmpStr length]+1, [tmpStr cStringUsingEncoding:NSASCIIStringEncoding]);
    }
    
    // CUSTOM CONVERSION RATES
    if (Rate == 5) {
        tmpStr =  [txtCustomSource stringValue];
        snprintf(InputRate, [tmpStr length]+1, [tmpStr cStringUsingEncoding:NSASCIIStringEncoding]);
        tmpStr =  [txtCustomDest stringValue];
        snprintf(OutputRate, [tmpStr length]+1, [tmpStr cStringUsingEncoding:NSASCIIStringEncoding]);
    }
    
    // DISABLE BUTTONS
    [btnSource setEnabled:false];
    [btnDest setEnabled:false];
    [btnConvert setEnabled:false];
    
    
    // LAUNCH THREAD
    [NSThread detachNewThreadSelector:@selector(LaunchProcess) toTarget:self withObject:nil];
}

- (void) LaunchProcess {
    threadStatus = true;
    [self process];
    
    NSRunAlertPanel(@"Process Complete", @"The Pulldown Process Has Completed", @"Ok", nil, nil);
    
    // ENABLE BUTTONS
    [btnSource setEnabled:true];
    [btnDest setEnabled:true];
    [btnConvert setEnabled:true];
}

- (IBAction)OnHelp:(id)sender {
    // OPEN PDF HELP FILE
    [[NSWorkspace sharedWorkspace] openFile:[[NSBundle mainBundle] pathForResource:@"Help" ofType:@"pdf"]];
}

- (IBAction)OnCustom:(id)sender {
    customState = [sender state];
}

- (IBAction)OnSetStartTime:(id)sender {
    // START_TIME 
    if ([sender state] == YES) {
        [txtHH setEnabled:YES];
        [txtMM setEnabled:YES];
        [txtSS setEnabled:YES];
        [txtFF setEnabled:YES];
    }else{
        [txtHH setEnabled:NO];
        [txtMM setEnabled:NO];
        [txtSS setEnabled:NO];
        [txtFF setEnabled:NO];
    }

}

// SAVE DIALOG
- (IBAction)OnDest:(id)sender {
    int result;
    
    NSSavePanel *sPanel = [NSSavePanel savePanel];
    [sPanel setRequiredFileType:@"m2v"];
    
//    NSString* fName = [[txtSource stringValue] stringByDeletingPathExtension];
//    NSLog(fName);
    result = [sPanel runModalForDirectory:nil file:nil];
    
    if (result == NSOKButton){
        [txtDest setStringValue:[sPanel filename]];
    }
}

// OPEN DIALOG
- (IBAction)OnSource:(id)sender {
    int result;   
    // create the dialog
    NSOpenPanel *oPanel = [NSOpenPanel openPanel];
    NSArray *fileTypes = [NSArray arrayWithObject:@"m2v"];
    NSString *newExt = @"-pulldown.m2v";
    // set the dialog parameters
    [oPanel setAllowsMultipleSelection:NO];
    [oPanel setCanChooseFiles:YES];
    [oPanel setTitle:@"Select the Source m2v file..."];
    [oPanel setCanChooseDirectories:NO];
    
    // launch the dialog
    result = [oPanel runModalForDirectory:NSHomeDirectory() file:nil types:fileTypes]; //runModalForDirectory:NSHomeDirectory()file:nil types:nil];
    
    // if "open" button is clicked, then process the returned filename(s)
    if ( result == NSOKButton) {
        NSArray *files = [oPanel filenames];
        NSString *chosenFilename = [files objectAtIndex:0];
        
        [txtSource setStringValue:chosenFilename];
//        [txtDvdTitle setStringValue:[(NSString *)chosenFilename lastPathComponent]];
        NSString *newFileName = [chosenFilename stringByDeletingPathExtension];
        [txtDest setStringValue:[newFileName stringByAppendingString:newExt]];
        [btnConvert setEnabled:YES];
        [btnDest setEnabled:YES];

    } 
    
    
    
}    


- (IBAction)OnAbout:(id)sender {
    
}


// RUN WHEN GUI IS UNACHIVED FROM NIB FILE
-(void)awakeFromNib {
    [NSApp setDelegate:self];
	[mainWindow center]; 
	[mainWindow makeKeyAndOrderFront: nil];
    [self setPbText:@"0"];
    //NSLog(@"%i",[chkTopFieldFirst state]);
}

// Exit Application when Window is Closed
- (BOOL) applicationShouldTerminateAfterLastWindowClosed: (NSApplication *) theApplication;{
    return YES;
}

// RADIO BUTTON HANDLER
-(IBAction)OnRBChange:(id)sender {
    current_rb = [[sender selectedCell] tag];    
    
    if (current_rb == 5) {
        [txtCustomSource setEnabled:YES];
        [txtCustomDest setEnabled:YES];
    }else{
        [txtCustomSource setEnabled:NO];
        [txtCustomDest setEnabled:NO];
    }
}

// SETS PROGRESS BAR VALUE
- (void) setPbValue:(double)pVal {
    pbValue = pVal;
}

// SETS PROGRESS BAR LABEL TEXT/VALUE
- (void) setPbText:(NSString*)pValText {
    pbValueText = pValText;
    NSString* nString = [pValText stringByAppendingString:@"% Complete"];
    [progressBarText setStringValue:nString];
}

// threadStatus IS USED TO DETERMINE STATE OF THREAD
- (void) KillThread
{
    threadStatus = false;
}


// ACTUAL WORK IS DONE HERE
- (int) process
{
    NSAutoreleasePool*    pool = [[NSAutoreleasePool alloc] init];
	F = 0;
	field_count = 0;
	pict = 0;
	sec = 0;
	minute = 0;
	hour = 0;
	drop_frame = 0;
	Start = time(NULL); //timeGetTime();
    // printf("%i\n",Start);
    
	// Open the input file.
	if (inplace)
	{
		if (input_file[0] == 0 || (wfd = open(input_file, O_RDWR)) == -1)
		{
            //			MessageBox(hWnd, "Could not open the input file!", "Error", MB_OK);
            NSRunCriticalAlertPanel(@"ERROR!",@"Could not open the input file!",@"OK",nil,nil);
            // printf("%s\n","Could not open the input file!");
			[self KillThread];
		}
		// Read the first chunk of data.
		Read = read(wfd, buffer, BUFFER_SIZE);
		Rdptr = buffer - 1;
	}
	else
	{
		if (input_file[0] == 0 || (fp = fopen(input_file, "rb")) == NULL)
		{
            //			MessageBox(hWnd, "Could not open the input file!", "Error", MB_OK);
            NSRunCriticalAlertPanel(@"ERROR!",@"Could not open the input file!",@"OK",nil,nil);
            // printf("%s\n","Could not open the input file!");
			[self KillThread];
            return false;
		}
		setvbuf(fp, NULL, _IOFBF, 0);
	}
    
	// Determine the stream type: ES or program.
	[self determine_stream_type];
	if (stream_type == PROGRAM)
	{
        //		MessageBox(hWnd, "The input file must be an elementary\nstream, not a program stream.", "Error", MB_OK);
        NSRunCriticalAlertPanel(@"ERROR!",@"The input file must be an elementary\nstream, not a program stream.",@"OK",nil,nil);
        // printf("%s\n","The input file must be an elementary\nstream, not a program stream.");
		[self KillThread];
        return false;
	}
    
	// Re-open the input file.
	if (inplace)
	{
		close(wfd);
		wfd = open(input_file, O_RDWR);
		if (wfd == -1)
		{
            //			MessageBox(hWnd, "Could not open the input file!", "Error", MB_OK);
            NSRunCriticalAlertPanel(@"ERROR!",@"Could not open the input file!",@"OK",nil,nil);
            // printf("%s\n","Could not open the input file!");
			[self KillThread];
            return false;
		}
		Read = read(wfd, buffer, BUFFER_SIZE);
		Rdptr = buffer - 1;
	}
	else
	{
		fclose(fp);
		fp = fopen(input_file, "rb");
		if (fp == NULL)
		{
            //			MessageBox(hWnd, "Could not open the input file!", "Error", MB_OK);
            NSRunCriticalAlertPanel(@"ERROR!",@"Could not open the input file!",@"OK",nil,nil);
            // printf("%s\n","Could not open the input file!");
			[self KillThread];
            return false;
		}
		setvbuf(fp, NULL, _IOFBF, 0);
	}
    
	// Get the file size.
	fd = open(input_file, O_RDONLY);
	size = lseek(fd, 0, SEEK_END);
	close(fd);
    
	// Make sure all the options are ok
	if (![self check_options]) 
	{
		[self KillThread];
        return false;
	}
    
	// Open the output file.
	if (output_m2v && !inplace)
	{
		wfp = fopen(output_file, "wb");
		if (wfp == NULL)
		{
			char buf[80];
			//sprintf(buf, "Could not open the output file\n%s!", output_file);
            //			MessageBox(hWnd, buf, "Error", MB_OK);
            NSRunCriticalAlertPanel(@"ERROR!",@"Could not open the output file!",@"OK",nil,nil);
            // printf("%s\n",buf);
			[self KillThread];
            return false;
		}
		setvbuf(wfp, NULL, _IOFBF, 0);
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
            //			MessageBox(hWnd, "Could not open the timecode file!", "Error", MB_OK);
            NSRunCriticalAlertPanel(@"ERROR!",@"Could not open the timecode file!",@"OK",nil,nil);
            // printf("%s\n","Could not open the timecode file!");
			[self KillThread];
            return false;
		}
		fprintf(dfp,"   field   frame  HH:MM:SSdFF\n");
	}
    
	// Generate the flags sequences.
	// It's easier and cleaner to pre-calculate than
	// to try to work it out on the fly in encode order.
	// Pre-calculation can work in display order.
	[self generate_flags];
    
	// Start converting.
	[self video_parser];
    [pool release];
    
	return 0;
}

// Write a value in the file at an offset relative
// to the last read character. This allows for
// read ahead (write behind) by using negative offsets.
// Currently, it's only used for the 4-byte timecode.
// The offset is ignored when not doing in-place operation.
- (void) put_byte:(int) offset: (unsigned char) val
{
	int64_t save, backup;
    
	if (inplace)
	{
		// Save the file position.
		save = lseek(wfd, 0, SEEK_CUR);
		// Calculate the amount to seek back to write this value.
		// offset is relative to the position of the last read value.
		backup = buffer + Read - Rdptr - offset;
		// Seek back.
		lseek(wfd, -backup, SEEK_CUR);
		// Write the value.
		write(wfd, &val, 1);
		// Restore the file position to be ready for the next
		// read buffer refill.
		lseek(wfd, save, SEEK_SET);
	}
	else
	{
		fwrite(&val, 1, 1, wfp);
	}
}

// Get a byte from the stream. We do our own buffering
// for in-place operation, otherwise we rely on the
// operating system.
- (unsigned char) get_byte
{
	unsigned char val;
    double pVal;
    
    
	if (inplace)
	{
		if (Rdptr > &buffer[Read-2])
		{
			// Incrementing to the next byte will take us outside the buffer,
			// so we need to refill it.
			Read = read(wfd, buffer, BUFFER_SIZE);
			Rdptr = buffer;
			if (Read < 1)
			{
				// No more data. Finish up.
				close(wfd);
				if (Debug) fclose(dfp);
                //				SendDlgItemMessage(hWnd, IDC_PROGRESS, PBM_SETPOS, 100, 0);
				[self KillThread];
                // printf("No More Data. Done\n");
                
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
            //			SendDlgItemMessage(hWnd, IDC_PROGRESS, PBM_SETPOS, 100, 0);
			[self KillThread];
            // printf("Closeing Files. Done\n");
            
		}
	}
	if (!(data_count++ % 524288) && (size > 0))
	{
		Now = time(NULL); //timeGetTime();
		if (Now >= Start)
			Elapsed = Now - Start;
		else
			Elapsed = Now - Start; //(4294967295 - Start) + Now + 1;
            pVal = (double)(data_count * 100.00 / size);
            NSString *pValText =[NSString stringWithFormat:@"%.0f",pVal];
            [self setPbText:pValText];
            //NSLog(pValText);
            [self setPbValue: pVal];
            //sprintf(stats,"%7d input frames : output time %02d:%02d:%02d%c%02d @ %6.3f [elapsed %d sec]",
			//    F, hour, minute, sec, (drop_frame?',':'.'), pict, tfps, Elapsed);
            sleep(0.02);
	}
    
	return val;
}

- (void) video_parser
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
        if (threadStatus == false) {
            
            break;
        }
		// Parse for start codes.
		val = [self get_byte];
        
	    if (output_m2v && !inplace) [self put_byte: 0: val];
        
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
			val = [self get_byte];
			if (output_m2v && !inplace) [self put_byte: 0: val];
            
			if (val == 0xb8)
			{
				// GOP.
				F += f;
				f = 0;
				if (set_tc)
				{
					// Timecode and drop frame
					for(pict=0; pict<4; pict++) 
						tc[pict] = [self get_byte];
                    
					//determine frame->tc
					pict = field_count >> 1;
					if (drop_frame) pict += 18*(pict/17982) + 2*((pict%17982 - 2) / 1798);
					pict -= (sec = pict / rounded_fps) * rounded_fps; 
					sec -= (minute = sec / 60) * 60; 
					minute -= (hour = minute / 60) * 60;
					hour %= 24;
					//now write timecode
					val = drop_frame | (hour << 2) | ((minute & 0x30) >> 4);
					if (output_m2v) [self put_byte: -3: val];
					val = ((minute & 0x0f) << 4) | 0x8 | ((sec & 0x38) >> 3);
					if (output_m2v) [self put_byte:-2: val];
					val = ((sec & 0x07) << 5) | ((pict & 0x1e) >> 1);
					if (output_m2v) [self put_byte: -1: val];
					val = (tc[3] & 0x7f) | ((pict & 0x1) << 7);
					if (output_m2v) [self put_byte: 0: val];
				}
				else
				{
					//just read timecode
					val = [self get_byte];
					if (output_m2v && !inplace) [self put_byte: 0: val];
					drop_frame = (val & 0x80) >> 7;
					minute = (val & 0x03) << 4;
					hour = (val & 0x7c) >> 2;
					val = [self get_byte];
					if (output_m2v && !inplace) [self put_byte: 0: val];
					minute |= (val & 0xf0) >> 4;
					sec = (val & 0x07) << 3;
					val = [self get_byte];
					if (output_m2v && !inplace) [self put_byte: 0: val];
					sec |= (val & 0xe0) >> 5;
					pict = (val & 0x1f) << 1;
					val = [self get_byte];
					if (output_m2v && !inplace) [self put_byte: 0: val];
					pict |= (val & 0x80) >> 7;
				}
				if (Debug) fprintf(dfp,"%7d  %02d:%02d:%02d%c%02d\n",F,hour,minute,sec,(drop_frame?',':'.'),pict);
			}
			else if (val == 0x00)
			{
				// Picture.
				val = [self get_byte];
				if (output_m2v && !inplace) [self put_byte: 0: val];
				ref = (val << 2);
				val = [self get_byte];
				if (output_m2v && !inplace) [self put_byte: 0: val];
				ref |= (val >> 6);
				D = F + ref;
				f++;
				if (D >= MAX_PATTERN_LENGTH - 1)
				{
					if (inplace)
						close(wfd);
					else
					{
						fclose(fp);
						if (output_m2v)
							fclose(wfp);
					}
                    //					MessageBox(hWnd, "Maximum filelength exceeded, aborting!", "Error", MB_OK);
                    NSRunCriticalAlertPanel(@"ERROR!",@"Maximum filelength exceeded, aborting!",@"OK",nil,nil);
                    // printf("%s\n","Maximum filelength exceeded, aborting!");
					[self KillThread];
                    return;
				}
			}
			else if ((rate != -1) && (val == 0xB3))
			{
				// Sequence header.
				val = [self get_byte];
				if (output_m2v && !inplace) [self put_byte: 0: val];
				val = [self get_byte];
				if (output_m2v && !inplace) [self put_byte: 0: val];
				val = [self get_byte];
				if (output_m2v && !inplace) [self put_byte: 0: val];
				val = ([self get_byte] & 0xf0) | rate;
				if (output_m2v) [self put_byte: 0: val];
			}
			else if (val == 0xB5)
			{
				val = [self get_byte];
				if (output_m2v && !inplace) [self put_byte: 0: val];
				if ((val & 0xf0) == 0x80)
				{
					// Picture coding extension.
					val = [self get_byte];
					if (output_m2v && !inplace) [self put_byte: 0: val];
					val = [self get_byte];
					if (output_m2v && !inplace) [self put_byte: 0: val];
					val = [self get_byte];
					//rewrite trf
					trf = tff ? tff_flags[D] : bff_flags[D];
					val &= 0x7d;
					val |= (trf & 2) << 6;
					val |= (trf & 1) << 1;
					field_count += 2 + (trf & 1);
					if (output_m2v) [self put_byte: 0: val];
					// Set progressive frame. This is needed for RFFs to work.
					val = [self get_byte] | 0x80;
					if (output_m2v) [self put_byte: 0: val];
				}
				else if ((val & 0xf0) == 0x10)
				{
					// Sequence extension
					// Clear progressive sequence. This is needed to
					// get RFFs to work field-based.
					val = [self get_byte] & ~0x08;
					if (output_m2v) [self put_byte: 0: val];
				}
			}
		}
	}
	return;
}

- (void) determine_stream_type
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
		val = [self get_byte];
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
			val = [self get_byte];
			if (val == 0xba)
			{
				stream_type = PROGRAM;
				break;
			}
			else if (val == 0xb8)
			{
				// GOP.
				if (field_count == -1) {
					for(pict=0; pict<4; pict++) tc[pict] = [self get_byte];
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
					[self get_byte];
					[self get_byte];
					[self get_byte];
					rate = [self get_byte] & 0x0f;
				}
			}
		}
        
	}
}

- (bool) check_options
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
			current_num = (int64_t) (f * 1000000.0);
			current_den = 1000000;
		}
        
		if(0) { //debug
			sprintf(buf, "%I64Ld", current_num);
            //			MessageBox(hWnd, buf, "Current fps numerator", MB_OK);
            printf("Current fps numerator %s\n",buf);
			sprintf(buf, "%I64Ld", current_den);
            //			MessageBox(hWnd, buf, "Current fps denominator", MB_OK);
			return false;
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
        //		MessageBox(hWnd, "Target rate is not a legal MPEG2 rate", "Error", MB_OK);
        NSRunCriticalAlertPanel(@"ERROR!",@"Target rate is not a legal MPEG2 rate!",@"OK",nil,nil);
        // printf("%s\n","Target rate is not a legal MPEG2 rate");
		return false;
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
        //		MessageBox(hWnd, buf, "Current fps numerator", MB_OK);
		sprintf(buf, "%I64Ld", current_den);
        //		MessageBox(hWnd, buf, "Current fps denominator", MB_OK);
		sprintf(buf, "%I64Ld", target_num);
        //		MessageBox(hWnd, buf, "Target fps numerator", MB_OK);
		sprintf(buf, "%I64Ld", target_den);
        //		MessageBox(hWnd, buf, "Target fps denominator", MB_OK);
		return false;
	}
    
	if (((target_num - current_num) >> 1) > current_num)
	{
        //		MessageBox(hWnd, "target rate/current rate\nmust not be greater than 1.5", "Error", MB_OK);
        NSRunCriticalAlertPanel(@"ERROR!",@"Target Rate/Current Rate must not be greater than 1.5!",@"OK",nil,nil);
        // printf("%s\n","target rate/current rate must not be greater than 1.5");
		return false;
	}
	else if (target_num < current_num)
	{
        //		MessageBox(hWnd, "target rate/current rate\nmust not be less than 1.0", "Error", MB_OK);
        NSRunCriticalAlertPanel(@"ERROR!",@"Target Rate/Current Rate must not be less than 1.0!",@"OK",nil,nil);
        // printf("%s\n","target rate/current rate must not be less than 1.0");
		return false;
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
    //	SetDlgItemText(hWnd, IDC_HH, buf);
	strcpy(HH, buf);
	sprintf(buf, "%02d",minute);
    //	SetDlgItemText(hWnd, IDC_MM, buf);
	strcpy(MM, buf);
	sprintf(buf, "%02d",sec);
    //	SetDlgItemText(hWnd, IDC_SS, buf);
	strcpy(SS, buf);
	sprintf(buf, "%02d",pict);
    //	SetDlgItemText(hWnd, IDC_FF, buf);
	strcpy(FF, buf);
	return true;
}

- (void) generate_flags

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

@end
