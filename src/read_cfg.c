#include <stdlib.h>
#include <string.h>
#include <json.h>

#include "read_cfg.h"
#include "main.h"
#include "utils.h"
#include "logger.h"

void read_cfg(struct cfg_info *p_cfg, char *cfg_file)
{
    const char *str_val = NULL;
    struct json_object *obj = NULL;
    struct json_object *cfg = json_object_from_file(cfg_file);
    if (cfg == NULL) {
        goto fail;
    }

    /* write and read port. */
    if (json_object_object_get_ex(cfg, "w_port", &obj)) {
        p_cfg->w_port = json_object_get_int(obj);
    } else goto fail;
    if (json_object_object_get_ex(cfg, "r_port", &obj)) {
        p_cfg->r_port = json_object_get_int(obj);
    } else goto fail;

    /* max connect count. */
    if (json_object_object_get_ex(cfg, "max_connect", &obj)) {
        p_cfg->max_connect = json_object_get_int64(obj);
    } else goto fail;
    if (json_object_object_get_ex(cfg, "num_threads", &obj)) {
        p_cfg->num_threads = json_object_get_int(obj);
    } else goto fail;

    /* work space directory. */
    if (json_object_object_get_ex(cfg, "work_path", &obj)) {
        str_val = json_object_get_string(obj);
        p_cfg->work_path = x_strdup(str_val);
    } else goto fail;

    /* log configure. */
    if (json_object_object_get_ex(cfg, "log_path", &obj)) {
        str_val = json_object_get_string(obj);
        p_cfg->log_path = x_strdup(str_val);
    } else goto fail;
    if (json_object_object_get_ex(cfg, "log_file", &obj)) {
        str_val = json_object_get_string(obj);
        p_cfg->log_file = x_strdup(str_val);
    } else goto fail;
    if (json_object_object_get_ex(cfg, "log_level", &obj)) {
        p_cfg->log_verbosity = json_object_get_int(obj);
    } else goto fail;

    /* mode configure. */
    if (json_object_object_get_ex(cfg, "ldb_readonly_switch", &obj)) {
        p_cfg->ldb_readonly_switch = json_object_get_int64(obj);
    } else goto fail;

    /* ldb configure. */
    if (json_object_object_get_ex(cfg, "ldb_write_buffer_size", &obj)) {
        p_cfg->ldb_write_buffer_size = json_object_get_int64(obj);
    } else goto fail;

    if (json_object_object_get_ex(cfg, "ldb_block_size", &obj)) {
        p_cfg->ldb_block_size = json_object_get_int64(obj);
    } else goto fail;

    if (json_object_object_get_ex(cfg, "ldb_cache_lru_size", &obj)) {
        p_cfg->ldb_cache_lru_size = json_object_get_int64(obj);
    } else goto fail;

    if (json_object_object_get_ex(cfg, "ldb_bloom_key_size", &obj)) {
        p_cfg->ldb_bloom_key_size = json_object_get_int(obj);
    } else goto fail;

    return;
fail:
    log_fatal("invalid config file :%s\n", cfg_file);
    exit(EXIT_FAILURE);
}
