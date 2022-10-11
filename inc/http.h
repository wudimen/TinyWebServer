//请求报文
//GET
// GET /hhhh.com ==>111.html?user_1=6666&password_1=7777  HTTP/1.1
// Host: 192.168.85.130:10086
// Connection: keep-alive
// Upgrade-Insecure-Requests: 1
// User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36 Edg/105.0.1343.53
// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9
// Accept-Encoding: gzip, deflate
// Accept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6

//POST:
// ST /000 HTTP/1.1
// Host: 192.168.85.130:10086
// Connection: keep-alive
// Content-Length: 0
// Cache-Control: max-age=0		//提交的内容在浏览器中的‘保质期’，maz-age时间内有效
// Upgrade-Insecure-Requests: 1			//请求将http协议升级为https服务
// Origin: http://192.168.85.130:10086		//提交内容的主机进程
// Content-Type: application/x-www-form-urlencoded		//提交的内容类型为原生表单
// User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/106.0.0.0 Safari/537.36 Edg/106.0.1370.34
// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9
// Referer: http://192.168.85.130:10086/form.html
// Accept-Encoding: gzip, deflate
// Accept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6
// 
// user=666&password=777

//回应报文
// HTTP/1.1 400 Bad Request
// Content-Type: text/html; charset=us-ascii
// Server: Microsoft-HTTPAPI/2.0
// Date: Sat, 01 Oct 2022 12:12:12 GMT
// Connection: close
// Content-Length: 326

// <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN""http://www.w3.org/TR/html4/strict.dtd">
// <HTML><HEAD><TITLE>Bad Request</TITLE>
// <META HTTP-EQUIV="Content-Type" Content="text/html; charset=us-ascii"></HEAD>
// <BODY><h2>Bad Request - Invalid Verb</h2>
// <hr><p>HTTP Error 400. The request verb is invalid.</p>
// </BODY></HTML>define _HTTP_H_

/*
	请求套首行中的间隔符是 32 ，没有用 \+? 表示，只能用 ascII 32 表示，行结尾是：\r\n。	即首行：GET'32'/hhhh.com'32'HTTP/1.1'\r''\n'...
*/

#ifndef _HTTP_H_
#define _HTTP_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "./dbConnPool.h"

#define MAX_BUF_LEN 1024
#define USER_PATH "/home/linjie/cpp/class2/tempDir/"
#define ERR_PAGE "/home/linjie/cpp/class2/tempDir/err.txt"

int readLine(char* buf, int* lens, char *body, char *cookie) {
	int len = (*lens);
	int i = 0;
	while(i < len && (buf[i] != '\r' || i == len-1 || buf[i+1] != '\n')) ++i;
	if(i == len) {
		printf("no line!\n");
		return -1;
	}

	if(strstr(buf, "Cookie")) {
		strcpy(cookie, strstr(buf, "Cookie") + 15);
printf("Cookie : %s\n", cookie);
	}
	int o = 0;
	int cookie_len = strlen(cookie);
	while(o < cookie_len && !(body[o] == '\r' && body[o+1] == '\n')) o++;
	if(o != cookie_len) cookie[o] = '\0'; else cookie[cookie_len - 1] = '\0';

	buf[i] = '\0';
	int length = *lens;
	(*lens) = i;

	while(!(i < (length - 3) && buf[i] == '\r' && buf[i+1] == '\n' && buf[i+2] == '\r' && buf[i+3] == '\n')) i++;
	strcpy(body, buf + i + 4);
	return 0;
}

