#import <Cocoa/Cocoa.h>
//#import "dg2.h"

@interface DGPdelegate : NSObject {
    IBOutlet id chkDropFrames;
    IBOutlet id chkModifyInPlace;
    IBOutlet id chkNoOutput;
    IBOutlet id chkStartTime;
    IBOutlet id chkTimeCodeToFile;
    IBOutlet id chkTimecodes;
    IBOutlet id chkTopFieldFirst;
    IBOutlet id chkCustom;
    IBOutlet id txtCustomDest;
    IBOutlet id txtCustomSource;
    IBOutlet id txtDest;
    IBOutlet id txtFF;
    IBOutlet id txtHH;
    IBOutlet id txtMM;
    IBOutlet id txtOutput;
    IBOutlet id txtSS;
    IBOutlet id txtSource;
    IBOutlet id mainWindow;
    IBOutlet id btnConvert;
    IBOutlet id btnSource;
    IBOutlet id btnDest;
    IBOutlet id progressBar;
    IBOutlet id progressBarText;
    double pbValue;
    int customState, startState;
    NSString *pbValueText;
}
- (IBAction)OnConvert:(id)sender;
- (IBAction)OnSetStartTime:(id)sender;
- (IBAction)OnDest:(id)sender;
- (IBAction)OnSource:(id)sender;
- (IBAction)OnAbout:(id)sender;
- (IBAction)OnHelp:(id)sender;
- (IBAction)OnRBChange:(id)sender;
- (IBAction)OnHelp:(id)sender;
- (IBAction)OnCustom:(id)sender;
- (void) LaunchProcess;
- (bool) check_options;
- (void) determine_stream_type;
- (void) video_parser;
- (void) generate_flags;
- (void) put_byte:(int) offset: (unsigned char) val;
- (unsigned char) get_byte;
- (int) process;
- (void) setPbValue:(double)pVal;
- (void) setPbText:(NSString*)pValText;
int current_rb=2;


unsigned int Rate;




#define ES 0
#define PROGRAM 1
int stream_type;


char input_file[2048];
char output_file[2048];

typedef enum CONVERT {
    CONVERT_NO_CHANGE=1,
    CONVERT_23976_TO_29970,
    CONVERT_24000_TO_29970,
    CONVERT_25000_TO_29970,
    CONVERT_CUSTOM    
}CONVERT;

int hThread;
int threadId;

unsigned long pOldEditProc;

unsigned int CliActive = 0;

unsigned int Rate; // = CONVERT_23976_TO_29970;
char InputRate[255];
char OutputRate[255];
unsigned int TimeCodes = 1;
unsigned int DropFrames = 2;
unsigned int StartTime = 0;
char HH[255] = {0}, MM[255] = {0}, SS[255] = {0}, FF[255] = {0};
unsigned int Debug = 0;
int inplace = 0;
int tff = 1;
static int output_m2v = 1;


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

#define MAX_PATTERN_LENGTH 2000000
unsigned char bff_flags[MAX_PATTERN_LENGTH];
unsigned char tff_flags[MAX_PATTERN_LENGTH];

// For progress bar.
int64_t size;
int64_t data_count;

static int state, found;
static int f, F, D;
static int ref;
int EOF_reached;
float tfps, cfps;
int64_t current_num, current_den, target_num, target_den;
int rate;
int rounded_fps;

int field_count, pict, sec, minute, hour, drop_frame;
int set_tc;

char stats[255];
char done[255];
unsigned int Start, Now, Elapsed;
int pbValue;

bool threadStatus;

@end
