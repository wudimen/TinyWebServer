/*
	注意：监听fd不能设置ET模式下非阻塞，不然将收不到连续的两个相同连接（当浏览器请求图标时会用到）！！
*/
#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
	
#include "threadPool.h"
	
#define MAX_EPOLL_NUMS 1024
#define MAX_BUF_LEN 1024
	
class data_1{
public:
	data_1(int ep, int (*cb)(int, int)) {
		epfd = ep;
		cb_recv = cb;
	}
public:
	int epfd;
	int fd;
	int (*cb_recv)(int , int);
	void func() {
		cb_recv(epfd, fd);
	}
};


class EpollServer{
public:
	EpollServer(const char *ip, const int port, int (*cb_recv)(int, int)) {
		strcpy(m_ip, ip);
		m_port = port;
		m_cb_recv = cb_recv;

		pool = new ThreadPool<data_1 *>();

		m_epfd = epoll_create(10);
		assert(m_epfd != -1);

		//users = new data_1[MAX_EPOLL_NUMS](m_epfd, m_cb_recv);
		users = new data_1*[MAX_EPOLL_NUMS];
		for(int i = 0; i < MAX_EPOLL_NUMS; ++i) {
			users[i] = new data_1(m_epfd, m_cb_recv);
		}
/*		for(int i = 0; i < MAX_EPOLL_NUMS; ++i) {
			users[fd].epfd = m_epfd;
			users[fd].recv_cb = m_cb_recv;
		}
*/
		startServer();
	}

	~EpollServer() {
		delete[] users;
		delete pool;
	}

	
	
	void epRun() {
		int nums = -1;
		epoll_event evs[MAX_EPOLL_NUMS];
		while(1) {
			memset(evs, 0, sizeof(epoll_event) * MAX_EPOLL_NUMS);
			nums = epoll_wait(m_epfd, evs, MAX_EPOLL_NUMS, -1);	//epoll描述符， 有时间的对象放置点， 一次性接收的最大对象数量， 阻塞时间（-1：永久）
			for(int i = 0; i < nums; ++i) {
				int tmp_fd = evs[i].data.fd;
				if(tmp_fd == lfd) {					//处理lfd事件
					struct sockaddr_in clt_addr;
					socklen_t addr_len = sizeof(clt_addr);
					int cfd = accept(lfd, (struct sockaddr*)&clt_addr, &addr_len);
	printf("***********fd:%d is used!\n", cfd);
					if(cfd < 0) {
						printf("accept in epRun error:%s\n", strerror(errno));
						continue;
						}
					addFd(cfd);
					continue;
				} else {
					pool->addTask(users[tmp_fd]);
					//m_cb_recv(m_epfd, tmp_fd);
				}
			}
		}
	}

private:
	//设置fd非阻塞（所有通信fd(除lfd)）
	void setNonBlock(int fd) {
		int old_option = fcntl(fd, F_GETFL);
		int new_option = old_option | O_NONBLOCK;
		fcntl(fd, F_SETFL, new_option);
	}
	
	//添加epoll监听事件（ET模式）
	void addFd(int fd) {
		epoll_event tmp_ev;
		tmp_ev.events = EPOLLIN | EPOLLET;
		tmp_ev.data.fd = fd;
		epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &tmp_ev);
		setNonBlock(fd);

		printf("fd:%d connected!\n", fd);
		users[fd]->fd = fd;
	}
	
	void startServer() {
		struct sockaddr_in server_addr;
		bzero(&server_addr, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(m_port);
		inet_pton(AF_INET, m_ip, &server_addr.sin_addr);
	
		lfd = socket(AF_INET, SOCK_STREAM, 0);
		assert(lfd >= 0);
	
		//设置端口复用
		int opt = 1;
		int ref = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, (void *)&opt, sizeof(opt));
		if(ref < 0) {
			printf("error in setsockopt in startServer:%s\n", strerror(errno));
			return;
		}
	
		ref = bind(lfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
		if(ref < 0) {
			printf("error in bind in startServer:%s\n", strerror(errno));
			return;
		}
	
		ref = listen(lfd, 10);
		assert(ref != -1);
	
		epoll_event tmp_ev;
		tmp_ev.events = EPOLLIN;
		tmp_ev.data.fd = lfd;
		epoll_ctl(m_epfd, EPOLL_CTL_ADD, lfd, &tmp_ev);
	}
private:
	char m_ip[32];
	int m_port;
	int m_epfd;
	int lfd;
	int (*m_cb_recv)(int, int);
	ThreadPool<data_1 *> *pool;
	data_1 **users;
};	
	
#endif
