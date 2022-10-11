/*
	采用升序链表结构，构成一个定时器，进行对非活跃用户的清除
*/

#ifndef _TIMER_H_
#define _TIMER_H_

#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>

class Timer;

class UserData{
public:
	struct sockaddr_in address;
	int sockfd;
	Timer* timer;
};

class Timer{
public:
	Timer(time_t wait_time) {
ID = wait_time;
		time_t now = time(NULL);
		m_time = now + wait_time;
		prev = NULL;
		next = NULL;
		func = NULL;
		data = NULL;
	}
public:
	time_t m_time;
	void (*func)(UserData*);
	UserData* data;
	Timer* prev;
int ID;
	Timer* next;
};

class TimerList{
public:
	TimerList() : head(NULL), tail(NULL) {};
	time_t getMinTime() {
		if(head) {
			time_t now = time(NULL);
			return head->m_time - now;
		}
		return -1;
	}
	void addTimer(Timer* timer) {
		if(!timer) return;
		if(!head) {			//节点异常
			head = timer;
			tail = timer;
			return;
		}
		Timer* tmp = head;
		while(tmp) {
			if(tmp->m_time > timer->m_time) {		//中间插入
				if(tmp->prev) {
					tmp->prev->next = timer;
				} else {
					head = timer;
				}
				timer->prev = tmp->prev;
				tmp->prev = timer;
				timer->next = tmp;
				break;
			}
			tmp = tmp->next;
		}
		if(!tmp) {				//应插入在最后
			tail->next = timer;
			timer->prev = tail;
			tail = timer;
		}
	}

	void delTimer(Timer* timer, bool rm_now = true) {
		if(!timer) return;
		if(!rm_now) {
			timer->func = NULL;
			return;
		}
		if(timer == head) {		//删除节点为首节点
			head = head->next;
			if(head) {
				head->prev = NULL;
			} else {
				tail = NULL;
			}
			delete timer;
		}
		Timer* tmp = head->next;
		while(tmp) {
			if(timer == tmp) {
				timer->prev->next = timer->next;
				if(timer->next) {
					timer->next->prev = timer->prev;
				} else {
					tail = timer->prev;
				}
				delete timer;
			}
			tmp = tmp->next;
		}
	}

	void delayTimer(Timer* timer, time_t wait_time) {
		if(!timer) return;
		timer->m_time = time(NULL) + wait_time;
		Timer* tmp = timer->next;
		if(!tmp || timer->m_time < tmp->m_time) {	//若节点在链表尾部或修改后的值仍小于后面的值，则不许改变位置
			return;
		}
		if(timer == head) {
			head = head->next;
			if(head) {
				head->prev = NULL;
			}
			timer->next = NULL;
		} else {
			timer->prev->next = timer->next;
			timer->next->prev = timer->prev;
			timer->next = NULL;
			timer->prev = NULL;
		}
		addTimer(timer);
	}

	void tick() {
		time_t now = time(NULL);
		if(head->m_time > now) {
			return;
		}
		while(head && head->m_time <= now) {
printf("func runing...\n");
			if(head->func) {
				head->func(head->data);
			}
			Timer* tmp = head;
			head = head->next;
			delete tmp;
			if(head) {
				head->prev = NULL;
			}
		}
		if(!head) {
			tail = NULL;
		}
	}
	void printList() {
		printf("正序：");
		Timer* tmp = head;
		while(tmp) {
			printf("%d ", tmp->ID);
			tmp = tmp->next;
		}
		printf("\n倒序：");
		tmp = tail;
		while(tmp) {
			printf("%d ", tmp->ID);
			tmp = tmp->prev;
		}
		printf("\n");	
	}
private:
	Timer* head;
	Timer* tail;
};

#endif
