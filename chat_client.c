#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
int sockfd = 0;
int send_file_switch = 0;
int check_switch = 0;
int recv_file_switch = 0;
int check_send = 0;

void *recv_msg(void *arg){

    char buffer[256];
	char msg_record[25];
	sprintf(msg_record,"--Do you want to receive");
    while(1)
    {
		bzero(buffer,256);
        int ret = recv(sockfd,buffer,256,0);
        if(0 > ret){	
        	perror("recv");
            pthread_exit(0);
        }

		if(check_switch != 0){
			if( strcmp(buffer, "--No this user") == 0 ){
				send_file_switch = -1;
				check_switch = 0;
			}
			else if( strcmp(buffer, "--check the user") == 0 ){
				send_file_switch = 1;
				check_switch = 0;
			}
		}
		else if( strncmp(buffer, msg_record, strlen(msg_record)-1) == 0 ){
			
			check_send = 1;
			puts(buffer);
			while(1){
				if(recv_file_switch != 0)
					break;
			}

			if(recv_file_switch == -1){
				recv_file_switch = 0;
				continue;
			}
			else if(recv_file_switch == 1)
				recv_file_switch = 0;

			char *file_path;
			file_path = strtok(buffer, " ");
			for(int i=0; i<5; i++){
				file_path = strtok(NULL," ");
			}


			FILE *fp = fopen(file_path,"w");
			if (fp == NULL){
            	printf("File:\t%s Open Fail!\n",file_path);
				continue;
			}

			bzero(buffer, 256);
			int len,write_len;
			while( (len = recv(sockfd,buffer,256,0)) > 0){
				
				if( strncmp(buffer, "/endll", 6) == 0 )
					break;

				write_len = fwrite(buffer,sizeof(char), len,fp);
				if(write_len < 0){
					printf("write fail\n");
					exit(1);
				}
				bzero(buffer,256);
			}
			printf("file recv complete\n");
			fclose(fp);
			continue;
		}

		
		puts(buffer);
    }
}


int main(){
	
 	sockfd = socket(AF_INET , SOCK_STREAM , 0);

	if (sockfd == -1){
		printf("Fail to create a socket.");
	}

	struct sockaddr_in info;
	bzero(&info,sizeof(info));
	info.sin_family = AF_INET;

	info.sin_addr.s_addr = inet_addr("127.0.0.1");
    	info.sin_port = htons(8202);

	
	int ret = connect(sockfd,(struct sockaddr *)&info,sizeof(info));
	if(ret==-1){
		printf("Connection error");
		return 0;
    }

	char user_name[20] = {};

	printf("Input your name:");
	scanf("%s", user_name);
	send(sockfd, user_name, sizeof(user_name), 0);

	pthread_t pid;
	ret = pthread_create(&pid,NULL,recv_msg,NULL);
	if(0 > ret){
    	perror("connect");
        return -1;
    }

	
	char buffer[256] = {};
	for( ; ; ){
		bzero(buffer,256);
		fgets(buffer,256,stdin);
		buffer[strlen(buffer)-1] = '\0';
		if(buffer[0] == '\n' || buffer[0] == ' ' || buffer[0] == '\t' ||  buffer[0] == '\0')
			continue;
		else if(check_send != 0){
			if( strncmp(buffer, "--y", 3) == 0 ){
				recv_file_switch = 1;
				check_send = 0;
			}
			else if( strncmp(buffer,"--n", 3) == 0 ){
				recv_file_switch = -1;
				check_send = 0;
			}
		}
		else if( strncmp(buffer,"-s",2) == 0){
			char *file_path;
			char copy_buffer[256];
			strcpy(copy_buffer, buffer);

			file_path = strtok(copy_buffer, " ");
			file_path = strtok(NULL, " ");
			file_path = strtok(NULL, " ");
			
			FILE *fp = fopen(file_path,"r");
			if (fp == NULL){
            	printf("File:\t%s Not Found!\n",file_path);
				continue;
			}

			send(sockfd, buffer, 256,0);
			check_switch = 1;
			while(1){
				if(send_file_switch != 0)
					break;
			}
			
			if(send_file_switch == -1){
				send_file_switch = 0;
				continue;
			}
			else
				send_file_switch = 0;

			int file_len, send_len;
			bzero(buffer,256);
			while( !feof(fp) ){
				file_len = fread(buffer,sizeof(char),sizeof(buffer),fp);
				//printf("have send %d byte\n",file_len);
				if(send(sockfd,buffer,file_len,0) < 0)
					printf("send fail\n");
				//send_len = write(sockfd, buffer, file_len);

				bzero(buffer,256);
			}
			

			fclose(fp);
        	printf("File: Transfer Finished!\n");
			printf("輸入任意健退出\n");
			getchar();
			sprintf(buffer,"/endll");
			send(sockfd,buffer,strlen(buffer),0);
			
			continue;
		}
 
		ret = send(sockfd, buffer, 256,0);
        
		if(0 > ret){
            perror("send");
            return -1;
        }

		if(strcmp("q!",buffer) == 0){
			printf("exit chat\n");
			return 0;
		}
	}
}
