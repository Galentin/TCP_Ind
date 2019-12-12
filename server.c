#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define PORT 57575
#define SIZE_MSG 100
#define SIZE_TEXT 500

struct Client {
	int socket;
    	char* address;
    	int port;
    	int number;
	char login[SIZE_MSG];
	pthread_t threadId;
} *clients;

struct Client_Login {
	char login[SIZE_MSG];
	char password[SIZE_MSG];	
} *clients_login;

struct Message {
	int number;
	int status; 
    	char recipient[SIZE_MSG];
	char sender[SIZE_MSG];
	char topic[SIZE_MSG];
	char text[SIZE_TEXT];
} *messages;

int num_clients = 0;
int num_login = 0;
int num_message = 0;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

void* clientThread(void* args);
void* addClientss(void* args);
void* kickClient(int kickNum);
int readN(int socket, char* buf, int size_msg);

int main(void) {

    	struct sockaddr_in local;
	int sock = -1;
	int bind_sock = -1;
	int lis = -1;
	char buf[SIZE_MSG] = {0};
	
	printf("Start server\n");
	fflush(stdout);
	local.sin_family = AF_INET;
	local.sin_port = htons(PORT);
	local.sin_addr.s_addr = htonl(INADDR_ANY);

	sock = socket(AF_INET, SOCK_STREAM, 0 );
	if (sock == -1) {
		perror("Error. Can't create socket: ");
		exit(1);
	}
	bind_sock = bind(sock, (struct sockaddr *)&local, sizeof(local));
	if (bind_sock == -1) {
		perror("Error. Can't bind socket: ");
		exit(1);
	}
	lis = listen(sock, 4);
	if (lis) {
		perror("Error. Can't listen socket: ");
		exit(1);
	}
	printf("Start listen socket\n");
	fflush(stdout);

	pthread_t listener_th;
    	if (pthread_create(&listener_th, NULL, addClientss, (void*) &sock)){
        	perror("Error. Can't create listener thread: ");
		exit(1);
    	}
    
	printf("\n You can call these commands:\n");
	printf("- view				view clients list \n");
	printf("- kick [number client]		kill the selected client \n");
	printf("- quit				finish\n");
	fflush(stdout);

	for(;;) {
		fgets(buf, SIZE_MSG, stdin);
		buf[strlen(buf) - 1] = '\0';
		if(!strcmp("/quit", buf)) {
			shutdown(sock, 2);
			close(sock);
			pthread_join(listener_th, NULL);
			break;
		} else if(!strcmp("/view", buf)) {
			printf("Clients on-line:\n");
			printf("%10s%20s%10s%20s\n","NUMBER","ADDRESS","PORT","LOGIN");

			pthread_mutex_lock(&mut);
			for(int i = 0; i < num_clients; i++){
				if(clients[i].socket != -1)
					printf("%10d%20s%10d%20s\n", clients[i].number, clients[i].address, clients[i].port, clients[i].login);
			}
			pthread_mutex_unlock(&mut);
			fflush(stdout);
		} else {
			char *separat = " ";
			char *str = strtok(buf, separat);
			if(str == NULL || (strcmp("/kick", str))) {
				printf("Illegal command!\n"); fflush(stdout);
				continue;
			}else{
				str = strtok(NULL, separat);
				int kickNum = atoi(str);
				if(str[0] != '0' && kickNum == 0){
					printf("Illegal format! Write number client!\n");
					fflush(stdout);
					continue;
				}
				kickClient(kickNum);
			}
		}
	}
	printf("End server\n"); 
	fflush(stdout);
	exit(0);
}

