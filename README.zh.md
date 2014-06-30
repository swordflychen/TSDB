tsdb readme
=========================

tsdb: time series database.
-------------------------

### 0. 描述：
* 基于Linux-C编写
* tsdb 是一个基于时间序列的KV数据库
* 底层存储采用 leveldb，支持 snappy 压缩
* 上层传输采用 redis 协议，所以支持 redis 的所有客户端程序，只是有的命令不支持

### 1. 功能说明：
* `get key`: 返回 key 所关联的字符串值
* `set key value`： 将字符串值 value 关联到 key ；
* `del key`： 删除key.
* `mset key1 val1 [key2 val2...]`：将字符串值 val1 关联到 key1，val2关联到key2...；
* `lrange prefix_key ts1 ts2`：返回key1=prefix_key+ts1 到 key2=prefix_key+ts2 范围内所有的KV对；
* `keys str`：返回数据库中所有以str开始的key（即前缀匹配）； 
* `info`：返回关于 TSDB 服务器的各种信息和统计数值；
* `ping`: 判断客户端是否连接到数据库；
* `exists key`: 判断key是否存在数据库中；

### 2. 安装说明：
* 编译 tsdb 依赖的 library：
    `make libs`
* 编译tsdb：
    `make all`
* 安装完毕.

### 3. 简单启动说明：
* 在 config.json 中指定工作目录、日志目录以及日志文件名，默认为：
```
    "work_path": "./var"
    "log_path": "./var/logs"
    "log_file": "access"
```
* 要保证上面的目录都存在：
```
    mkdir ./var
    mkdir ./var/logs
```
* 启动 tsdb：
```
   ./tsdb_start.sh 
```
* 经过上面的步骤，tsdb 已经启动了，如果需要详细配置，请看4。

### 4. 配置文件说明：
* 采用json格式保存，具体参数说明：
```
    w_port：integer，写数据的端口号；
    r_port：integer，读数据的端口号；

    max_connect：integer，最大链接数；
    num_threads：integer，后台处理线程数；
    
    work_path：string，工作目录，主要用于存放数据库文件；
    log_path：string，日志保存目录；
    log_file：string，日志文件名称；
    log_level：integer，日志级别（0-5），具体见下面的日志级别；
    
    ldb_write_buffer_size：integer，db写的缓存大小；
    ldb_block_size：integer，db块的大小；
    ldb_cache_lru_size：integer，db lru的cache大小；
    ldb_bloom_key_size：integer，db bloomkey的数目；
```
* 日志级别（log_level）说明：
```
    0: LEVEL_FATAL
    1: LEVEL_ERROR
    2: LEVEL_WARN
    3: LEVEL_INFO
    4: LEVEL_DEBUG
    5: LEVEL_TRACE
```
     
### 5. 代码结构说明：
* 各目录描述，详细信息请参见具体代码：
```
    .
    |-- docs                    --> 相关文档
    |-- deps                    --> tsdb依赖的library
    |-- obj                     --> tsdb代码生成的二进制文件保存目录
    |-- src                     --> tsdb代码所在的目录
    `-- var                     --> tsdb 默认的工作目录

```

### 6. 客户端：
* tsdb没有自己的客户端程序，支持Redis客户端程序，下面是redis.lua的一个例子：
```
    require 'redis'
    local conn = Redis.connect('127.0.0.1', 7501)
    conn:set('usr:rk', 10)
    conn:set('usr:nobody', 5)
    local value = conn:get('usr:rk')      -- 10
```

### 7. leveldb修改说明：
* 只修改下面两个文件的一些参数，具体修改内容可以在下面文件中搜索`chenjianfei@daoke.me`：
```
    deps/leveldb-1.15.0/build_detect_platform
    deps/leveldb-1.15.0/db/dbformat.h
    deps/leveldb-1.15.0/db/version_set.cc
```

### 8. Links:
* 以下是依赖的library的下载地址：
* [json-c](https://github.com/json-c/json-c)
* [levelDB](http://code.google.com/p/leveldb/)
* [snappy](http://code.google.com/p/snappy/downloads/detail?name=snappy-1.1.1.tar.gz)
* [libev](http://software.schmorp.de/pkg/libev.html)

### 9. License：
* 请见LICENSE文件。

### 10. TODO 
* TODO

