#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include "utils.h"
#include "main.h"
#include "read_cfg.h"
#include "logger.h"


char *x_strdup(const char *src)
{ 
    if (src == NULL)
        return NULL;

    int len = strlen(src);
    char *out = (char *)calloc(len + 1, sizeof(char));
    strcpy(out, src);
    return out; 
}

extern struct cfg_info g_cfg_pool;


int get_current_time(void)
{
    struct timeval tv = {0};
    gettimeofday(&tv, NULL);
    return (tv.tv_sec + 8*60*60) % (ONE_DAY_TIMESTAMP);
}



void open_new_log(void)
{
    char name[512] = {0};
    snprintf(name, (sizeof(name) - 1), "%s%s%s.log", g_cfg_pool.log_path, "/", g_cfg_pool.log_file);

    open_log(name, g_cfg_pool.log_verbosity, 1, 256*1024*1024);
}

void close_old_log(void)
{
	close_log();
}



void check_pid_file(void)
{
    char pidfile[512] = {0};
    snprintf(pidfile, (sizeof(pidfile) - 1), "%s%s", g_cfg_pool.work_path, "/pidfile");
    if(access(pidfile, F_OK) == 0){
        fprintf(stderr, "Fatal error!\nPidfile %s already exists!\n"
                "You must kill the process and then "
                "remove this file before starting tsdb-server.\n", pidfile);
        exit(1);
    }
}

void write_pid_file(void)
{
    char pidfile[512] = {0};
    snprintf(pidfile, (sizeof(pidfile) - 1), "%s%s", g_cfg_pool.work_path, "/pidfile");
    FILE *fp = fopen(pidfile, "w");

    if(!fp){
        log_fatal("failed to open pid file: %s", pidfile);
        exit(1);
    }

    char buf[128];
    pid_t pid = getpid();
    snprintf(buf, sizeof(buf), "%d", pid);
    log_info("pidfile: %s, pid: %d", pidfile, pid);
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
