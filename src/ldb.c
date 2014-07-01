#include <string.h>
#include <unistd.h>
#include <ftw.h>
#include <sys/utsname.h>
#include <time.h>

#include "ldb.h"
#include "ctl.h"
#include "version.h"
#include "read_cfg.h"
#include "logger.h"

extern size_t LDB_WRITE_BUFFER_SIZE;
extern size_t LDB_BLOCK_SIZE;
extern size_t LDB_CACHE_LRU_SIZE;
extern short LDB_BLOOM_KEY_SIZE;
extern char *LDB_WORK_PATH;
extern short LDB_READONLY_SWITCH;
extern int W_PORT;
extern int R_PORT;
extern time_t LDB_START_TIME;
extern struct cfg_info g_cfg_pool;

struct _leveldb_stuff *ldb_initialize(char *path)
{

    struct _leveldb_stuff *ldbs = NULL;
    leveldb_cache_t *cache;
    leveldb_filterpolicy_t *policy;
    char* err = NULL;

    ldbs = (struct _leveldb_stuff *)malloc(sizeof(struct _leveldb_stuff));
    memset(ldbs, 0, sizeof(struct _leveldb_stuff));

    ldbs->options = leveldb_options_create();
    snprintf(ldbs->dbname, sizeof(ldbs->dbname), "%s%s%s", LDB_WORK_PATH, "/", path);

    cache = leveldb_cache_create_lru(LDB_CACHE_LRU_SIZE);
    policy = leveldb_filterpolicy_create_bloom(LDB_BLOOM_KEY_SIZE);

    leveldb_options_set_filter_policy(ldbs->options, policy);
    leveldb_options_set_cache(ldbs->options, cache);
    leveldb_options_set_block_size(ldbs->options, LDB_BLOCK_SIZE);
    leveldb_options_set_write_buffer_size(ldbs->options, LDB_WRITE_BUFFER_SIZE);
#if defined(OPEN_COMPRESSION)
    leveldb_options_set_compression(ldbs->options, leveldb_snappy_compression);
#else
    leveldb_options_set_compression(ldbs->options, leveldb_no_compression);
#endif
    /* Read options */
    ldbs->roptions = leveldb_readoptions_create();
    leveldb_readoptions_set_verify_checksums(ldbs->roptions, 1);
    leveldb_readoptions_set_fill_cache(ldbs->roptions, 1);/* set 1 is faster */

    /* W  options */
    ldbs->woptions = leveldb_writeoptions_create();
#ifdef SYNC_PUT
    leveldb_writeoptions_set_sync(ldbs->woptions, 1);
#else
    leveldb_writeoptions_set_sync(ldbs->woptions, 0);
#endif

    /* Batch write */
    ldbs->wbatch = leveldb_writebatch_create();

    leveldb_options_set_create_if_missing(ldbs->options, 1);
    ldbs->db = leveldb_open(ldbs->options, ldbs->dbname, &err);

    if (err) {
        fprintf(stderr, "%s", err);
        leveldb_free(err);
        err = NULL;
        free(ldbs);
        return NULL ;
    } else {
        return ldbs;
    }

}

void ldb_close(struct _leveldb_stuff *ldbs)
{
    leveldb_close(ldbs->db);

    leveldb_options_destroy(ldbs->options);
    leveldb_readoptions_destroy(ldbs->roptions);
    leveldb_writeoptions_destroy(ldbs->woptions);
    leveldb_writebatch_destroy(ldbs->wbatch);
}

void ldb_destroy(struct _leveldb_stuff *ldbs)
{
    char* err = NULL;
    leveldb_close(ldbs->db);
    leveldb_destroy_db(ldbs->options, ldbs->dbname, &err);
    if (err) {
        fprintf(stderr, "%s", err);
        leveldb_free(err);
        err = NULL;
    }
    leveldb_options_destroy(ldbs->options);
    leveldb_readoptions_destroy(ldbs->roptions);
    leveldb_writeoptions_destroy(ldbs->woptions);
    leveldb_writebatch_destroy(ldbs->wbatch);
    free(ldbs);
}