//获取请求方式、路径、版本号
int analyseHead(char *line, int len_0, char *method, char *path, char *prot, bool flag = false) {
	if(flag) {
		sscanf(line, "%[^ ] %[^ ] %[^ ]", method, path, prot);
		return 0;
	}
	int len = 0, len_1 = 0;
	while(len < len_0 && line[len] != 32) method[len_1++] = line[len++];
	len += 1;
	method[len_1] = '\0';
//printf("method:%s\n", method);
//for(int j = 0; j < len_1; ++j) {
//	printf("%d ", method[j]);
//}
	len_1 = 0;
	while(len < len_0 && line[len] != 32) path[len_1++] = line[len++];
	len += 1;
	path[len_1] = '\0';
//printf("path:%s\n", path);
	len_1 = 0;
	while(len < len_0 && line[len] != '\r') prot[len_1++] = line[len++];
	prot[len_1] = '\0';
//printf("prot:%s\n", prot);
	if(len > len_0) {
		printf("error head!\n");
		return -1;
	}
	return 0;
}


const char* getTypeByName(const char* name) {
	int i = 0, len = strlen(name);
	char tail[256] = {0};
	while(i < len && name[i] != '.') ++i;
	if(++i == len) return NULL;
	strcpy(tail, name+i);
	if(tail == NULL) 
		return "text/plain; charset=utf-8";
	if(strcmp(tail, "html") == 0 || strcmp(tail, "htm") == 0)
		return "text/html; charset=utf-8";
	if(strcmp(tail, "jpg") == 0 || strcmp(tail, "jpeg") == 0)
		return "image/jpeg";
	if(strcmp(tail, "mov") == 0 || strcmp(tail, "qt") == 0)
		return "video/quicktime";
	if(strcmp(tail, "mpeg") == 0 || strcmp(tail, "mpe") == 0)
		return "video/mpeg";
	if(strcmp(tail, "vrml") == 0 || strcmp(tail, "wrl") == 0)
		return "model/vrml";
	if(strcmp(tail, "midi") == 0 || strcmp(tail, "mid") == 0)
		return "audio/midi";
	if(strcmp(tail, "gif") == 0)
		return "image/gif";
	if(strcmp(tail, "png") == 0)
		return "image/png";
	if(strcmp(tail, "css") == 0)
		return "text/css";
	if(strcmp(tail, "au") == 0)
		return "audio/basic";
	if(strcmp(tail, "wav") == 0)
		return "audio/wav";
	if(strcmp(tail, "avi") == 0)
		return "video/x-msvideo";
	if(strcmp(tail, "mp3") == 0)
		return "audio/mpeg";
	if(strcmp(tail, "ogg") == 0)
		return "application/ogg";
	if(strcmp(tail, "pac") == 0)
		return "application/x-ns-proxy-autoconfig";

	return "text/plain; charset=utf-8";
}
	//打开文件/目录
	//判断文件/目录/是否存在
	//文件
	//	封装回应头
	//		获取文件后缀、文件大小
	//		发送相应报文文件头
	//		发送文件内容
	//目录
	//	查询目录下所有文件（区分文件与目录），并构成.html文件，发送
	//为空
	//	编辑html文件，设置文件类型为.html 并发送

int sendHead(int fd, int NO, const char* disp, const char *type, char *cookie = NULL) {
	char buf[1024] = {0};

	sprintf(buf + strlen(buf), "HTTP/1.1 %d %s\r\n", NO, disp);
	sprintf(buf + strlen(buf), "Content-Type: %s\r\n", type);
	sprintf(buf + strlen(buf), "Content-Length:%d\r\n", -1);		//要么填-1系统自动补齐，要么填准确
	if(cookie) sprintf(buf + strlen(buf), "Set-Cookie: UserID=%s;Max-Age=3600;Version=1\r\n", cookie);
	sprintf(buf + strlen(buf), "Connection: close\r\n\r\n");			//此次发送后连接是否关闭

	int ref = send(fd, buf, strlen(buf), 0);
	if(ref < 0) {
		printf("error in send in sendHead:%s\n", strerror(errno));
		return -1;
	}
	
	return 0;
}

