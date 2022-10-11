#include <sys/epoll.h>

#include "./inc/server.h"
#include "./inc/http.h"
#include "./inc/threadPool.h"

#define IP_ADDR "127.0.0.1"
#define PORT 10086
#define USER_PATH "/home/linjie/cpp/class2/tempDir/"

using namespace std;

	void delFd(int epfd, int fd) {
		int ref = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
		if(ref < 0) {
			printf("error in epoll_ctl in delFd:%s\n", strerror(errno));
			return;
		}
		close(fd);
		printf("fd:%d leaved!\n", fd);
	}

int cb_recv(int epfd, int fd) {
	char buf[MAX_BUF_LEN] = {0};
	int read_len = recv(fd, buf, MAX_BUF_LEN, MSG_PEEK);	//处理cfd事件
	if(read_len < 0) {
		if(errno != EAGAIN || errno != EWOULDBLOCK) {
			printf("recv in epRun error:%s\n", strerror(errno));
			delFd(epfd, fd);
			return read_len;
		} else {
			return read_len;
		}
	} else if(read_len == 0) {
		delFd(epfd, fd);
		return 0;
	}
	//打印接收到的请求报文
	printf("%s\n", buf);

	//处理报文
	processDram(fd);

	//要断开，浏览器不会自动断开，要不然就会一直连接
	delFd(epfd, fd);

	return 0;
}

int main(void) {
	EpollServer server("localhost", PORT, cb_recv);
	server.epRun();
	return 0;
}