char *ldb_get(struct _leveldb_stuff *ldbs, const char *key, size_t klen, int *vlen)
{
    char *err = NULL;
    char *val = NULL;
    val = leveldb_get(ldbs->db, ldbs->roptions, key, klen, (size_t *) vlen, &err);
    if (err) {
        fprintf(stderr, "\n%s\n", err);
        leveldb_free(err);
        err = NULL;
        *vlen = -1;
        return NULL ;
    } else {
        return val;
    }
}

static int get_number_len(int len)
{
    int i = 1;
    int mlen = len;
    while ((mlen /= 10) >= 1) {
        i++;
    }
    return i;
}

static char *set_bulk(char *dst, const char *put, int len)
{
    char *ptr = NULL;
    sprintf(dst, "$%d\r\n", len);
    int hlen = strlen(dst);
    ptr = dst + hlen;
    memcpy(ptr, put, len);
    ptr += len;
    memcpy(ptr, "\r\n", 2);
    return (ptr + 2);
}

char *ldb_lrangeget(struct _leveldb_stuff *ldbs, const char *pre_key, size_t pre_klen, const char *st_time, size_t st_tlen, const char *ed_time, size_t ed_tlen, int *size)
{
    char *err = NULL;
    char *result = NULL;
    char *p_dst = NULL;
    char *p_val = NULL;
    char *p_key = NULL;
    size_t klen = 0;
    size_t vlen = 0;
    int index = 0;
    int i, j, z = 0;
    struct kv_list list = { 0 };
    struct some_kv *p_new, *p_old, *p_tmp = NULL;

    /* piece the start_key && end_key together */
    size_t st_klen = pre_klen + st_tlen;
    size_t ed_klen = pre_klen + ed_tlen;
    char *st_key = (char *)malloc(st_klen);
    char *ed_key = (char *)malloc(ed_klen);
    memcpy(st_key, pre_key, pre_klen);
    memcpy(st_key + pre_klen, st_time, st_tlen);
    memcpy(ed_key, pre_key, pre_klen);
    memcpy(ed_key + pre_klen, ed_time, ed_tlen);

    leveldb_iterator_t* iter = leveldb_create_iterator(ldbs->db, ldbs->roptions);
    leveldb_iterator_t* iter_save = iter;
    if (!!leveldb_iter_valid(iter)) {/* first use it is invalid */
        fprintf(stderr, "%s:%d: this iter is valid already!\n", __FILE__, __LINE__);
        *size = -1;
        return NULL ;
    }
    leveldb_iter_seek(iter, st_key, st_klen);
    p_key = (char *) st_key;
    p_old = p_new = &list.head;
    if (0 <= strncmp(ed_key, p_key, ed_klen)) {
        while (leveldb_iter_valid(iter)) {
            /* parse kv */
            p_key = (char *) leveldb_iter_key(iter, &klen);
            log_debug("%p iter key = %s, klen = %ld\n", p_key, p_key, klen);

            p_val = (char *) leveldb_iter_value(iter, &vlen);

            leveldb_iter_get_error(iter, &err);
            if (err) {
                goto FAIL_ITER_PARSE;
            }

            if (0 > strncmp(ed_key, p_key, ed_klen)) {
            	log_debug("--------------break------------------\n");
                break;
            }
            /* save parse */
            list.count++;/* kv counts */
            list.klens += klen;
            list.knubs += get_number_len(klen);
            list.vlens += vlen;
            list.vnubs += get_number_len(vlen);
            index = list.count % SOME_KV_NODES_COUNT;
            if ((list.count / SOME_KV_NODES_COUNT >= 1) && (index == 1)) {
                /* new store */
                p_new = (struct some_kv *)malloc(sizeof(struct some_kv));
                if (p_new == NULL ) {
                    /* free stroe */
                    index = GET_NEED_COUNT( list.count, SOME_KV_NODES_COUNT );
                    p_tmp = &list.head;
                    for (i = 0, z = list.count - 1; (i < index) && (p_tmp != NULL ); i++ ) {
                        for (j = 0; (j < SOME_KV_NODES_COUNT) && (z > 0); j++, z--) {
                            free(p_tmp->nodes[j].key);
                            free(p_tmp->nodes[j].val);
                        }
                        p_old = p_tmp;
                        p_tmp = p_tmp->next;
                        if (p_old != &list.head) {
                            free(p_old);
                        }
                    }
                    goto FAIL_MEMORY;
                }
                memset(p_new, 0, sizeof(struct some_kv));
                p_old->next = p_new;
                p_new->prev = p_old;
                p_old = p_new;
            }

            /*
             * fix bug: index is error if list.count = n * SOME_KV_NODES_COUNT(1024),
             *          SOME_KV_NODES_COUNT = 1024, n > 0.
             */
            if (index == 0) {
                index = SOME_KV_NODES_COUNT;
            }

            /* save key */
            p_new->nodes[index - 1].klen = klen;
            p_new->nodes[index - 1].key = (char *)malloc(GET_NEED_COUNT( klen, G_PAGE_SIZE ) * G_PAGE_SIZE);
            memcpy(p_new->nodes[index - 1].key, p_key, klen);

            /* save val */
            p_new->nodes[index - 1].vlen = vlen;
            p_new->nodes[index - 1].val = (char *)malloc(GET_NEED_COUNT( vlen, G_PAGE_SIZE ) * G_PAGE_SIZE);
            memcpy(p_new->nodes[index - 1].val, p_val, vlen);

            /* find next */
            leveldb_iter_next(iter);
        }
    }

    /* free space */
    free(st_key);
    free(ed_key);

    /* create result */
    if (list.count > 0) {
        /* has members */
        /* *2\r\n$5\r\nmykey\r\n$5\r\nmyval\r\n */
        *size = strlen("*\r\n") + get_number_len(list.count * 2)
                + strlen("$\r\n\r\n") * (list.count * 2) + list.knubs + list.klens + list.vnubs
                + list.vlens;
        index = GET_NEED_COUNT( *size, G_PAGE_SIZE ) * G_PAGE_SIZE;
        result = (char *) malloc(index);
        if (result == NULL ) goto FAIL_MEMORY;
        memset(result, 0, index);
        log_debug("----->>>ALL SIZE IS %d, BUFF %p : LEN IS %d\n", *size, result, index);

        /* split piece */
        index = GET_NEED_COUNT( list.count, SOME_KV_NODES_COUNT );
        p_tmp = &list.head;
        sprintf(result, "*%d\r\n", list.count * 2);
        p_dst = result + strlen(result);
        for (i = 0, z = list.count; (i < index) && (p_tmp != NULL ); i++ ) {
            for (j = 0; (j < SOME_KV_NODES_COUNT) && (z > 0); j++, z--) {
                p_dst = set_bulk(p_dst, p_tmp->nodes[j].key, p_tmp->nodes[j].klen);
                free(p_tmp->nodes[j].key);
                p_dst = set_bulk(p_dst, p_tmp->nodes[j].val, p_tmp->nodes[j].vlen);
                free(p_tmp->nodes[j].val);
            }
            p_old = p_tmp;
            p_tmp = p_tmp->next;
            if (p_old != &list.head) {
                free(p_old);
            }
        }
    } else {
        /* no members */
        *size = 0;
    }
    leveldb_iter_destroy(iter_save);
    return result;
    FAIL_ITER_PARSE: fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, (err));
    leveldb_free(err);
    err = NULL;
    leveldb_iter_destroy(iter);
    *size = -1;
    return NULL ;
    FAIL_MEMORY: fprintf(stderr, "%s:%d: FAILED MALLOC !\n", __FILE__, __LINE__);
    leveldb_iter_destroy(iter);
    *size = -1;
    return NULL ;
}

