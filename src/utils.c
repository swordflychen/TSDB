#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include "main.h"
#include "read_cfg.h"
#include "utils.h"


char *x_strdup(const char *src)
{ 
    if (src == NULL)
        return NULL;

    int len = strlen(src);
    char *out = (char *)calloc(len + 1, sizeof(char));
    strcpy(out, src);
    return out; 
}


int get_overplus_time(void)
{
    static int g_mon = -1;

    time_t t_now = time(NULL);
    struct tm *p_tm = localtime(&t_now);
    struct tm tm = {0};
    if (g_mon != p_tm->tm_mon) {
        g_mon = p_tm->tm_mon;
        x_printf("current month:             %d\n", g_mon);
        return 0;
    }
    tm.tm_mon = (p_tm->tm_mon + 1) % 12;
    tm.tm_year = (p_tm->tm_mon == 11) ? (p_tm->tm_year + 1):( p_tm->tm_year);
    time_t t_end = timelocal(&tm);
    int space = t_end - t_now;
    return (space > 0)?space:1;
}

int get_current_time(void)
{
    struct timeval tv = {0};
    gettimeofday(&tv, NULL);
    return (tv.tv_sec + 8*60*60) % (ONE_DAY_TIMESTAMP);
}

/*********************************LOGS***********************************************/
extern struct cfg_info g_cfg_pool;
//struct cfg_info g_cfg_pool;
static int g_new_log_file = 0;
static int g_old_log_file = 0;

void open_new_log(void)
{
    /*recode old file*/
    g_old_log_file = g_new_log_file;
    /*open new file*/
    time_t t_now = time(NULL);
    struct tm *p_tm = localtime(&t_now);
    char name[512] = {0};
    snprintf(name, (sizeof(name) - 1), "%s%s%s_%04d%02d.log", g_cfg_pool.log_path, "/", g_cfg_pool.log_file, 1900 + p_tm->tm_year, 1 + p_tm->tm_mon);
    g_new_log_file = open(name, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR|S_IWUSR);

    if(-1 == g_new_log_file)
    {
        x_printf("failed to open log file: %s\n", name);
        fprintf(stderr, "failed to open log file: %s.\n", name);
        exit(1);
    }
}

void close_old_log(void)
{
    close(g_old_log_file);
}

void dyn_log( int level, const char *fmt, ... )
{
    if (level > g_cfg_pool.log_verbosity)
        return; /* too verbose */

    /* get current time and log level */
    time_t now = time(NULL);
    char time_buf[32] = {0};
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    static const char lv[] = "EWINDA";
    dprintf(g_new_log_file, "%s|%c|", time_buf, lv[level]);

    /* write message. */
    va_list ap;
    va_start(ap, fmt);
    vdprintf(g_new_log_file, fmt, ap);
    va_end(ap);

#if 0
    /* write to log and flush to disk. */
    fsync(g_new_log_file);
    //fdatasync(g_new_log_file);
#endif
}

void check_pid_file(void)
{
    char pidfile[512] = {0};
    snprintf(pidfile, (sizeof(pidfile) - 1), "%s%s", g_cfg_pool.work_path, "/pidfile");
    if(access(pidfile, F_OK) == 0){
        fprintf(stderr, "Fatal error!\nPidfile %s already exists!\n"
                "You must kill the process and then "
                "remove this file before starting ssdb-server.\n", pidfile);
        exit(1);
    }
}

void write_pid_file(void)
{
    char pidfile[512] = {0};
    snprintf(pidfile, (sizeof(pidfile) - 1), "%s%s", g_cfg_pool.work_path, "/pidfile");
    FILE *fp = fopen(pidfile, "w");

    if(!fp){
        x_perror("failed to open pid file: %s\n", pidfile);
        exit(1);
    }

    char buf[128];
    pid_t pid = getpid();
    snprintf(buf, sizeof(buf), "%d", pid);
    x_printf("pidfile: %s, pid: %d\n", pidfile, pid);
    fwrite(buf, 1, strlen(buf), fp);
    fclose(fp);
}

void remove_pid_file(void)
{
    char pidfile[512] = {0};
    snprintf(pidfile, (sizeof(pidfile) - 1), "%s%s", g_cfg_pool.work_path, "/pidfile");

    if(strlen(pidfile)){
        remove(pidfile);
    }
}


int string_match_len(const char *pattern, int patternLen,
        const char *string, int stringLen, int nocase)
{
    while(patternLen) {
        switch(pattern[0]) {
            case '*':
                while (pattern[1] == '*') {
                    pattern++;
                    patternLen--;
                }
                if (patternLen == 1)
                    return 1; /* match */
                while(stringLen) {
                    if (string_match_len(pattern+1, patternLen-1,
                                string, stringLen, nocase))
                        return 1; /* match */
                    string++;
                    stringLen--;
                }
                return 0; /* no match */
                break;
            case '?':
                if (stringLen == 0)
                    return 0; /* no match */
                string++;
                stringLen--;
                break;
            case '[':
                {
                    int no, match;

                    pattern++;
                    patternLen--;
                    no = pattern[0] == '^';
                    if (no) {
                        pattern++;
                        patternLen--;
                    }
                    match = 0;
                    while(1) {
                        if (pattern[0] == '\\') {
                            pattern++;
                            patternLen--;
                            if (pattern[0] == string[0])
                                match = 1;
                        } else if (pattern[0] == ']') {
                            break;
                        } else if (patternLen == 0) {
                            pattern--;
                            patternLen++;
                            break;
                        } else if (pattern[1] == '-' && patternLen >= 3) {
                            int start = pattern[0];
                            int end = pattern[2];
                            int c = string[0];
                            if (start > end) {
                                int t = start;
                                start = end;
                                end = t;
                            }
                            if (nocase) {
                                start = tolower(start);
                                end = tolower(end);
                                c = tolower(c);
                            }
                            pattern += 2;
                            patternLen -= 2;
                            if (c >= start && c <= end)
                                match = 1;
                        } else {
                            if (!nocase) {
                                if (pattern[0] == string[0])
                                    match = 1;
                            } else {
                                if (tolower((int)pattern[0]) == tolower((int)string[0]))
                                    match = 1;
                            }
                        }
                        pattern++;
                        patternLen--;
                    }
                    if (no)
                        match = !match;
                    if (!match)
                        return 0; /* no match */
                    string++;
                    stringLen--;
                    break;
                }
            case '\\':
                if (patternLen >= 2) {
                    pattern++;
                    patternLen--;
                }
                /* fall through */
            default:
                if (!nocase) {
                    if (pattern[0] != string[0])
                        return 0; /* no match */
                } else {
                    if (tolower((int)pattern[0]) != tolower((int)string[0]))
                        return 0; /* no match */
                }
                string++;
                stringLen--;
                break;
        }
        pattern++;
        patternLen--;
        if (stringLen == 0) {
            while(*pattern == '*') {
                pattern++;
                patternLen--;
            }
            break;
        }
    }
    if (patternLen == 0 && stringLen == 0)
        return 1;
    return 0;
}

int string_match(const char *pattern, const char *string, int nocase)
{
    return string_match_len(pattern,strlen(pattern),string,strlen(string),nocase);
}


int prefix_match_len(const char *pre, int preLen, const char *string, int stringLen)
{
    if (preLen > stringLen) {
        return 0;
    }

    if (strncmp(pre, string, preLen) == 0) {
        return 1;
    }

    return 0;
}
