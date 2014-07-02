/*
 * file: logger.h
 * auth: chenjianfei@daoke.me
 * date: Jun 30, 2014
 * desc:
 */

#pragma once

#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define NONE            "\x1B[m"
#define GRAY            "\x1B[0;30m"
#define LIGHT_GRAY      "\x1B[1;30m"
#define RED             "\x1B[0;31m"
#define LIGHT_RED       "\x1B[1;31m"
#define GREEN           "\x1B[0;32m"
#define LIGHT_GREEN     "\x1B[1;32m"
#define YELLOW          "\x1B[0;33m"
#define LIGHT_YELLOW    "\x1B[1;33m"
#define BLUE            "\x1B[0;34m"
#define LIGHT_BLUE      "\x1B[1;34m"
#define PURPLE          "\x1B[0;35m"
#define LIGHT_PURPLE    "\x1B[1;35m"
#define CYAN            "\x1B[0;36m"
#define LIGHT_CYAN      "\x1B[1;36m"
#define WHITE           "\x1B[0;37m"
#define LIGHT_WHITE     "\x1B[1;37m"

enum
{
    LEVEL_NONE = (-1),
    LEVEL_MIN = 0,
    LEVEL_FATAL = 0,
    LEVEL_ERROR = 1,
    LEVEL_WARN = 2,
    LEVEL_INFO = 3,
    LEVEL_DEBUG = 4,
    LEVEL_TRACE = 5,
    LEVEL_MAX = 5,
};

/*
 * open logger.
 */
int32_t open_log(const char *filename, int32_t level, int8_t is_threadsafe,
        uint64_t rotate_size);

/*
 * close logger.
 */
void close_log();

/*
 * write log.
 */
int log_write(int level, const char *fmt, ...);

/*
 * set log level.
 */
void set_log_level(int level);

#define x_out_time(x) { gettimeofday( ((struct timeval *)x), NULL ); \
	printf("time: %ld.%ld s\n", ((struct timeval *)x)->tv_sec, ((struct timeval *)x)->tv_usec); }

#if OPEN_DEBUG

#define log_debug(fmt, args...) \
    fprintf(stderr, "[DEBUG]%s(%d): " fmt "\n", __FILE__, __LINE__, ##args)
#define log_info(fmt, args...) \
    fprintf(stderr, "[INFO ]%s(%d): " fmt "\n", __FILE__, __LINE__, ##args)
#define log_warn(fmt, args...) \
    fprintf(stderr, "[WARN ]%s(%d): " fmt "\n", __FILE__, __LINE__, ##args)
#define log_error(fmt, args...) \
    fprintf(stderr, "[ERROR]%s(%d): " fmt "\n", __FILE__, __LINE__, ##args)
#define log_fatal(fmt, args...) \
    fprintf(stderr, "[FATAL]%s(%d): " fmt "\n", __FILE__, __LINE__, ##args)

#else

#define log_debug(fmt, args...) \
    log_write(LEVEL_DEBUG, "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define log_info(fmt, args...)  \
    log_write(LEVEL_INFO,  "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define log_warn(fmt, args...)  \
    log_write(LEVEL_WARN,  "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define log_error(fmt, args...) \
    log_write(LEVEL_ERROR, "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define log_fatal(fmt, args...) \
    log_write(LEVEL_FATAL, "%s(%d): " fmt, __FILE__, __LINE__, ##args)

#endif