char *ldb_keys(struct _leveldb_stuff *ldbs, const char *ptn, size_t ptn_len, int *size)
{
    char *err = NULL;
    char *result = NULL;
    char *p_dst = NULL;
    char *p_key = NULL;
    size_t klen = 0;
    int index = 0;
    int i, j, z = 0;
    struct kv_list list = { 0 };
    struct some_kv *p_new, *p_old, *p_tmp = NULL;

    leveldb_iterator_t* iter = leveldb_create_iterator(ldbs->db, ldbs->roptions);
    leveldb_iterator_t* iter_save = iter;
    if (!!leveldb_iter_valid(iter)) {/* first use it is invalid */
        fprintf(stderr, "%s:%d: this iter is valid already!\n", __FILE__, __LINE__);
        *size = -1;
        return NULL ;
    }

    leveldb_iter_seek(iter, ptn, ptn_len);
    p_old = p_new = &list.head;
    while (leveldb_iter_valid(iter)) {
        /* parse kv */
        p_key = (char *) leveldb_iter_key(iter, &klen);
        log_debug("%p iter key = %s, klen = %ld\n", p_key, p_key, klen);

        leveldb_iter_get_error(iter, &err);
        if (err) {
            goto FAIL_ITER_PARSE;
        }

        if (!prefix_match_len(ptn, ptn_len, p_key, klen)) {
            break;
        }

        /* save parse */
        list.count++;/* kv counts */
        list.klens += klen;
        list.knubs += get_number_len(klen);
        index = list.count % SOME_KV_NODES_COUNT;
        if ((list.count / SOME_KV_NODES_COUNT >= 1) && (index == 1)) {
            /* new store */
            p_new = (struct some_kv *)malloc(sizeof(struct some_kv));
            if (p_new == NULL ) {
                /* free store */
                index = GET_NEED_COUNT( list.count, SOME_KV_NODES_COUNT );
                p_tmp = &list.head;
                for (i = 0, z = list.count - 1; (i < index) && (p_tmp != NULL ); i++ ) {
                    for (j = 0; (j < SOME_KV_NODES_COUNT) && (z > 0); j++, z--) {
                        free(p_tmp->nodes[j].key);
                    }
                    p_old = p_tmp;
                    p_tmp = p_tmp->next;
                    if (p_old != &list.head) {
                        free(p_old);
                    }
                }
                goto FAIL_MEMORY;
            }
            memset(p_new, 0, sizeof(struct some_kv));
            p_old->next = p_new;
            p_new->prev = p_old;
            p_old = p_new;
        }

        /*
         * fix bug: index is error if list.count = n * SOME_KV_NODES_COUNT(1024),
         *          SOME_KV_NODES_COUNT = 1024, n > 0.
         */
        if (index == 0) {
            index = SOME_KV_NODES_COUNT;
        }

        /* save key */
        p_new->nodes[index - 1].klen = klen;
        p_new->nodes[index - 1].key = (char *)malloc(GET_NEED_COUNT( klen, G_PAGE_SIZE ) * G_PAGE_SIZE);
        memcpy(p_new->nodes[index - 1].key, p_key, klen);

        /* find next */
        leveldb_iter_next(iter);

    }

    /* create result */
    if (list.count > 0) {
        /* has members */
        /* *2\r\n$5\r\nmykey\r\n$5\r\nmyval\r\n */
        *size = strlen("*\r\n") + get_number_len(list.count) + strlen("$\r\n\r\n") * (list.count)
                + list.knubs + list.klens;
        index = GET_NEED_COUNT( *size, G_PAGE_SIZE ) * G_PAGE_SIZE;
        result = (char *) malloc(index);
        if (result == NULL ) goto FAIL_MEMORY;
        memset(result, 0, index);
        log_debug("----->>>ALL SIZE IS %d, BUFF %p : LEN IS %d\n", *size, result, index);

        /* split piece */
        index = GET_NEED_COUNT( list.count, SOME_KV_NODES_COUNT );
        p_tmp = &list.head;
        sprintf(result, "*%d\r\n", list.count);
        p_dst = result + strlen(result);
        for (i = 0, z = list.count; (i < index) && (p_tmp != NULL ); i++ ) {
            for (j = 0; (j < SOME_KV_NODES_COUNT) && (z > 0); j++, z--) {
                p_dst = set_bulk(p_dst, p_tmp->nodes[j].key, p_tmp->nodes[j].klen);
                free(p_tmp->nodes[j].key);
            }
            p_old = p_tmp;
            p_tmp = p_tmp->next;
            if (p_old != &list.head) {
                free(p_old);
            }
        }
    } else {
        /* no members */
        *size = 0;
    }

    leveldb_iter_destroy(iter_save);
    return result;
    FAIL_ITER_PARSE: fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, (err));
    leveldb_free(err);
    err = NULL;
    leveldb_iter_destroy(iter);
    *size = -1;
    return NULL ;
    FAIL_MEMORY: fprintf(stderr, "%s:%d: FAILED MALLOC !\n", __FILE__, __LINE__);
    leveldb_iter_destroy(iter);
    *size = -1;
    return NULL ;
}

