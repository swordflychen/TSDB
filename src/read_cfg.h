#pragma once



struct cfg_info {
    char *ip;				// IP address.
    short w_port;			// write port.
    short r_port;			// read port.
    size_t max_connect;		// max connect count.
    short num_threads;		// max thread count.

    char *work_path;		// work space directory.
    char *log_path;			// log directory.
    char *log_file;			// log file name format.
    short log_verbosity;	// log level.

    short ldb_readonly_switch;	// read only switch <=> mode
    size_t ldb_write_buffer_size;
    size_t ldb_block_size;
    size_t ldb_cache_lru_size;
    short ldb_bloom_key_size;
};

extern void read_cfg(struct cfg_info *p_cfg, char *cfg_file);
