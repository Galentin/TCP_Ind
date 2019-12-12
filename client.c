#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define SIZE_MSG 100
#define SIZE_TEXT 500

int readN(int socket, char* buf, int size_msg);

struct Message {
	int number;
	int status;
    	char recipient[SIZE_MSG];
	char sender[SIZE_MSG];
	char topic[SIZE_MSG];
	char text[SIZE_TEXT];
} *messages;

int main(void) {

	char login[SIZE_MSG] = {0};

	char buf[SIZE_MSG] = {0};
	int sock = -1;
	int conn = -1;

	struct sockaddr_in peer;
	peer.sin_family = AF_INET;

	printf("Enter IP address\n"); 
	fflush(stdout);
	fgets(buf, SIZE_MSG, stdin);
	buf[strlen(buf) - 1] = '\0';
	peer.sin_addr.s_addr = inet_addr(buf);

	printf("Enter number port\n"); 
	fflush(stdout);
	memset(buf, 0, sizeof(buf));
	fgets(buf, SIZE_MSG, stdin);
	buf[strlen(buf) - 1] = '\0';
	peer.sin_port = htons(atoi(buf));

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1) {
		perror("Error. Can't create socket");
		exit(1);
	}
	conn = connect(sock, (struct sockaddr*)&peer, sizeof(peer));
	if(conn == -1) {
		perror("Error. Can't create connection");
		close(sock);
		exit(1);
	}
	printf("\nConnected.\n"); 
	fflush(stdout);

	char msg[SIZE_MSG] = {0};
	int s = 0;
	
	for(;;){
		printf("\nEnter your login:	"); 
		fflush(stdout);
		fgets(login, SIZE_MSG, stdin);
		login[strlen(login) - 1] = '\0';
		s = send(sock, login, sizeof(login), 0);

		printf("Enter your password:	"); 
		fflush(stdout);
		memset(buf, 0, sizeof(buf));
		fgets(buf, SIZE_MSG, stdin);
		buf[strlen(buf) - 1] = '\0';
		s = send(sock, buf, sizeof(buf), 0);
		
		memset(buf, 0, sizeof(buf));
		readN(sock, buf, SIZE_MSG);
		if('-'== buf[0]){
			printf("Bad. Try again\n"); 
			fflush(stdout);
			continue;
		}else {
			printf("Good\n"); 
			fflush(stdout);
			break;
		}
	}
	
	printf("\n You can call these commands:\n");
	printf("- view				View incoming messages \n");
	printf("- get [number]			Get message \n");
	printf("- send				Send message \n");
	printf("- quit				Enter, if you want to exit\n");
	fflush(stdout);
	int quit = 1;

	for(;;){
		memset(buf, 0, sizeof(buf));
		fgets(buf, SIZE_MSG, stdin);
		buf[strlen(buf) - 1] = '\0';
		if(!strcmp("/quit", buf)) {
			close(sock);
			break;
		} else if(!strcmp("/view", buf)) {
			s = send(sock, buf, sizeof(buf), 0);
			if(s == -1){
				printf("Can't send message \n");
				fflush(stdout);
				close(sock);
				break;
			} 
			char list_message[215] = {0};
			printf("%s\t\t\t%s\t\t\t%s\n","№", "FROM","TOPIC");
			for(;;){
				if(readN(sock, list_message, sizeof(list_message)) == -1){
					quit = 0;
					break;
				} 
				if('-'== list_message[0]){
					break;
				}	
				printf("%s\n", list_message); 
				fflush(stdout);	
				memset(list_message, 0, sizeof(list_message));	
			}
			if(quit == 0){
				printf("Disconnect\n");
				fflush(stdout);
				close(sock);
				break;
			}
		} else if(!strcmp("/send", buf)) {
			s = send(sock, buf, sizeof(buf), 0);
			if(s == -1){
				printf("Can't send message \n");
				fflush(stdout);
				close(sock);
				break;
			}
			memset(msg, 0, sizeof(msg));
			if(readN(sock, msg, SIZE_MSG) == -1){
				printf("Disconnect\n");
				fflush(stdout);
				close(sock);
				break;
			} 
			struct Message message;
			memcpy(message.sender, login, sizeof(login));
			printf("\nFor:	");
			fflush(stdout);
			fgets(message.recipient, SIZE_MSG, stdin);
			message.recipient[strlen(message.recipient) - 1] = '\0';
			printf("Topic:	");
			fflush(stdout);
			fgets(message.topic, SIZE_MSG, stdin);
			message.topic[strlen(message.topic) - 1] = '\0';
			printf("Text:	");
			fflush(stdout);
			fgets(message.text, SIZE_TEXT, stdin);
			message.text[strlen(message.text) - 1] = '\0';
			s = send(sock, &message, sizeof(struct Message), 0);
			if(s == -1){
				printf("Can't send message \n");
				fflush(stdout);
				close(sock);
				break;
			}
			memset(msg, 0, sizeof(msg));
			if(readN(sock, msg, SIZE_MSG) == -1){
				printf("Disconnect\n");
				fflush(stdout);
				close(sock);
				break;
			} 
			printf("\nSuccesfull.\n");
			fflush(stdout);
		}else {
			memcpy(msg, buf, sizeof(msg));			
			char *separat = " ";
			char *str = strtok(buf, separat);
			if(str == NULL || (strcmp("/get", str))) {
				printf("Illegal command!\n"); 
				fflush(stdout);
				continue;
			}else{
				str = strtok(NULL, separat);
				int messageNumb = atoi(str);
				if(str[0] != '0' && messageNumb == 0){
					printf("Illegal format! Write number message again!\n");
					fflush(stdout);
					continue;
				}
				s = send(sock, msg, sizeof(msg), 0);
				memset(msg, 0, sizeof(msg));
				if(readN(sock, msg, SIZE_MSG) == -1){
					printf("Disconnect\n");
					fflush(stdout);
					close(sock);
					break;
				} 
				if('-'== msg[0]){
					printf("Illegal format! Write number message again!\n");
					fflush(stdout);
					continue;
				}else{
					struct Message message_input;
					readN(sock, &message_input, sizeof(struct Message));
					printf("From:	%s\n", message_input.sender);
					fflush(stdout);
					printf("Topic:	%s\n", message_input.topic);
					fflush(stdout);
					printf("Text:	%s\n", message_input.text);
					fflush(stdout);
				}
			}
		}
	}
	printf("Exit.\n"); 
	fflush(stdout);	
	exit(0);
}

int readN(int socket, char* buf, int size_msg){
	int result = 0;
	int readBytes = 0;
	int size = size_msg;
	while(size > 0){
		readBytes = recv(socket, buf + result, size, 0);
		if (readBytes <= 0){
			return -1;
		}
		result += readBytes;
		size -= readBytes;
	}
	return result;
}

