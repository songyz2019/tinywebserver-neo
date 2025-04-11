#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include "log.h"
#include <pthread.h>
using namespace std;

Logger::~Logger()
{
    if (mFile != NULL)
    {
        fclose(mFile);
    }
}
//异步需要设置阻塞队列的长度，同步不需要设置
bool Logger::init(const char *file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size)
{
    //如果设置了max_queue_size,则设置为异步
    if (max_queue_size >= 1)
    {
        mIsAsync = true;
        mLogQueue = new block_queue<string>(max_queue_size);
        pthread_t tid;
        //flush_log_thread为回调函数,这里表示创建线程异步写日志
        pthread_create(&tid, NULL, [](void *args){Logger::getInstance()->writeAsync(); return (void*)NULL;}, NULL);
    }
    
    mDisableLogging = close_log;
    m_log_buf_size = log_buf_size;
    m_buf = new char[m_log_buf_size];
    memset(m_buf, '\0', m_log_buf_size);
    m_split_lines = split_lines;

    time_t t = time(NULL);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

 
    const char *p = strrchr(file_name, '/');
    char log_full_name[256] = {0};

    if (p == NULL)
    {
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    }
    else
    {
        strcpy(log_name, p + 1);
        strncpy(dir_name, file_name, p - file_name + 1);
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
    }

    m_today = my_tm.tm_mday;
    
    mFile = fopen(log_full_name, "a");
    if (mFile == NULL)
    {
        return false;
    }

    return true;
}

// TODO: Replace T with a template type
void Logger::write(int level, const char *format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[16] = {0};
    switch (level)
    {
    case 0:
        strcpy(s, "[debug]:");
        break;
    case 1:
        strcpy(s, "[info]:");
        break;
    case 2:
        strcpy(s, "[warn]:");
        break;
    case 3:
        strcpy(s, "[erro]:");
        break;
    default:
        strcpy(s, "[info]:");
        break;
    }
    //写入一个log，对m_count++, m_split_lines最大行数
    m_mutex.lock();
    mCount++;

    if (m_today != my_tm.tm_mday || mCount % m_split_lines == 0) //everyday log
    {
        
        char new_log[256] = {0};
        fflush(mFile);
        fclose(mFile);
        char tail[16] = {0};
       
        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);
       
        if (m_today != my_tm.tm_mday)
        {
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            mCount = 0;
        }
        else
        {
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, mCount / m_split_lines);
        }
        mFile = fopen(new_log, "a");
    }
 
    m_mutex.unlock();

    va_list valst;
    va_start(valst, format);

    string log_str;
    m_mutex.lock();

    //写入的具体时间内容格式
    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    
    int m = vsnprintf(m_buf + n, m_log_buf_size - 1, format, valst);
    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';
    log_str = m_buf;

    m_mutex.unlock();

    if (mIsAsync && !mLogQueue->full())
    {
        mLogQueue->push(log_str);
    }
    else
    {
        m_mutex.lock();
        fputs(log_str.c_str(), mFile);
        m_mutex.unlock();
    }

    va_end(valst);
}

void Logger::flush(void)
{
    m_mutex.lock();
    //强制刷新写入流缓冲区
    fflush(mFile);
    m_mutex.unlock();
}

void* Logger::writeAsync()
{
    string single_log;
    //从阻塞队列中取出一个日志string，写入文件
    while (mLogQueue->pop(single_log))
    {
        m_mutex.lock();
            fputs(single_log.c_str(), mFile);
        m_mutex.unlock();
    }
}