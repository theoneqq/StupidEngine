#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>

#define IPADDRESS "127.0.0.1"
#define PORT 6666
#define MAXSIZE 1024
#define LISTENQ 5
#define FDSIZE 1000
#define EPOLLEVENTS 100

int socket_bind(const char* ip, int port);
void do_epoll(int listenfd);
void handle_events(int epollfd, struct epoll_event* events, int num, int listenfd, char* buff);
void handle_accept(int epollfd, int listenfd);
void do_read(int epollfd, int fd, char* buff);
void do_write(int epollfd, int fd, char* buff);
void add_event(int epollfd, int fd, int state);
void modify_event(int epollfd, int fd, int state);
void delete_event(int epollfd, int fd, int state);

int main() {
	int listenfd = socket_bind(IPADDRESS, PORT);
	listen(listenfd, LISTENQ);
	do_epoll(listenfd);
	return 0;
}

int socket_bind(const char* ip, int port) {
	int listenfd;
	struct sockaddr_in server_addr;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd == -1) {
		perror("socket error: ");
		exit(1);
	}
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &server_addr.sin_addr);
	server_addr.sin_port = htons(port);
	if (bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
		perror("bind error: ");
		exit(1);
	}
	return listenfd;
}

void do_epoll(int listenfd) {
	int epollfd;
	struct epoll_event events[EPOLLEVENTS];
	int ret;
	char buff[MAXSIZE];
	memset(buff, 0, MAXSIZE);
	epollfd = epoll_create(FDSIZE);
	add_event(epollfd, listenfd, EPOLLIN);
	while (1) {
		ret = epoll_wait(epollfd, events, EPOLLEVENTS, -1);
		handle_events(epollfd, events, ret, listenfd, buff);
	}
	close(epollfd);
}

void handle_events(int epollfd, struct epoll_event* events, int num, int listenfd, char* buff) {
	int i, fd;
	for (i = 0; i < num; ++i) {
		fd = events[i].data.fd;
		if ((fd == listenfd) && (events[i].events & EPOLLIN)) {
			handle_accept(epollfd, listenfd);
		} else if (events[i].events & EPOLLIN) {
			do_read(epollfd, fd, buff);
		} else if (events[i].events & EPOLLOUT) {
			do_write(epollfd, fd, buff);
		}

	}
}

void handle_accept(int epollfd, int listenfd) {
	int clifd;
	struct sockaddr_in cliaddr;
	socklen_t cliaddrlen;
	clifd = accept(listenfd, (struct sockaddr*)&cliaddr, &cliaddrlen);
	if (clifd == -1) {
		perror("accept error: ");
	} else {
		printf("accept a new client: %s:%d\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
		add_event(epollfd, clifd, EPOLLIN);
	}
}

void do_read(int epollfd, int fd, char* buff) {
	int nread = read(fd, buff, MAXSIZE);
	if (nread == -1) {
		perror("read error: ");
		close(fd);
		delete_event(epollfd, fd, EPOLLIN);
	} else if (nread == 0) {
		fprintf(stderr, "client close.\n");
		close(fd);
		delete_event(epollfd, fd, EPOLLIN);
	} else {
		printf("read message is : %s", buff);
		modify_event(epollfd, fd, EPOLLOUT);
	}
}

void do_write(int epollfd, int fd, char* buff) {
	int nwrite = write(fd, buff, strlen(buff));
	if (nwrite == -1) {
		perror("write error: ");
		close(fd);
		delete_event(epollfd, fd, EPOLLOUT);
	} else {
		modify_event(epollfd, fd, EPOLLIN);
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







































