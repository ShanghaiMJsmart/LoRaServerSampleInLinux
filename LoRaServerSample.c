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
#include <time.h>

#define PORT  32500
#define QUEUE_SIZE   10
#define SOCKET_RCV_BUFFER_SIZE	1024 * 5
#define MAX_NODE_NUM	6
#define MAX_RADIO_PKG_SIZE	256
typedef struct{
    uint8_t *strDevEUI[8 * 2 + 1];
    uint32_t iDevAddr;
    bool isClassC;    
}st_ServerNodeDatabase,*pst_ServerNodeDatabase;

void Hex2Str( const char *sSrc,  char *sDest, int nSrcLen )
{
    int  i;
    char szTmp[3];

    for( i = 0; i < nSrcLen; i++ )
    {
        sprintf( szTmp, "%02X", (unsigned char) sSrc[i] );
        memcpy( &sDest[i * 2], szTmp, 2 );
    }
    //sDest[i * 2] = '\0';
    return ;
}

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
    uint8_t buffer[SOCKET_RCV_BUFFER_SIZE];
    int len;
    int err;
    pthread_t main_tid;
    struct json_object *pragma = NULL;
    struct json_object *obj = NULL;
    uint8_t *pstrchr;
    uint32_t iDevAddr = 0;
    uint8_t iPort = 0;
    bool isconfirmrequest = false;
    st_ServerNodeDatabase stServerNodeDatabase[MAX_NODE_NUM];
    const uint8_t *sendData = "hello to node";
    uint8_t datatosend[strlen(sendData) * 2 + 1];
	uint8_t echodata[MAX_RADIO_PKG_SIZE * 2];
    Hex2Str( sendData, datatosend, strlen(sendData) );
    datatosend[strlen(sendData) * 2] = '\0';
    memset(stServerNodeDatabase,0,sizeof(st_ServerNodeDatabase));
    srand((int)time(0));
	//err = pthread_create(&main_tid, NULL, server_send, &sockfd);
    pid_t pid = getpid();
    while(1)
    {
        memset(buffer,0,sizeof(buffer));
        len = recv(sockfd, buffer, sizeof(buffer) - 1,0);
        if(len < 1)
        {
            printf("child process: %d exited.\n",pid);
            break;
        }
        printf("pid:%d %dreceive: %s\r\n",pid,len,buffer);
        printf("\r\n");
	pstrchr = buffer;
        while((pstrchr - buffer) < len)
        {
            pstrchr = strchr(pstrchr,'{');
            if(pstrchr == NULL)
            {
                break;
            }
            else
            {
                pstrchr ++;
            }
            pragma = json_tokener_parse((const char *)(pstrchr - 1));
            if(pragma == NULL)
            {
                break;
            }

            json_object_object_get_ex(pragma,"FrameType",&obj);
            if(obj == NULL)
            {
		printf("Format eerror %d\r\n",__LINE__);
                json_object_put(pragma);
                break;
            }
            if(strcmp((const char*)json_object_get_string(obj),"UpData") == 0)
            {
		json_object_object_get_ex(pragma,"NetAddr",&obj);
            	if(obj == NULL)
            	{
                    printf("Format eerror %d\r\n",__LINE__);
                    json_object_put(pragma);
                    break;
            	}
                iDevAddr = json_object_get_int(obj);
		json_object_object_get_ex(pragma,"Port",&obj);
                if(obj == NULL)
                {
                    printf("Format eerror %d\r\n",__LINE__);
                    json_object_put(pragma);
                    break;
                }
                iPort = json_object_get_int(obj);

                json_object_object_get_ex(pragma,"DevEUI",&obj);
                if(obj == NULL)
                {
                    printf("Format eerror %d\r\n",__LINE__);
                    json_object_put(pragma);
                    break;
                }
                strcpy((char *)stServerNodeDatabase[iDevAddr].strDevEUI,(const char*)json_object_get_string(obj));

                json_object_object_get_ex(pragma,"NodeType",&obj);
                if(obj == NULL)
                {
                    printf("Format eerror %d\r\n",__LINE__);
                    json_object_put(pragma);
                    break;
                }
                if(strcmp("Class C",json_object_get_string(obj)))
                {
                    stServerNodeDatabase[iDevAddr].isClassC = true;
                }
                else
                {
                    stServerNodeDatabase[iDevAddr].isClassC = false;
                }
                json_object_object_get_ex(pragma,"ConfirmRequest",&obj);
	        if(obj == NULL)
                {
                    printf("Format eerror %d\r\n",__LINE__);
                    json_object_put(pragma);
                    break;
                }
                isconfirmrequest = json_object_get_boolean(obj);
				
				json_object_object_get_ex(pragma,"Data",&obj);
				if(obj == NULL)
				{
					printf("Format eerror %d\r\n",__LINE__);
					json_object_put(pragma);
					break;
				}
				memset(echodata,0,sizeof(echodata));
				strcpy((char *)echodata,(const char*)json_object_get_string(obj));
                json_object_put(pragma);
				for(int loop = 0;loop < MAX_NODE_NUM;loop ++)
				{
                	pragma = json_object_new_object();
	                json_object_object_add(pragma,"FrameType",json_object_new_string("DownData"));
	                json_object_object_add(pragma,"NetAddr",json_object_new_int(loop));//(iDevAddr));
			printf("iPort = %d\r\n",iPort);
			//if(iPort == 1)
			{
				json_object_object_add(pragma,"Port",json_object_new_int(iPort));
			}
			//else
			{
				//json_object_object_add(pragma,"Port",json_object_new_int((223.0*rand()/(RAND_MAX+1.0))));
			}
	                json_object_object_add(pragma,"ConfirmRequest",json_object_new_boolean((1.999*rand()/(RAND_MAX+1.0))));
					json_object_object_add(pragma,"Confirm",json_object_new_boolean(isconfirmrequest));
			if(iPort == 1)
			{
				if((echodata[0] == '0') && (echodata[1] == '1'))
				{
	                		json_object_object_add(pragma,"Data",json_object_new_string("00"));
				}
				else
				{
					json_object_object_add(pragma,"Data",json_object_new_string("01"));
				}
			}
			else
			{
				json_object_object_add(pragma,"Data",json_object_new_string(echodata));
			}
	                len = strlen(json_object_to_json_string(pragma));
	                len = send(sockfd,json_object_to_json_string(pragma), len, 0);
	                if(len < 1)
	                {
	                    printf("reconnect %d\r\n",__LINE__);
	                    json_object_put(pragma);
	                    pthread_exit(NULL);
	                }
	                printf("%s",json_object_to_json_string(pragma));
	                printf("\r\n");
	                json_object_put(pragma);
				}
            }
            //memset(buffer,0,1024 * 100);
        }
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
