//
//  main.c
//  processDemo
//
//  Created by Apus on 2018/11/20.
//  Copyright © 2018年 kangyf. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define IP      "127.0.0.1"
#define PORT    8888
#define WORKER  4
int worker(int listenfd, int i);

int main(int argc, const char * argv[]) {
    //lsof -itcp:8888 | grep LISTEN | awk '{print $2}' |xargs kill -9
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    inet_pton(AF_INET,IP, &address.sin_addr);
    address.sin_port = htons(PORT);
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    
    //绑定8888监听端口
    int ret = bind(listenfd, (struct sockaddr*) &address, sizeof(address));
    if(ret == -1){
        //拼接结束旧监听进程shell命令
        char command1[sizeof("lsof -itcp:")] = "lsof -itcp:";
        char command2[sizeof(" | grep LISTEN | awk '{print $2}' |xargs kill -9")] = " | grep LISTEN | awk '{print $2}' |xargs kill -9";
        //拼接命令 "lsof -itcp:8888 | grep LISTEN | awk '{print $2}' |xargs kill -9"
        char commandBuf[sizeof(command1)+sizeof(int)+sizeof(command2)] ;
        sprintf(commandBuf, command1, PORT, command2);
        //执行结束8888端口监听进程shell命令
        int callstatus = system("lsof -itcp:8888 | grep LISTEN | awk '{print $2}' |xargs kill -9");
        if(callstatus==0){
            //重新绑定8888端口
             ret = bind(listenfd, (struct sockaddr*) &address, sizeof(address));
        }
    }

    assert(ret != -1);
    ret = listen(listenfd, 5);
    assert(ret != -1);
    
    int pids[WORKER];           //定义子进程ID数组
    
    int fd[2];                  //定义读写管道
    int *write_fd = &fd[1];     //写管道
    int *read_fd = &fd[0];      //读管道
    int result = -1;
    result = pipe(fd);          //创建读写管道（父进程与子进程通信，只能通过管道方式）
    if(-1 == result){
        printf("fail to create pipe \n");
        return -1;
    }

    for(int i=0; i < WORKER; i++) {
        printf("Create worker %d\n", i+1);
        //创建子进程
        pid_t pid = fork();
        if(pid ==0){
            int childPid = getpid();            //取得子进程PID。
            char buf[sizeof(int)*8];            //定义一个char类型buffer,存放子进程号。
            close(*read_fd);                    //由于不需要定义读操作，因此需要关闭读管道。
            sprintf(buf,"%d",childPid);         //int类型进程号，转换为char类型字符保存。
            write(*write_fd, buf, sizeof(buf)); //子进程通过写管道，传递进程号给父进程。
            close(*write_fd);                   //关闭写管道。
            worker(listenfd,i);                 //启动8888端口监听
        }
        
        if(pid < 0) {
            printf("fork error");
        }
    }
    
    close(*write_fd);                           //父进程关闭写管道操作，因为只需要接收子进程发送信息。
    char readBuf[sizeof(int)*8];
    //取出所有子进程PID
    for(int i=0; i < sizeof(pids)/sizeof(int) ; i++) {
        //printf("进程%d", pids[i]);
        read(*read_fd,readBuf,sizeof(readBuf));
        pids[i] = atoi(readBuf);
        memset(readBuf, 0 , sizeof(readBuf));   //清空缓存区
        printf("%d \n", pids[i]);
    }
    close(*read_fd);                            //关闭读操作

    int status;
    wait(&status);

    return 0;
}


int worker(int listenfd, int i)
{
    
    while (1) {
        printf("I am worker %d, begin to accept connection.\n", i);
        struct sockaddr_in client_addr;
        socklen_t client_addrlen = sizeof(client_addr);
        int connfd = accept(listenfd, (struct sockaddr*) &client_addr, &client_addrlen);
        if(connfd != -1) {
            printf("worker %d accept a connection success.\t",i);
            printf("ip: %s\t", inet_ntoa(client_addr.sin_addr));
            printf("port: %d\n", client_addr.sin_port);
        } else {
            printf("worker %d accept a connection failed, error:%s", i, strerror(errno));
            close(connfd);
        }
        
    }// end while
}
