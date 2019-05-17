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
#include <signal.h>
#include <time.h>
#include <pthread.h>

#define PORT 1234
#define MAXSIZE 1024
#define SERVERIP "192.168.1.200"

void process(int);
void * recvNetThread(void *);
void * recvStdinThread(void *);
void writefile(int,char *);
char * getmessage(char *);

//线程终止判断变量
int isRecvNetThreadEnd=0;
int isrecvStdinThreadEnd=0;

struct threadparameter{
    int fd;
    int sockfd;
    char cname[30];
};

int publicsockfd=0;
int filefd;

//客户端异常关闭前应向服务端发送断开连接的信息以防止服务端继续使用失效套接字导致出错（如设置捕获置进程异常终止的信号并在处理函数中发送关闭消息）
void sig_handler(int sig){

	printf("begin signal handler...\n");
	switch(sig){
		case SIGINT:
			printf("SIGINT is caught\n");
            send(publicsockfd,"bye",strlen("bye"),0);
			printf("connection close\n");
            writefile(filefd,"close");
            close(filefd);
            exit(0);
			break;
		default:
			printf("other is not caught\n");
	}
}

int main(){

    int sockfd;
    struct sockaddr_in serveraddr,clientaddr;

    if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1){
        perror("socket");
        exit(0);
    }

    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=PORT;
    serveraddr.sin_addr.s_addr=inet_addr(SERVERIP);
    bzero(&serveraddr.sin_zero,8);

    if(connect(sockfd,(struct sockaddr *)&serveraddr,sizeof(struct sockaddr))==-1){
        perror("connect");
        exit(0);
    }

    publicsockfd=sockfd;

    if(signal(SIGINT,sig_handler)==SIG_ERR){
		printf("signal(SIGINT) error\n");
		exit(1);
	}

    process(sockfd);

    return 0;
}

void writefile(int fd,char * str){
    time_t t1;
    char * t2;
    char buf[2048];
    time(&t1);
    t2=ctime(&t1);
    t2[strlen(t2)-1]='\0';//ctime()返回的时间信息字符串的末尾字符为换行符
    sprintf(buf,"%s\t%s\n",t2,str);
    write(fd,buf,strlen(buf));
}

char * getmessage(char * sendline){//读取输入流
    return fgets(sendline,MAXSIZE-1,stdin);
}

void process(int sockfd){

    char cname[30];
    char sendline[MAXSIZE],recvline[MAXSIZE],tmp[MAXSIZE];
    int fd;
    int num;
    pthread_t communicationthread;
    struct threadparameter thread_parameter;

    printf("connect to server\n");
    printf("input client's name:");
    fgets(cname,30,stdin);
    cname[strlen(cname)-1]='\0';//fgets()从标准输入流获取的数据包括换行符，作为数据发送时应考虑用替换清除
    send(sockfd,cname,strlen(cname),0);
    if((fd=open(cname,O_WRONLY|O_CREAT|O_APPEND,0644))<0){
        perror("open");
    }
    writefile(fd,"connection success");//写入聊天记录
    filefd=fd;

    thread_parameter.sockfd=sockfd;
    thread_parameter.fd=fd;
    strcpy(thread_parameter.cname,cname);

    //可由多线程实现客户端同时接收网络信息与标准输入流
    pthread_create(&communicationthread,NULL,recvNetThread,(void *)&thread_parameter);
    pthread_create(&communicationthread,NULL,recvStdinThread,(void *)&thread_parameter);

    //使用多线程时应保持线程所属进程不提前结束（主线程可通过循环判断子线程的终止判断变量来等待子线程）
    while(!(isRecvNetThreadEnd&&isrecvStdinThreadEnd));
    printf("connection close\n");
    writefile(filefd,"close");
    close(sockfd);
}

void * recvNetThread(void * pthread_parameter){//接受网络信息
    int sockfd=((struct threadparameter*)pthread_parameter)->sockfd;
    int fd=((struct threadparameter*)pthread_parameter)->fd;
    char recvline[MAXSIZE];
    int num;

    while(1){
        if((num=recv(sockfd,recvline,MAXSIZE-1,0))>0){
            recvline[num]=0;
            printf("%s\n",recvline);
            writefile(fd,recvline);
            memset(recvline,0,MAXSIZE);
        }
        else{
            isRecvNetThreadEnd=1;
            break;
        }
    }
}

void * recvStdinThread(void * pthread_parameter){//接受标准输入流
    int sockfd=((struct threadparameter*)pthread_parameter)->sockfd;
    int fd=((struct threadparameter*)pthread_parameter)->fd;
    char cname[30];
    strcpy(cname,((struct threadparameter*)pthread_parameter)->cname);
    char sendline[MAXSIZE];
    char tempbuf[MAXSIZE];

    while(1){

        getmessage(sendline);
        sendline[strlen(sendline)-1]='\0';
        send(sockfd,sendline,strlen(sendline),0);
        sprintf(tempbuf,"%s : %s",cname,sendline);
        writefile(fd,tempbuf);

        if(strcmp(sendline,"bye")==0){
            break;
        }
        memset(sendline,0,MAXSIZE);
        memset(tempbuf,0,MAXSIZE);
    }
    isrecvStdinThreadEnd=1;
}