int sendFile(int fd, const char *all_path) {
	int ffd = open(all_path, O_RDONLY);
	if(ffd < 0) {
		printf("open in sendFile error:%s\n", strerror(errno));
		return -1;
	}
	int ref = -1;
	char buf[MAX_BUF_LEN] = {0};

	while((ref = read(ffd, buf, MAX_BUF_LEN)) >= 0) {
//printf("file length:%d\n", ref);
		if(ref == 0) {
			if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
				usleep(100000);
				continue;
			} else {
				break;
			}
		}
		int ret = -1;
		ret = send(fd, buf, ref, 0);
		if(ret <= 0) {
			printf("send in sendFile error:%s\n", strerror(errno));
			return -1;
		}
	}
	if(ref < 0) {
		printf("read in sendFile error:%s\n", strerror(errno));
		close(ffd);
		return -1;
	}
	//close(ffd);
	return 0;
}

int sendDir(int fd, const char *all_path) {
	DIR *d;
	struct dirent * file;
	struct stat st;
	char buf[1024] = {0};
	d = opendir(all_path);
	if(!d) {
		printf("error in opendir in sendDir:%s\n", strerror(errno));
		close(fd);
		return -1;
	}
	//封装文件头部
	sprintf(buf, "<!DOCTYPE html>\n<html>\n<head>\n<title>%s</title>\n</head>\n<meta charset=\"utf-8\">\n<body bgcolor=\"white\">\n", all_path);
	while((file = readdir(d))) {
		const char *name = file->d_name;
		char all_name[128] = {0};
		strcpy(all_name, all_path);
		if(all_name[strlen(all_name) - 1] != '/') 
			strcpy(all_name + strlen(all_name), "/");
		strcpy(all_name + strlen(all_name), name);
		int ref = stat(all_name, &st);
		if(S_ISDIR(st.st_mode)) {
			//添加目录超链接
			sprintf(buf + strlen(buf), "<a href = \"%s\">%s</a><p><hr>", name, name);
		} else if(S_ISREG(st.st_mode)) {
			//添加文件超链接
			sprintf(buf + strlen(buf), "<a href = \"%s\">%s</a><p><hr>", name, name);
		}
	}
	//封装文件尾部
	sprintf(buf + strlen(buf), "</body>\n</html>\n");
	send(fd, buf, strlen(buf), 0);

	closedir(d);
	//close(fd);
	return 0;
}

int resp(int fd, const char *all_path, char *cookie = NULL) {
	const char *type = NULL;
	struct stat stats;
	int ref = stat(all_path, &stats);
	if(ref == -1) {		//无文件，发送404界面
printf("NOT A FILE!\n");
		sendHead(fd, 404, "Not Found!", getTypeByName("..."), cookie);
		sendFile(fd, ERR_PAGE);
	}
	if(S_ISDIR(stats.st_mode)) {		//是目录
printf("IS A DIR!\n");
		sendHead(fd, 502, "OK!", getTypeByName("XXX.html"), cookie);
		sendDir(fd, all_path);
	} else if(S_ISREG(stats.st_mode)) {
printf("IS A REG!\n");
		const char *type = getTypeByName(all_path);
printf("type:%s\n", type);
		sendHead(fd, 502, "OK!", type, cookie);
		sendFile(fd, all_path);
	}
}

int regist_func(char *name, char *password) {
	char sql[1024] = {0};
	DbConnPool *pool = DbConnPool::getConnPool();
	DbConn *conn = pool->getConn();

	sprintf(sql, "select * from webserver where name='%s';", name);
	conn->query(sql);
	if(conn->next()) {
		pool->releaseConn(conn);
		return -1;		//已有该用户
	}

	sprintf(sql, "insert into webserver(name, password) values(%s, %s);", name, password);
	pool->releaseConn(conn);
	if(!conn->update(sql))	{
		return 1;		//插入失败
	}
	return 0;		//插入成功
}

