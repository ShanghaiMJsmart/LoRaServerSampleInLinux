#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <json-c/json.h>

#define PORT  32500
#define QUEUE_SIZE   10

void *server_send(void *arg)
{
    int sockfd = *(int*)arg;
    int len;
    
    while(1)
    {
    }
}

void str_echo(int sockfd)
{
    uint8_t buffer[1024 * 100];
    int len;
    int err;
    //uint8_t *tmp;
    //int index;
    pthread_t main_tid;
    struct json_object *pragma = NULL;
    struct json_object *obj = NULL;
    uint8_t stringformat[256 * 2];
    uint8_t tx_data_buf[300];
    uint8_t *pstrchr;
    err = pthread_create(&main_tid, NULL, server_send, &sockfd); //创建线程
    pid_t pid = getpid();
    while(1)
    {
        memset(buffer,0,sizeof(buffer));
        len = recv(sockfd, buffer, sizeof(buffer) - 1,0);
        if(len < 0)
        {
            printf("child process: %d exited.\n",pid);
            break;
        }
        printf("pid:%d %dreceive: %s\r\n",pid,len,buffer);
    }
    close(sockfd);
}

int main(void)
{
    //定义IPV4的TCP连接的套接字描述符
    int server_sockfd = socket(AF_INET,SOCK_STREAM, 0);

    //定义sockaddr_in
    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_sockaddr.sin_port = htons(PORT);
    int opt = 1;
    if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0)
    {
        perror("Server setsockopt failed");
        return 0;
    }
    //bind成功返回0，出错返回-1
    if(bind(server_sockfd,(struct sockaddr *)&server_sockaddr,sizeof(server_sockaddr))==-1)
    {
        perror("bind");
        exit(1);//1为异常退出
    }
    printf("bind success.\n");

    //listen成功返回0，出错返回-1，允许同时帧听的连接数为QUEUE_SIZE
    while(listen(server_sockfd,QUEUE_SIZE) == -1)
    {
        perror("listen");
        sleep(1);//exit(1);
    }
    printf("listen success.\n");

    for(;;)
    {
        struct sockaddr_in client_addr;
        socklen_t length = sizeof(client_addr);
        //进程阻塞在accept上，成功返回非负描述字，出错返回-1
        int conn = accept(server_sockfd, (struct sockaddr*)&client_addr,&length);
        if(conn<0)
        {
            perror("connect");
            continue;//exit(1);
        }
        printf("new client accepted.\n");

        pid_t childid;
        if(childid=fork()==0)//子进程
        {
            printf("child process: %d created.\n", getpid());
            close(server_sockfd);//在子进程中关闭监听
            str_echo(conn);//处理监听的连接
            exit(0);
        }
    }

    printf("closed.\n");
    close(server_sockfd);
    return 0;
}