void* clientThread(void* args) {
	pthread_mutex_lock(&mut);
	int index = *((int*)args);
	int sock = clients[index].socket;
	pthread_mutex_unlock(&mut);

	char msg[SIZE_MSG] = {0};
	char text_msg[SIZE_TEXT] = {0};
	int login_id = -1;
	
	pthread_mutex_lock(&mut);
	for(;;){
		memset(msg, 0, sizeof(msg));
		login_id = -1;
		readN(sock, msg, SIZE_MSG);
		for(int i = 0; i < num_login; i++){
			if(!strcmp(clients_login[i].login, msg)){
        			login_id = i;
				break;
			}
    		}
		if(login_id == -1) {
			clients_login = (struct Client_Login*) realloc(clients_login, sizeof(struct Client_Login) * (num_login + 1));
			memcpy(clients_login[num_login].login, msg, sizeof(msg));
			memcpy(clients[index].login, msg, sizeof(msg));
			login_id = num_login;
			memset(msg, 0, sizeof(msg));
			readN(sock, msg, SIZE_MSG);
			memcpy(clients_login[num_login].password, msg, sizeof(msg));
			num_login++;
			memset(msg, 0, sizeof(msg));
			msg[0] = '+';
			send(sock, msg, sizeof(msg), 0);
			break;
		}else {
			memset(msg, 0, sizeof(msg));
			readN(sock, msg, SIZE_MSG);
			if(!strcmp(clients_login[login_id].password, msg)){
				memcpy(clients[index].login, clients_login[login_id].login, sizeof(msg));
				memset(msg, 0, sizeof(msg));
				msg[0] = '+';
				send(sock, msg, sizeof(msg), 0);
				break;
			}else {
				memset(msg, 0, sizeof(msg));
				msg[0] = '-';
				send(sock, msg, sizeof(msg), 0);
			}
		}
	}
	pthread_mutex_unlock(&mut);
	memset(msg, 0, sizeof(msg));
	for(;;) {
		if (readN(sock, msg, SIZE_MSG) <= 0) {
			printf("Client №%d disconnect\n", index);
			fflush(stdout);
			close(sock);
			clients[index].socket = -1;
			break;
		}else if(!strcmp("/view", msg)) {
			char info_about_incom_messages[215];
			pthread_mutex_lock(&mut);
			for(int i = 0; i < num_message; i++){
				if(!strcmp(messages[i].recipient, clients_login[login_id].login) && messages[i].status == 0){
					sprintf(info_about_incom_messages, "%d\t\t\t%s\t\t\t%s", messages[i].number, messages[i].sender, messages[i].topic);	       		
					send(sock, info_about_incom_messages, sizeof(info_about_incom_messages), 0);
				}
    			}
			pthread_mutex_unlock(&mut);
			memset(info_about_incom_messages, 0, sizeof(info_about_incom_messages));
			info_about_incom_messages[0] = '-';
			send(sock, info_about_incom_messages, sizeof(info_about_incom_messages), 0);
		}else if(!strcmp("/send", msg)){
			send(sock, msg, sizeof(msg), 0);			
			struct Message message;
			if (readN(sock, &message, sizeof(struct Message)) <= 0) {
				printf("Client №%d disconnect\n", index);
				fflush(stdout);
				close(sock);
				clients[index].socket = -1;
				break;
			}
			pthread_mutex_lock(&mut);
			messages = (struct Message*) realloc(messages, sizeof(struct Message) * (num_message + 1));
			messages[num_message] = message;
			messages[num_message].status = 0;
			messages[num_message].number = num_message;
			num_message++;
			pthread_mutex_unlock(&mut);
			send(sock, msg, sizeof(msg), 0);	
		}else{
			struct Message message_output;
			char *separat = " ";
			char *str = strtok(msg, separat);
			str = strtok(NULL, separat);
			int numb_send_mes = atoi(str);
			pthread_mutex_lock(&mut);
			if (numb_send_mes < 0 || numb_send_mes >= num_message || strcmp(messages[numb_send_mes].recipient, clients_login[login_id].login) || messages[numb_send_mes].status == 1){
				memset(msg, 0, sizeof(msg));
				msg[0] = '-';
				send(sock, msg, sizeof(msg), 0);
			}else{
				memset(msg, 0, sizeof(msg));
				msg[0] = '+';
				send(sock, msg, sizeof(msg), 0);
				message_output = messages[numb_send_mes];
				send(sock, &message_output, sizeof(struct Message), 0);
				messages[numb_send_mes].status = 1;
			}
			pthread_mutex_unlock(&mut);
		}
		memset(text_msg, 0, sizeof(text_msg));
		memset(msg, 0, sizeof(msg));
	}
	printf("End client №%d!\n", index); 
	fflush(stdout);
}

void* addClientss(void* args){
    	int sock = *((int*) args);
    	struct sockaddr_in client_addr;
    	int client_sock;
    	int client_ind;
    	int len_client = sizeof(client_addr);
    	for(;;){

        	client_sock = accept(sock, &client_addr, &len_client);
        	if (client_sock <= 0){
        		printf("STOPING SERVER...\n");
			fflush(stdout);
			pthread_mutex_lock(&mut);
    			for(int i = 0; i < num_clients; i++){
				if(clients[i].socket != -1){
        				shutdown(clients[i].socket, 2);
        				close(clients[i].socket);
        				clients[i].socket = -1;
					pthread_join(clients[i].threadId, NULL);
				}
    			}
    			pthread_mutex_unlock(&mut);
      			break;
        	}
        	pthread_mutex_lock(&mut);
        	clients = (struct Client*) realloc(clients, sizeof(struct Client) * (num_clients + 1));
        	clients[num_clients].socket = client_sock;
        	clients[num_clients].address = inet_ntoa(client_addr.sin_addr);
        	clients[num_clients].port = client_addr.sin_port;
        	clients[num_clients].number = num_clients;
        	client_ind = num_clients;
        	if(pthread_create(&(clients[num_clients].threadId), NULL, clientThread, (void*) &client_ind)) {
            		printf("Can't create thread for client!\n"); 		
			fflush(stdout);
            		continue;
        	}
        	pthread_mutex_unlock(&mut);
        	num_clients++;
    	}
    	printf("Stop listener\n"); 
	fflush(stdout);
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

void* kickClient(int kickNum){
	pthread_mutex_lock(&mut);

	shutdown(clients[kickNum].socket, 2);
	close(clients[kickNum].socket);
	clients[kickNum].socket = -1;

	pthread_mutex_unlock(&mut);
}