char *ldb_info(struct _leveldb_stuff *ldbs, int *size)
{
    int cnt = 0;
    *size = 0;

    /* #server */
    char svr[16] = "# Server:";
    ++cnt;
    *size += strlen(svr) + get_number_len(strlen(svr));

    /* tsdb version */
    char vers[32] = TSDB_VERSION;
    ++cnt;
    *size += strlen(vers) + get_number_len(strlen(vers));

    /* os */
    char os[64] = { 0 };
    struct utsname name;
    uname(&name);
    sprintf(os, "os: %s %s %s", name.sysname, name.release, name.machine);
    ++cnt;
    *size += strlen(os) + get_number_len(strlen(os));

    /* pid */
    char pid[32] = { 0 };
    sprintf(pid, "process_id: %ld", (long) getpid());
    ++cnt;
    *size += strlen(pid) + get_number_len(strlen(pid));

    /* mode. */
    char mode[32] = { 0 };
    sprintf(mode, "mode: %s", LDB_READONLY_SWITCH ? "readonly" : "read/write");
    ++cnt;
    *size += strlen(mode) + get_number_len(strlen(mode));

    /* write_port */
    char wport[32] = { 0 };
    sprintf(wport, "write_port: %d", W_PORT);
    ++cnt;
    *size += strlen(wport) + get_number_len(strlen(wport));

    /* read_port */
    char rport[32] = { 0 };
    sprintf(rport, "read_port: %d", R_PORT);
    ++cnt;
    *size += strlen(rport) + get_number_len(strlen(rport));

    /* uptime_in_seconds */
    char uptis[64] = { 0 };
    time_t curtime;
    time(&curtime);
    sprintf(uptis, "uptime_in_seconds: %ld", (curtime - LDB_START_TIME));
    ++cnt;
    *size += strlen(uptis) + get_number_len(strlen(uptis));

    /* uptime_in_days */
    char uptid[64] = { 0 };
    sprintf(uptid, "uptime_in_days: %ld", (curtime - LDB_START_TIME) / (24 * 3600));
    ++cnt;
    *size += strlen(uptid) + get_number_len(strlen(uptid));

    /* # Keyspace. */
    char keyspace[32] = "\n# Keyspace:\nTODO";
    ++cnt;
    *size += strlen(keyspace) + get_number_len(strlen(keyspace));

    /* #LeveldbStatus: */
    char ldbstat[32] = "\n# Leveldbstatus:";
    ++cnt;
    *size += strlen(ldbstat) + get_number_len(strlen(ldbstat));

    /* get property. */
    char *property = leveldb_property_value(ldbs->db, "leveldb.stats");
    ++cnt;
    *size += strlen(property) + get_number_len(strlen(property));

    /* create result */
    *size += strlen("*\r\n") + get_number_len(cnt) + strlen("$\r\n\r\n") * cnt;
    int index = GET_NEED_COUNT( *size, G_PAGE_SIZE ) * G_PAGE_SIZE;
    char *result = (char *) malloc(index);

    char *ptr = NULL;
    sprintf(result, "*%d\r\n", cnt);
    /* #Server */
    ptr = result + strlen(result);
    ptr = set_bulk(ptr, svr, strlen(svr));
    ptr = set_bulk(ptr, vers, strlen(vers));
    ptr = set_bulk(ptr, os, strlen(os));
    ptr = set_bulk(ptr, mode, strlen(mode));
    ptr = set_bulk(ptr, pid, strlen(pid));
    ptr = set_bulk(ptr, wport, strlen(wport));
    ptr = set_bulk(ptr, rport, strlen(rport));
    ptr = set_bulk(ptr, uptis, strlen(uptis));
    ptr = set_bulk(ptr, uptid, strlen(uptid));
    /* #Keyspace */
    ptr = set_bulk(ptr, keyspace, strlen(keyspace));
    /* #Leveldbstatus */
    ptr = set_bulk(ptr, ldbstat, strlen(ldbstat));
    ptr = set_bulk(ptr, property, strlen(property));

    leveldb_free(property);

    log_debug("%s\n", result);

    return result;
}