int login_func(char *name, char *password) {
	char sql[1024] = {0};
	DbConnPool *pool = DbConnPool::getConnPool();
	DbConn *conn = pool->getConn();

	sprintf(sql, "select * from webserver where name='%s';", name);
	conn->query(sql);
	if(!conn->next()) {
		pool->releaseConn(conn);
		return -1;		//无该用户
	}
	char realPass[20] = {0};
	strcpy(realPass, conn->value(1));
printf("***in login_func() pass:%s\treadPass:%s\n", password, realPass);
	pool->releaseConn(conn);
	if(strcmp(password, realPass)) {
		return 1;		//密码错误
	}
	return 0;		//密码正确
}

/*
striFindSub(char *str_1, char *str_2, char *str_3, char *buf) {
	int i = 0, j = 0;
	while(str_1[i] != '\0' &&str_2[j] != '\0') {
		if(str_1[i] == str_2[j]) {
			i++;
			j++;
		} else {
			i = 0;
		}
	}
}
*/
int getStr(char *str_1, char begin, char end, char *buf) {
	int len = strlen(str_1);
	int i = 0;
	while(i < len && str_1[i] != begin) ++i;
	int j = i;
	while( j < len && str_1[j] != end) ++j;
	if(j == len && end != '\0') return -1;
	strncpy(buf, str_1 + i + 1, j - i - 1);
	return j;
}

int processDram(int fd) {
/*接收报文*/
	bool cookie_flag = true;
	char *cookie = NULL;
	char buf[1024] = {0};
	char body[1024] = {0};
	char recv_cookie[128] = {0};
	int read_len = recv(fd, buf, MAX_BUF_LEN, 0);	//处理cfd事件

/*分析报文*/
	//获取首行
	int ref = readLine(buf, &read_len, body, recv_cookie);
	if(ref == -1) return -1;
printf("body:%s\tcookie:%s\tcookie_len:%d\n", body, recv_cookie, (int)strlen(recv_cookie));
	if(strlen(recv_cookie) <= 1) cookie_flag = false;

	//获取请求的方法、路径、版本号
	char method[128], path[128], prot[128];
	ref = analyseHead(buf, strlen(buf), method, path, prot);
	if(ref == -1) return -1;
//printf("method:%s\tpath:%s\tprot:%s\n", method, path, prot);

/*逻辑处理*/
	char all_path[128] = {0};
	strcpy(all_path, USER_PATH);

	//处理 GET 请求
	if(!strcmp(method, "GET")  || cookie_flag) {
if(cookie_flag) printf("有cookie，直接反馈页面，无需登录!\n");
		strcpy(all_path + strlen(all_path), path + (path[0] == '/' ? 1 : 0));
//printf("all_path:%s\n", all_path);

	//处理 POST 请求
	} else if(!strcmp(method, "POST")) {
		char name[128] = {0};
		char passwd[128] = {0};

		if(!strcmp(path, "/regist")) {		//注册
			int ref = getStr(body, '=', '&', name);
			getStr(body + ref, '=', '\0', passwd);
printf("name:%s\tpasswd:%s\n", name, passwd);
			ref = regist_func(name, passwd);
printf("regist_func return value:%d\n", ref);
			if(!ref) {
				char temp_cookie[128] = "1234567890987654321";
				cookie = temp_cookie;
				strcpy(all_path + strlen(all_path), "/register.html");
			} else {
				strcpy(all_path + strlen(all_path), "/err.html");
			}
		} else if(!strcmp(path, "/login_check")) {		//登录
			int ref = getStr(body, '=', '&', name);
			getStr(body + ref, '=', '\0', passwd);
printf("name:%s\tpasswd:%s\n", name, passwd);
			ref = login_func(name, passwd);
printf("login_func return value:%d\n", ref);
			if(!ref) {
				char temp_cookie[128] = "1234567890987654321";
				cookie = temp_cookie;
				strcpy(all_path + strlen(all_path), "/login.html");
			} else {
				strcpy(all_path + strlen(all_path), "/err.html");
			}
		}
	}

/*回送报文*/
	resp(fd, all_path, cookie);
}

#endif
