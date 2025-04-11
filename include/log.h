#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <pthread.h>
#include "block_queue.h"

using namespace std;

class Logger
{
public:
    static Logger* getInstance()
    {
        static Logger instance;
        return &instance;
    }
    Logger(const Logger&)  = delete;
    Logger(const Logger&&) = delete;
    Logger& operator=(const Logger&)  = delete;
    Logger& operator=(const Logger&&) = delete;

    //可选择的参数有日志文件、日志缓冲区大小、最大行数以及最长日志条队列
    bool init(const char *file_name, int close_log, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);
    void write(int level, const char *format, ...);
    void flush();

private:
    Logger() {}
    virtual ~Logger();
    void* writeAsync();

private:
    char dir_name[128]; //路径名
    char log_name[128]; //log文件名
    int m_split_lines;  //日志最大行数
    int m_log_buf_size; //日志缓冲区大小
    long long mCount {0};  //日志行数记录
    int m_today;        //因为按天分类,记录当前时间是那一天
    FILE *mFile;         //打开log的文件指针
    char *m_buf;
    block_queue<string> *mLogQueue; //阻塞队列
    bool mIsAsync {false};                  //是否同步标志位
    Locker m_mutex;
    int mDisableLogging; //关闭日志
};

#define LOG_DEBUG(format, ...) {Logger::getInstance()->write_log(0, format, ##__VA_ARGS__); Logger::getInstance()->flush();}
#define LOG_INFO(format, ...) {Logger::getInstance()->write_log(1, format, ##__VA_ARGS__); Logger::getInstance()->flush();}
#define LOG_WARN(format, ...) {Logger::getInstance()->write_log(2, format, ##__VA_ARGS__); Logger::getInstance()->flush();}
#define LOG_ERROR(format, ...) {Logger::getInstance()->write_log(3, format, ##__VA_ARGS__); Logger::getInstance()->flush();}

#endif
