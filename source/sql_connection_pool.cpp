#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include "sql_connection_pool.h"
#include "log.h"

using namespace std;

SrvConnectionPool *SrvConnectionPool::GetInstance()
{
	static SrvConnectionPool connPool;
	return &connPool;
}

//构造初始化
void SrvConnectionPool::init(string url, string User, string PassWord, string DBName, int Port, int MaxConn, int close_log)
{
	m_url = url;
	m_Port = Port;
	m_User = User;
	m_PassWord = PassWord;
	m_DatabaseName = DBName;
	m_close_log = close_log;

	for (int i = 0; i < MaxConn; i++)
	{
		MYSQL *con = NULL;
		con = mysql_init(con);

		if (con == NULL)
		{
			LOG_ERROR("MySQL mysql_init(con) Error");
			exit(1);
		}

		con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DBName.c_str(), Port, NULL, 0);

		if (con == NULL)
		{
			fprintf(stderr, "MySQL Error: %s\n", mysql_error(con));
			LOG_ERROR("MySQL mysql_real_connect Error");
			exit(1);
		}
		connList.push_back(con);
		++m_free_conn;
	}

	reserve = sem(m_free_conn);

	m_MaxConn = m_free_conn;
}


//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *SrvConnectionPool::GetConnection()
{
	MYSQL *con = NULL;

	if (0 == connList.size())
		return NULL;

	reserve.wait();
	
	lock.lock();

	con = connList.front();
	connList.pop_front();

	--m_free_conn;
	++m_used_conn;

	lock.unlock();
	return con;
}

//释放当前使用的连接
bool SrvConnectionPool::ReleaseConnection(MYSQL *con)
{
	if (NULL == con)
		return false;

	lock.lock();

	connList.push_back(con);
	++m_free_conn;
	--m_used_conn;

	lock.unlock();

	reserve.post();
	return true;
}

//销毁数据库连接池
void SrvConnectionPool::DestroyPool()
{

	lock.lock();
	if (connList.size() > 0)
	{
		list<MYSQL *>::iterator it;
		for (it = connList.begin(); it != connList.end(); ++it)
		{
			MYSQL *con = *it;
			mysql_close(con);
		}
		m_used_conn = 0;
		m_free_conn = 0;
		connList.clear();
	}

	lock.unlock();
}

//当前空闲的连接数
int SrvConnectionPool::GetFreeConn()
{
	return this->m_free_conn;
}

SrvConnectionPool::~SrvConnectionPool()
{
	DestroyPool();
}

ConnectionRAII::ConnectionRAII(MYSQL **SQL, SrvConnectionPool *connPool){
	*SQL = connPool->GetConnection();
	
	conRAII = *SQL;
	poolRAII = connPool;
}

ConnectionRAII::~ConnectionRAII(){
	poolRAII->ReleaseConnection(conRAII);
}