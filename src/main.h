#pragma once

#include <ev.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/time.h>

#include "utils.h"
//#include "read_cfg.h"

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

#define dbg(f)	fprintf(f, CYAN "FILE--> %20s | " YELLOW "FUN--> %20s | " PURPLE "LINE--> %4d | " BLUE "MSG--> ", __FILE__, __FUNCTION__, __LINE__)
#define ero(f)	fprintf(f, CYAN "FILE--> %20s | " YELLOW "FUN--> %20s | " PURPLE "LINE--> %4d | " RED "MSG--> ", __FILE__, __FUNCTION__, __LINE__)

#ifdef OPEN_DEBUG
//#define x_perror(msg)		{ ero(stderr); perror(msg); }
#define x_perror(msg, ...)	{ ero(stderr); fprintf(stderr, RED msg NONE, ##__VA_ARGS__ ); perror("\n"); }

#define x_printf(msg, ...)	{ dbg(stdout); fprintf(stdout, BLUE msg NONE, ##__VA_ARGS__ ); }
#define x_out_time(x)		{ gettimeofday( ((struct timeval *)x), NULL ); \
                                  printf("time: %ld.%ld s\n", ((struct timeval *)x)->tv_sec, ((struct timeval *)x)->tv_usec); }
#else
#define x_perror(msg, ...)	{ dyn_log( LOGLV_ERROR, msg, ##__VA_ARGS__ ); }
#define x_printf(msg, ...)	{ dyn_log( LOGLV_DEBUG, msg, ##__VA_ARGS__ ); };
#define x_out_time(x)		;
#endif

/*************************************************/
#define MAX_DEF_LEN	1024*1024
#define BACKLOG		1024

#define FETCH_MAX_CNT_MSG	"-you have time travel!\r\n"
/*************************************************/
/* An item in the connection queue. */
typedef struct conn_queue_item CQ_ITEM;
struct conn_queue_item {
    int sfd;
    int port;
    struct conn_queue_item *next;
    char szAddr[ INET_ADDRSTRLEN ];/* 255.255.255.255 */
};


/* A connection queue. */
typedef struct conn_queue CQ_LIST;
struct conn_queue {
    CQ_ITEM *head;
    CQ_ITEM *tail;
    pthread_mutex_t lock;
};

/*
 * libev default loop, with a accept_watcher to accept the new connect
 * and dispatch to WORK_THREAD.
 */
typedef struct {
    pthread_t thread_id;         /* unique ID of this thread */
    struct ev_loop *loop;     /* libev loop this thread uses */
    struct ev_io accept_watcher;   /* accept watcher for new connect */
} DISPATCHER_THREAD;

/*
 * Each libev instance has a async_watcher, which other threads
 * can use to signal that they've put a new connection on its queue.
 */
typedef struct {
    pthread_t thread_id;         /* unique ID of this thread */
    struct ev_loop *loop;     /* libev loop this thread uses */
    struct ev_async async_watcher;   /* async watcher for new connect */
    struct ev_prepare prepare_watcher;
    struct ev_check check_watcher;
    struct conn_queue new_conn_queue; /* queue of new connections to handle */
} WORK_THREAD;

enum {
    NO_WORKING = 0,
    IS_WORKING = 1,
};


struct data_node {
    /***cqi***/
    struct conn_queue_item item;
    /***ev***/
    struct ev_loop *loop;     /* libev loop this thread uses */

    ev_io io_watcher;
    int io_work_status;
    pthread_mutex_t io_lock;

    ev_timer timer_watcher;
    /***R***/
    int alsize;
    int gtlen;
    char *svbf;
    char recv[MAX_DEF_LEN];

    /***W***/
    int mxsize;
    int mxlen;
    int ptlen;
    char *sdbf;
    char send[MAX_DEF_LEN];
    /***P***/
    int kvs;
    int cmd;
    size_t klen;
    size_t vlen;

    int doptr;
    int key;
    int val;

    int val2;
    size_t vlen2;

    /***C***/
    int status;
};

/************public variable****************/
extern int G_PAGE_SIZE;
/**************public api*******************/
extern void clean_send_node(struct data_node *p_node);

extern int add_send_node(struct data_node *p_node, const char *output, int add);

extern int set_send_node(struct data_node *p_node, const char *output, int add);
