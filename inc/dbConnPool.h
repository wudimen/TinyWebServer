#ifndef _DB_CONN_POOL_H_
#define _DB_CONN_POOL_H_

#include <mysql/mysql.h>			//g++ xxx.cpp -o xxx -lmysqlclient
#include <queue>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

using namespace std;

//#include <boost/make_shared.hpp>
//boost::shared_ptr<Net<float> > net_;

#define MIN_CONN_SIZE 10
#define MAX_CONN_SIZE 100

class DbConn{
public:
	//构造函数：完成初始化数据库对象、设置编码格式
	DbConn() {
		m_mysql = mysql_init(NULL);
		mysql_set_character_set(m_mysql, "utf8");
	}
	//析构函数：清理堆内存
	~DbConn() {
		if(m_mysql) {
			mysql_close(m_mysql);
		}
		clearResult();
	}
	//对象连接数据库
	bool init(const char *host, const char *user, const char *passwd, const char *db_name, const unsigned short port = 3306) {
		m_mysql = mysql_real_connect(m_mysql, host, user, passwd, db_name, port,  NULL, 0);
		return m_mysql != NULL;
	}
	//完成数据库删除、增加、更改操作
	bool update(const char *sql) {
		return 0 == mysql_query(m_mysql, sql);
	}
	//完成数据库查找操作
	bool query(const char *sql) {
		clearResult();
		//m_row = NULL;
		if(0 != mysql_query(m_mysql, sql)) {
			return false;
		}
		m_result = mysql_store_result(m_mysql);
		return m_result != NULL;
	}
	//获取查询结果
	bool next() {
		if(m_result) {
			m_row = mysql_fetch_row(m_result);
			return m_row != NULL;
		}
		return false;
	}
	//获取查询结果中的值
	char *value(int index) {
		int col = mysql_num_fields(m_result);
		if(index >= col || index < 0) {
			return NULL;
		}
		unsigned long leng = mysql_fetch_lengths(m_result)[index];
		memcpy(m_row_buf, m_row[index], leng);
		return m_row_buf;
	}
	//实务操作
	bool transaction() {
		return 0 == mysql_autocommit(m_mysql, false);
	}
	//事务保存
	bool submit() {
		return 0 == mysql_commit(m_mysql);
	}
	//事务回滚
	bool rollback() {
		return 0 == mysql_rollback(m_mysql);
	}

private:
	bool clearResult() {
		if(m_result) {
			mysql_free_result(m_result);
			m_result = NULL;
			return true;
		}
		return false;
	}

private:
	MYSQL *m_mysql = NULL;		//数据库对象
	MYSQL_RES *m_result = NULL;		//查询结果
	MYSQL_ROW m_row = NULL;			//查询结果每一行的值
	char m_row_buf[1024];
};

class DbConnPool{
public:
	//获取唯一的数据库连接池对象
	static DbConnPool *getConnPool() {
		static DbConnPool pool("192.168.85.1", "root", "root", "test_db");
		return &pool;
	}

	//获取数据库对象
/*
	shared_ptr<DbConn> getConn() {
		while(m_conns.empty()) {
			sem_wait(&m_queue_empty);
		}

		//采用智能指针的特性，指定智能指针的删除器，让其在释放时自动回到容器里（也可以再开一个函数，在对象用完后调用）
		pthread_mutex_lock(&m_queue_mutex);
		shared_ptr<DbConn> tmp_conn = make_shared(m_conns.front(), [this](DbConn *tmp_conn) {
			pthread_mutex_lock(&m_queue_mutex);
			m_conns.push(tmp_conn);
			pthread_mutex_unlock(&m_queue_mutex);
		});
		m_conns.pop();
		pthread_mutex_unlock(&m_queue_mutex);
		sem_post(&m_queue_full);
		return tmp_conn;
	}
*/
	DbConn *getConn() {
		while(m_conns.empty()) {
			sem_wait(&m_queue_empty);
		}

		//采用智能指针的特性，指定智能指针的删除器，让其在释放时自动回到容器里（也可以再开一个函数，在对象用完后调用）
		pthread_mutex_lock(&m_queue_mutex);
		DbConn *tmp_conn = m_conns.front();
		m_conns.pop();
		pthread_mutex_unlock(&m_queue_mutex);
		sem_post(&m_queue_full);
		return tmp_conn;
	}

	//析构函数：清理堆区内存
	~DbConnPool() {
		DbConn *tmp_conn = NULL;
		while(!m_conns.empty()) {
			tmp_conn = m_conns.front();
			m_conns.pop();
			delete tmp_conn;
		}
		sem_destroy(&m_queue_full);
		sem_destroy(&m_queue_empty);
		pthread_mutex_destroy(&m_queue_mutex);
	}

	//线程函数——增加数据库对象，防止队列空了
	void connProduser(){
		while(1) {
			while(m_conns.size() >= m_min_size) {
				sem_wait(&m_queue_full);
			}
			pthread_mutex_lock(&m_queue_mutex);
			addConn();
			pthread_mutex_unlock(&m_queue_mutex);
			sem_post(&m_queue_empty);
		}
	}

	static void *connProduser_sub(void *args) {
		DbConnPool *tmp_pool = (DbConnPool *)args;
		tmp_pool->connProduser();
		return tmp_pool;
	}

	void releaseConn(DbConn *conn) {
		//pthread_mutex_lock(&m_queue_mutex);
		m_conns.push(conn);
		//pthread_mutex_unlock(&m_queue_mutex);
	}	

private:
	//私有化构造函数，为了形成单例（只能有一个数据库连接池）
	DbConnPool(const char *host, const char *user, const char *passwd, const char *db_name, unsigned short port = 3306, int min_size = MIN_CONN_SIZE, int max_size = MAX_CONN_SIZE) :m_port(port), m_min_size(min_size), m_max_size(max_size) {
		strcpy(m_host, host);
		strcpy(m_user, user);
		strcpy(m_passwd, passwd);
		strcpy(m_db_name, db_name);

		sem_init(&m_queue_empty, 0, 0);
		sem_init(&m_queue_full, 0, 0);
		pthread_mutex_init(&m_queue_mutex, NULL);

		for(int i = 0; i < m_min_size; ++i) {
			pthread_mutex_lock(&m_queue_mutex);
			addConn();
			pthread_mutex_unlock(&m_queue_mutex);
		}

		pthread_t tid = -1;
		pthread_create(&tid, NULL, connProduser_sub, (void*)this);
		//pthread_create(&tid, NULL, connProduser, NULL);
		pthread_detach(tid);
	}

	//重用代码（增加一个数据库对象）
	void addConn() {
		DbConn *tmp_conn = new DbConn();
		tmp_conn->init(m_host, m_user, m_passwd, m_db_name, m_port);
		m_conns.push(tmp_conn);
	}
private:
	queue<DbConn*>  m_conns;

	char m_host[128];
	char m_user[128];
	char m_passwd[128];
	char m_db_name[128];
	unsigned short m_port;

	int m_min_size;
	int m_max_size;

	pthread_mutex_t m_queue_mutex;
	sem_t m_queue_empty;
	sem_t m_queue_full;
};

#endif
