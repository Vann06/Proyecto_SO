 #ifndef USER_H
 #define USER_H

 #include <arpa/inet.h>
 #include <time.h>

 #include "common.h"

 typedef struct {
	 char username[MAX_NAME];
	 char ip[INET_ADDRSTRLEN];
	 char status[MAX_STATUS];
	 int socket_fd;
	 time_t last_activity;
 } User;

 typedef struct UserNode {
	 User user;
	 struct UserNode *next;
 } UserNode;

 #endif
