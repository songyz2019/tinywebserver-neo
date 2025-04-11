#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string>
#include "locker.h"


using namespace std;

class SqlConnectionPool
{
public:
	MYSQL *GetConnection();				 //获取数据库连接
	bool ReleaseConnection(MYSQL *conn); //释放连接
	int GetFreeConn();					 //获取连接
	void DestroyPool();					 //销毁所有连接

	//单例模式
	static SqlConnectionPool* GetInstance();

	void init(string url, string User, string PassWord, string DataBaseName, int Port, int MaxConn, int close_log); 

private:
	SqlConnectionPool() = default;
	~SqlConnectionPool();

	int mMaxConn;  //最大连接数
	int mUsedConn {};  //当前已使用的连接数
	int mFreeConn {}; //当前空闲的连接数
	Locker lock;
	list<MYSQL *> mConnList; //连接池
	Sem mSemConn;

public:
	string m_url;			 //主机地址
	string mPort;		 //数据库端口号
	string mUsername;		 //登陆数据库用户名
	string mPassword;	 //登陆数据库密码
	string mDbName; //使用数据库名
	int mDisableLogging;	//日志开关
};

class ConnectionRAII{

public:
	ConnectionRAII(MYSQL **con, SqlConnectionPool *connPool);
	~ConnectionRAII();
	
private:
	MYSQL *conRAII;
	SqlConnectionPool *poolRAII;
};

#endif
