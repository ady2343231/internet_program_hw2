#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

int connfd[10];
int count = 0;
short send_file_switch[10];
short check_switch[10];
char user_name[10][20];

void *handle_client(void *serve_index){
	
	int index = *(int *)serve_index;
	char recv_buffer[256],send_buffer[256];

	int ret = recv(connfd[index], user_name[index], sizeof(user_name[0]), 0);
	if(ret < 0){
		perror("recv");
		close(connfd[index]);
		return(NULL);
	}

	for( ; ; ){
		bzero(recv_buffer, 256);
		bzero(send_buffer,256);

		ret = recv(connfd[index], recv_buffer, 256, 0);

		//////////////////離開聊天室////////////////////
		if( strcmp("q!", recv_buffer) == 0 ){

			sprintf(send_buffer, "%s off line\n", user_name[index]);

			for(int i=0; i <= count; i++){
				if(i == index || connfd[i] == 0)
					continue;

				send(connfd[i], send_buffer, 256, 0);
			}
			connfd[index]=-1;
			pthread_exit(0);
		}
		///////////////////查看有多少使用者/////////////////////////
		else if( strcmp("-online", recv_buffer) == 0 ){
			
			for(int i=0; i<10; i++){

				if(connfd[i] > 0){
					sprintf(send_buffer, "user%d:%s\n", i, user_name[i]);
					send(connfd[index], send_buffer, 256, 0);
				}
			}
			continue;
		}
		////////////私訊給某人////////////////
		else if( strncmp(recv_buffer, "-dm", 3) == 0){
			int number = atoi(&recv_buffer[4]);			
			if(connfd[number]==-1){
				send(connfd[index],"No this user", 13, 0);
				continue;
			}

			send(connfd[index],"Direct message mode", 19, 0);
			for(; ;){
				ret = recv(connfd[index], recv_buffer, 256, 0);
				if( strcmp("q!", recv_buffer) == 0 ){
						sprintf(send_buffer, "%s off line\n", user_name[index]);
						send(connfd[number], send_buffer, 256, 0);
						connfd[index]=-1;
						pthread_exit(0);
				}
				else if( strcmp("-q", recv_buffer) == 0 ){
					sprintf(send_buffer,"Quit direct message mode");
					send(connfd[index], send_buffer, strlen(send_buffer), 0);
					bzero(send_buffer,256);
					break;
				}
					
				sprintf(send_buffer,"%s >> %s", user_name[index], recv_buffer);
				send(connfd[number], send_buffer, 256, 0);
			}
			continue;
		}
		else if( strncmp(recv_buffer, "-s", 2) == 0){
			
			char *token;
			char file_path[50];
			token = strtok(recv_buffer, " "); //過濾掉-s
			
			token = strtok(NULL, " "); // number
			int number = atoi(token);
			if(connfd[number]==-1){
				sprintf(send_buffer,"--No this user");
				send(connfd[index], send_buffer, strlen(send_buffer), 0);
				continue;
			}
			else{
				sprintf(send_buffer,"--check the user");
				send(connfd[index], send_buffer, strlen(send_buffer), 0);
			}
			
			token = strtok(NULL, " "); // 檔案路徑
			strcpy(file_path, token); // 避免檔案路徑被洗掉


			FILE *fp = fopen(file_path,"w");
			if (fp == NULL){
            	printf("File:\t%s Open Fail!\n",file_path);
				continue;
			}

			/////////////////////從client接收檔案//////////////
			bzero(recv_buffer, 256);
			int len,write_len;
			while( (len = recv(connfd[index],recv_buffer,256,0)) > 0){
				
				if( strncmp(recv_buffer, "/endll", 6) == 0 )
					break;

				write_len = fwrite(recv_buffer,sizeof(char), len,fp);
				if(write_len < 0){
					printf("write fail\n");
					exit(1);
				}
				bzero(recv_buffer,256);
			}
			
			printf("recv complete\n");
			fclose(fp);

			//////////////////////開始傳給目標//////////////////////
			
			sprintf(send_buffer,"--Do you want to receive %s from %s (--y/--n)", file_path, user_name[index]);
			send(connfd[number], send_buffer, strlen(send_buffer), 0);

			check_switch[number] = 1;
			while(1){
				if(send_file_switch[number] != 0)
					break;
			}

			if(send_file_switch[number] == -1){
				send_file_switch[number] = 0;
				continue;
			}
			else
				send_file_switch[number] = 0;


			fp = fopen(file_path,"r");

			int file_len, send_len;
			bzero(send_buffer,256);
			while( !feof(fp) ){
				file_len = fread(send_buffer,sizeof(char),sizeof(send_buffer),fp);
				if(send(connfd[number],send_buffer,file_len,0) < 0)
					printf("send fail\n");

				bzero(send_buffer,256);
			}
			
			fclose(fp);
        	printf("Send File Finished!\n");
			sprintf(send_buffer,"/endll");
			send(connfd[number],send_buffer,strlen(send_buffer),0);
			
			continue;
		}
		else if( check_switch[index] != 0 ){
			if( strcmp(recv_buffer, "--n") == 0 ){
				send_file_switch[index] = -1;
				check_switch[index] = 0;
				continue;
			}
			else if( strcmp(recv_buffer, "--y") ==0 ){
				send_file_switch[index] = 1;
				check_switch[index] = 0;
				continue;
			}
		}


		sprintf(send_buffer,"%s: %s", user_name[index], recv_buffer);
		for(int i=0; i <= count; i++){
			if(i == index || connfd[i] == 0)
				continue;

			send(connfd[i], send_buffer, 256, 0);
		}
	}
}




int main(){

	int listenfd = 0;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (listenfd == -1){
        	printf("Fail to create a socket.");
	}

        struct sockaddr_in servaddr;

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	
	if( inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) <= 0 ){
		printf("addr err");
		return 0;
	}


	servaddr.sin_port = htons(8202);

	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	listen(listenfd, 10);

	struct sockaddr_in	cliaddr[10];
	int  len,index = 0;
	char recvline[256];
	//char str[10];

	for(int i=0; i<10; i++){
		connfd[i]=-1;
	}


	printf("ready\n");
	while(count < 11){
		len = sizeof(cliaddr[0]);
		connfd[count] = accept(listenfd, (struct sockaddr *) &cliaddr[index], &len);

		count++;
		index = count-1;

		pthread_t pid;
		int ret = pthread_create(&pid,NULL,handle_client,&index);

		if(ret < 0){
			perror("pthread_create");
			return -1;
		}
	}
}
