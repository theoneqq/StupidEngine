#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>

#define IPADDRESS "127.0.0.1"
#define SERVER_PORT 6666
#define MAXSIZE 1024
#define FDSIZE 1000
#define EPOLLEVENTS 20

void handle_connection(int sockfd);
void handle_events(int epollfd, struct epoll_event* events, int num, int listenfd, char* buff);
void do_read(int epollfd, int fd, int sockfd, char* buff);
void do_write(int epollfd, int fd, int sockfd, char* buff);
void add_event(int epollfd, int fd, int state);
void modify_event(int epollfd, int fd, int state);
void delete_event(int epollfd, int fd, int state);
int count = 0;

int main() {
	int sockfd;
	struct sockaddr_in serveraddr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, IPADDRESS, &serveraddr.sin_addr);
	connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	handle_connection(sockfd);
	close(sockfd);
	return 0;
}

void handle_connection(int sockfd) {
	int epollfd;
	struct epoll_event events[EPOLLEVENTS];
	char buff[MAXSIZE];
	int ret;
	epollfd = epoll_create(FDSIZE);
	add_event(epollfd, STDIN_FILENO, EPOLLIN);
	while (1) {
		ret = epoll_wait(epollfd, events, EPOLLEVENTS, -1);
		handle_events(epollfd, events, ret, sockfd, buff);
	}
	close(epollfd);
}

void handle_events(int epollfd, struct epoll_event* events, int num, int sockfd, char* buff) {
	int i, fd;
	for (i = 0; i < num; ++i) {
		fd = events[i].data.fd;
		if (events[i].events & EPOLLIN) {
			do_read(epollfd, fd, sockfd, buff);
		} else if (events[i].events & EPOLLOUT) {
			do_write(epollfd, fd, sockfd, buff);
		}

	}
}

void do_read(int epollfd, int fd, int sockfd, char* buff) {
	int nread = read(fd, buff, MAXSIZE);
	if (nread == -1) {
		perror("read error: ");
		close(fd);
		delete_event(epollfd, fd, EPOLLIN);
	} else if (nread == 0) {
		fprintf(stderr, "server close.\n");
		close(fd);
		delete_event(epollfd, fd, EPOLLIN);
	} else {
		if (fd == STDIN_FILENO) {
			add_event(epollfd, sockfd, EPOLLOUT);
		} else {
			delete_event(epollfd, sockfd, EPOLLIN);
			add_event(epollfd, STDOUT_FILENO, EPOLLOUT);
		}
	}
}

void do_write(int epollfd, int fd, int sockfd, char* buff) {
	int nwrite;
	char temp[1000];
	buff[strlen(buff) - 1] = '\0';
	snprintf(temp, sizeof(temp), "%s_%02d\n", buff, count++);
	nwrite = write(fd, temp, strlen(temp));
	if (nwrite == -1) {
		perror("write error: ");
		close(fd);
	} else {
		if (fd == STDOUT_FILENO) {
			delete_event(epollfd, fd, EPOLLOUT);
		} else {
			modify_event(epollfd, fd, EPOLLIN);
		}
	}
	memset(buff, 0, MAXSIZE);
}

void add_event(int epollfd, int fd, int state) {
	struct epoll_event ev;
	ev.events = state;
	ev.data.fd = fd;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}

void delete_event(int epollfd, int fd, int state) {
	struct epoll_event ev;
	ev.events = state;
	ev.data.fd = fd;
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
}


void modify_event(int epollfd, int fd, int state) {
	struct epoll_event ev;
	ev.events = state;
	ev.data.fd = fd;
	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}







