int ldb_put(struct _leveldb_stuff *ldbs, const char *key, size_t klen, const char *value, size_t vlen)
{
    char *err = NULL;
    leveldb_put(ldbs->db, ldbs->woptions, key, klen, value, vlen, &err);
    if (err) {
        fprintf(stderr, "%s", err);
        leveldb_free(err);
        err = NULL;
        return -1;
    } else {
        return 0;
    }
}

int ldb_delete(struct _leveldb_stuff *ldbs, const char *key, size_t klen)
{
    char* err = NULL;
	char *retval = NULL;
	size_t retvlen = 0;
	retval = leveldb_get(ldbs->db, ldbs->roptions, key, klen, &retvlen, &err);
	if (err) {
		fprintf(stderr, "\n%s\n", err);
		leveldb_free(err);
		err = NULL;
		return -1;
	}
	leveldb_free(retval);

	/* if not found, then return 0. */
	if (retvlen == 0) {
		return 0;
	}

	/* if found, delete it, then return 1. */
    leveldb_delete(ldbs->db, ldbs->woptions, key, klen, &err);
    if (err) {
        fprintf(stderr, "%s", err);
        leveldb_free(err);
        err = NULL;
        return -1;
    }

    return 1;
}

int ldb_batch_put(struct _leveldb_stuff *ldbs, const char *key, size_t klen, const char *value, size_t vlen)
{
    leveldb_writebatch_put(ldbs->wbatch, key, klen, value, vlen);
    return 0;
}

int ldb_batch_commit(struct _leveldb_stuff *ldbs)
{
    char* err = NULL;
    leveldb_write(ldbs->db, ldbs->woptions, ldbs->wbatch, &err);
    leveldb_writebatch_clear(ldbs->wbatch);
    if (err) {
        fprintf(stderr, "%s", err);
        leveldb_free(err);
        err = NULL;
        return -1;
    } else {
        return 0;
    }
}

int ldb_exists(struct _leveldb_stuff *ldbs, const char *key, size_t klen)
{
	char *err = NULL;
	char *val = NULL;
	size_t vlen = 0;
	val = leveldb_get(ldbs->db, ldbs->roptions, key, klen, &vlen, &err);
	if (err) {
		fprintf(stderr, "\n%s\n", err);
		leveldb_free(err);
		return -1;
	}
	leveldb_free(val);
	return vlen;
}
