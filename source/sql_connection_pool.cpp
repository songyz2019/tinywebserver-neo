#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include "sql_connection_pool.h"
#include "log.h"
#include <assert.h>

using namespace std;

SqlConnectionPool *SqlConnectionPool::GetInstance()
{
	static SqlConnectionPool connPool;
	return &connPool;
}

//构造初始化
// 创建connList,和reserve信号量
void SqlConnectionPool::init(string host, string username, string password, string dbname, int port, int maxconn, int close_log)
{
	m_url = host;
	mPort = port;
	mUsername = username;
	mPassword = password;
	mDbName = dbname;
	mDisableLogging = close_log;
	

	// 这里应该是作者本来想做成能申请几个就申请几个，但是用exit退出导致代码没有做到
	for (int i = 0; i < maxconn; i++)
	{
		MYSQL *con = NULL;
		con = mysql_init(con);
		if (con == NULL)
		{
			LOG_ERROR("MySQL Init failed");
			continue;
		}

		con = mysql_real_connect(con, host.c_str(), username.c_str(), password.c_str(), dbname.c_str(), port, NULL, 0);
		if (con == NULL)
		{
			LOG_ERROR("MySQL mysql_real_connect failed");
			continue;
		}

		mConnList.push_back(con);
		++mFreeConn;
	}

	mSemConn = Sem(mFreeConn);
	mMaxConn = mFreeConn;
}


//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *SqlConnectionPool::GetConnection()
{
	MYSQL *con = NULL;

	if (0 == mConnList.size())
		return NULL;

	mSemConn.wait(); // 信号量的减一或等待模式
	
	lock.lock();
		con = mConnList.front();
		mConnList.pop_front();

		--mFreeConn;
		++mUsedConn;
	lock.unlock();
	return con;
}

//释放当前使用的连接
bool SqlConnectionPool::ReleaseConnection(MYSQL *con)
{
	if (NULL == con)
		return false;

	lock.lock();
		mConnList.push_back(con);
		++mFreeConn;
		--mUsedConn;
	lock.unlock();

	mSemConn.post();
	return true;
}

//销毁数据库连接池
void SqlConnectionPool::DestroyPool()
{
	lock.lock();
		if (mConnList.size() > 0)
		{
			list<MYSQL *>::iterator it;
			for (it = mConnList.begin(); it != mConnList.end(); ++it)
			{
				MYSQL *con = *it;
				mysql_close(con);
			}
			mUsedConn = 0;
			mFreeConn = 0;
			mConnList.clear();
		}
	lock.unlock();
}

//当前空闲的连接数
int SqlConnectionPool::GetFreeConn()
{
	return this->mFreeConn;
}

SqlConnectionPool::~SqlConnectionPool()
{
	DestroyPool();
}

ConnectionRAII::ConnectionRAII(MYSQL **SQL, SqlConnectionPool *connPool){
	*SQL = connPool->GetConnection();
	
	conRAII = *SQL;
	poolRAII = connPool;
}

ConnectionRAII::~ConnectionRAII(){
	poolRAII->ReleaseConnection(conRAII);
}