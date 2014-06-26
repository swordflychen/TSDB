TSDB readme
=========================

TSDB: Time Series Database.
-------------------------

### 0. Features
* TSDB is a key-value, time series database.
* Storage engine uses levelDB.
* Supports snappy compression.
* Redis clients are supported, some commands just are supported, see details below.
* Written in Linux C.

### 1. Commands support
* `get key`: Get the value of a key.
* `set key value`: Set the value of the key.
* `del key`: Delete the key.
* `mset key1 value1 [key2 value2 ...]`: Set multiple keys to multiple values.
* `lrange prefix_key ts1 ts2`: Get a range of key-values.
* `keys string`: Find all keys prefix matching the given pattern.
* `info`: Get information and statistics about the server.

### 2. Make TSDB
* Make the dependented libraries:
    `make libs`
* Make the TSDB: 
    `make all`
* Then make OK.

### 3. Startup TSDB server
* Modify the configuration file in `config.json`, the defaults as following:
```
    "work_path": "./var"
    "log_path": "./var/logs"
    "log_file": "access"
```
* Must ensure the directories exist, if not exist, then:
```
    mkdir ./var
    mkdir ./var/logs
```
* Startup the TSDB server:
```
   ./tsdb_start.sh 
```
* Start OK.

### 4. Config file -- config.json
* The file is json format. Below is a description of each item.
```
    w_port: integer, write port.
    r_port: integer, read port.

    max_connect: integer, maximum number of connections.
    num_threads: integer, the number of processing threads.
    
    work_path: string, workspace directory, data files storage directory.
    log_path: string, logging directory.
    log_file: string, log file name.
    log_level: integer, log level, the details see below.
    
    ldb_write_buffer_size: integer, write buffer size in bytes. 
    ldb_block_size: integer, block size in bytes.
    ldb_cache_lru_size: integer, lru cache size in bytes.
    ldb_bloom_key_size: integer, number of bloomkeys. 
```
* Log level(log_level)
```
    0: LOGLV_ERROR
    1: LOGLV_WARNING
    2: LOGLV_INFO
    3: LOGLV_NOTICE
    4: LOGLV_DEBUG
    5: LOGLV_ALL
```
     
### 5. Source code
* Description the directories.
```
    .
    |-- doc                     --> documents
    |-- lib                     --> dependented libraries
    |-- obj                     --> object binary files
    |-- src                     --> source code
    `-- var                     --> default workspace

```

### 6. Client API example 
* Lua
```
    require 'redis'
    local conn = Redis.connect('127.0.0.1', 7501)
    conn:set('usr:rk', 10)
    conn:set('usr:nobody', 5)
    local value = conn:get('usr:rk')      -- 10
	print(value)
```

### 7. LevelDB 
* Modified the levelDB source code( you can research the string chenjianfei@daoke.me in levelDB directory).
```
    lib/leveldb-1.15.0/build_detect_platform
    lib/leveldb-1.15.0/db/dbformat.h
    lib/leveldb-1.15.0/db/version_set.cc
```

### 8. Links
* The dependented libraries:
* [json-c](https://github.com/json-c/json-c)
* [levelDB](http://code.google.com/p/leveldb/)
* [snappy]( http://code.google.com/p/snappy/downloads/detail?name=snappy-1.1.1.tar.gz)
* [libev](http://software.schmorp.de/pkg/libev.html)

### 9. License
* See the LICENSE file. 

### 10. TODO 
* TODO
