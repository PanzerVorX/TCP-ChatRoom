#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <errno.h>
#include <pthread.h>

#define PORT 1234
#define CONNECTNUM 5
#define SERVERIP "192.168.1.200"
#define MAXSIZE 1024

void * communication(void *);
void sendMessage(int ,char *);
void removeClient(int);

struct threadparameter{//封装线程函数的参数
	char * clientip_pstr;//客户端ip的点分十形式
	int connfd;//与对应客户端通信的连接套接字
};

struct client{//客户端集合
    int clientArr[MAXSIZE];//连接套接字数组
    int count;//当前客户端数
}clientSet;

int isrecvparameter=0;

void main(){
    int sockfd,connfd;
    struct sockaddr_in serveraddr,clientaddr;
    int addrlen;
    struct threadparameter thread_parameter;
    pthread_t communicationthread;//存储线程id
    int val;
    clientSet.count=0;

    if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1){
        perror("socket");
        exit(0);
    }

    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=PORT;
    serveraddr.sin_addr.s_addr=inet_addr(SERVERIP);
    bzero(&serveraddr.sin_zero,8);

    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(char *)&val,sizeof(val));//端口可重用
    bind(sockfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr));
    listen(sockfd,CONNECTNUM);

    while(1){
        if((connfd=accept(sockfd,(struct sockaddr *)&clientaddr,&addrlen))==-1){
            perror("accept");
        }
        thread_parameter.clientip_pstr=inet_ntoa(clientaddr.sin_addr);
        thread_parameter.connfd=connfd;
        pthread_create(&communicationthread,NULL,communication,(void *)&thread_parameter);
        while(isrecvparameter==0);
        isrecvparameter=1;
    }
}

void * communication(void * pthread_parameter){

    char * clientip_pstr=((struct threadparameter *)pthread_parameter)->clientip_pstr;
    int connfd=((struct threadparameter *)pthread_parameter)->connfd;
    isrecvparameter=1;//确认已接收参数

    clientSet.clientArr[clientSet.count]=connfd;
    clientSet.count++;

    int num;
    char clientname[30];
    char recvbuf[MAXSIZE],sendbuf[MAXSIZE];

    printf("get a connection from %s\n",clientip_pstr);
    num=recv(connfd,clientname,MAXSIZE-1,0);
    clientname[num]='\0';
    printf("%s client's name is %s\n",clientip_pstr,clientname);

    sprintf(sendbuf,"server message : %s online",clientname);
    sendMessage(connfd,sendbuf);//转发上线消息
    memset(sendbuf,0,MAXSIZE);

    while((num=recv(connfd,recvbuf,MAXSIZE-1,0))){

        recvbuf[num]='\0';
        printf("%s : %s\n",clientname,recvbuf);
        sprintf(sendbuf,"%s : %s",clientname,recvbuf);
        sendMessage(connfd,sendbuf);

        if(strcmp(recvbuf,"bye")==0){
            printf("%s Offline\n",clientname);
            sprintf(sendbuf,"server message : %s Offline",clientname);//转发离线消息
            sendMessage(connfd,sendbuf);
            removeClient(connfd);
            break;
        }
        memset(recvbuf,0,MAXSIZE);
        memset(sendbuf,0,MAXSIZE);
    }
    close(connfd);
}

void sendMessage(int connfd,char * sendbuf){//向其它客户端转发消息
    int i;
    for(i=0;i<clientSet.count;i++){
        if(clientSet.clientArr[i]!=connfd){
            send(clientSet.clientArr[i],sendbuf,strlen(sendbuf),0);
        }
    }
}

void removeClient(int connfd){//移除断线客户端
    int isThroughPosition=0;
    int i=0;
    for(i=0;i<clientSet.count;i++){
        if(clientSet.clientArr[i]==connfd){
            isThroughPosition=1;
        }
        else if(isThroughPosition){
            clientSet.clientArr[i-1]=clientSet.clientArr[i];
        }
    }
    clientSet.count--;
}
