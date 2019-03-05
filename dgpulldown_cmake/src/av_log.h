//
// Created by jaystevens on 12/1/2018.
//

#ifndef AV_LOG_H
#define AV_LOG_H

// print no output
#define AV_LOG_QUIET   -8
// Something went really wrong and we will crash now.
#define AV_LOG_PANIC    0
// Something went wrong and recovery is not possible.
#define AV_LOG_FATAL    8
// Something went wrong and cannot losslessly be recovered.
#define AV_LOG_ERROR   16
//  Something somehow does not look correct.
#define AV_LOG_WARNING 24
// Standard information.
#define AV_LOG_INFO    32
// Detailed information.
#define AV_LOG_VERBOSE 40
// Stuff which is only useful for developers.
#define AV_LOG_DEBUG   48
// Extremely verbose debugging, useful for development.
#define AV_LOG_TRACE   56


int av_log_set_level_str(const char *level_str);
int av_log_get_level(void);
void av_log_set_level(int level);
void av_log(int level, const char *fmt, ...);
void av_log_newline(void);

#endif //AV_LOG_H
