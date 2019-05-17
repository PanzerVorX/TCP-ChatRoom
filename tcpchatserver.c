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

struct threadparameter{//��װ�̺߳����Ĳ���
	char * clientip_pstr;//�ͻ���ip�ĵ��ʮ��ʽ
	int connfd;//���Ӧ�ͻ���ͨ�ŵ������׽���
};

struct client{//�ͻ��˼���
    int clientArr[MAXSIZE];//�����׽�������
    int count;//��ǰ�ͻ�����
}clientSet;

int isrecvparameter=0;

void main(){
    int sockfd,connfd;
    struct sockaddr_in serveraddr,clientaddr;
    int addrlen;
    struct threadparameter thread_parameter;
    pthread_t communicationthread;//�洢�߳�id
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

    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(char *)&val,sizeof(val));//�˿ڿ�����
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
    isrecvparameter=1;//ȷ���ѽ��ղ���

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
    sendMessage(connfd,sendbuf);//ת��������Ϣ
    memset(sendbuf,0,MAXSIZE);

    while((num=recv(connfd,recvbuf,MAXSIZE-1,0))){

        recvbuf[num]='\0';
        printf("%s : %s\n",clientname,recvbuf);
        sprintf(sendbuf,"%s : %s",clientname,recvbuf);
        sendMessage(connfd,sendbuf);

        if(strcmp(recvbuf,"bye")==0){
            printf("%s Offline\n",clientname);
            sprintf(sendbuf,"server message : %s Offline",clientname);//ת��������Ϣ
            sendMessage(connfd,sendbuf);
            removeClient(connfd);
            break;
        }
        memset(recvbuf,0,MAXSIZE);
        memset(sendbuf,0,MAXSIZE);
    }
    close(connfd);
}

void sendMessage(int connfd,char * sendbuf){//�������ͻ���ת����Ϣ
    int i;
    for(i=0;i<clientSet.count;i++){
        if(clientSet.clientArr[i]!=connfd){
            send(clientSet.clientArr[i],sendbuf,strlen(sendbuf),0);
        }
    }
}

void removeClient(int connfd){//�Ƴ����߿ͻ���
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
