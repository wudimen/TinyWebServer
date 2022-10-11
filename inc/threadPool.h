/*
	
*/

#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <queue>
#include <pthread.h>
#include <semaphore.h>

using namespace std;

template<typename T>
class Tids{
public:
	Tids() {};

	~Tids() {
		delete[] m_ids;
	}

	void init(int nu) {
		m_nums = nu;
		m_ids = new T[m_nums];
		for(int i = 0; i < m_nums; ++i) {
			m_ids[i] = -1;
		}
	}

	bool addId(T tid) {
		for(int i = 0; i < m_nums; ++i) {
			if(m_ids[i] == -1) {
				m_ids[i] = tid;
				return true;
			}
		}
		return false;
	}

	bool delId(T tid) {
		for(int i = 0; i < m_nums; ++i) {
			if(m_ids[i] == tid) {
				m_ids[i] = -1;
				return true;
			}
		}
		return false;
	}
public:
	int m_nums;
	T* m_ids;
};

template<typename TASK>
class ThreadPool{
public:
	//完成初始化类参数、最少数线程的创建与线程管理线程的创建
	ThreadPool(int min_size = 3, int max_size = 100, int max_tasks = 300) : m_min_threads(min_size), m_max_threads(max_size), m_alive_threads(min_size), m_max_tasks(max_tasks), m_destroy(false), m_kill_threads(0) {
		sem_init(&m_sem, 0, 0);
		pthread_mutex_init(&m_queue_mutex, NULL);
		pthread_mutex_init(&m_kill_mutex, NULL);

		m_tids.init(m_max_threads);
		for(int i = 0; i < m_min_threads; ++i) {
			pthread_create(&m_tids.m_ids[i], NULL, Worker_sub, (void*)this);
			pthread_detach(m_tids.m_ids[i]);
		}

		pthread_create(&manage_tid, NULL, ManageThread_sub, (void*)this);
		pthread_detach(manage_tid);
	}

	//完成堆变量的销毁、线程的销毁
	~ThreadPool() {
		m_destroy = true;

		pthread_mutex_destroy(&m_queue_mutex);
		pthread_mutex_destroy(&m_kill_mutex);
		sem_destroy(&m_sem);
	}

	//增加任务
	bool addTask(TASK task) {
		pthread_mutex_lock(&m_queue_mutex);
		if(tasks.size() >= m_max_tasks) {
			pthread_mutex_unlock(&m_queue_mutex);
			return false;
		}
		tasks.push(task);
		pthread_mutex_unlock(&m_queue_mutex);
		sem_post(&m_sem);
		return true;
	}
private:
	static void *ManageThread_sub(void *args) {
		ThreadPool* pool = (ThreadPool*)args;
		pool->ManageThread();
		return pool;
	}

	static void *Worker_sub(void *args) {
		ThreadPool* pool = (ThreadPool*)args;
		pool->Worker();
		return pool;
	}

	//管理线程函数
	void ManageThread() {
		while(1) {
			//printf("manage thread works...\n");
			sleep(1);
		}
	}

	//工作线程函数
	void Worker() {
		while(!m_destroy) {
			while(tasks.empty()) {			//等待任务到来
				printf("%d waiting...!\n", (int)pthread_self());
				sem_wait(&m_sem);
				if(m_kill_threads > 0) {		//执行管理线程指令，销毁多余线程
					pthread_mutex_lock(&m_kill_mutex);
					m_kill_threads--;
					pthread_mutex_unlock(&m_kill_mutex);
					break;
				}
			}
			pthread_mutex_lock(&m_queue_mutex);
			TASK tmp = tasks.front();
			tasks.pop();
			pthread_mutex_unlock(&m_queue_mutex);
			if(tmp) {
				printf("%d works...!\n", (int)pthread_self());
				tmp->func();
			}
		}
		m_tids.delId(pthread_self());
		m_alive_threads--;
	}
private:
	int m_min_threads;		//最小的线程数
	int m_max_threads;		//最大的线程数
	int m_alive_threads;	//现存的线程数

	queue<TASK> tasks;		//任务队列
	int m_max_tasks;		//最大的任务数（过多的话影响任务执行效率）

	Tids<pthread_t> m_tids;		//任务线程id
	pthread_t manage_tid;	//管理线程id

	int m_kill_threads;		//待销毁的线程数量
	bool m_destroy;			//是否销毁整个线程池

	sem_t m_sem;			//是否有任务		//sem_wait(&m_sem);		sem_post(&m_sem);
	pthread_mutex_t m_queue_mutex;		//任务队列的锁		//pthread_mutex_lock(&m_queue_mutex);	pthread_mutex_unlock(&m_queue_mutex);
	pthread_mutex_t m_kill_mutex;		//待杀死线程数的锁
};


#endif
