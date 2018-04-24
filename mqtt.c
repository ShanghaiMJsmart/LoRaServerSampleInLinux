#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mosquitto.h>
#include <string.h>
#include <json-c/json.h>
#include <errno.h>

#define HOST "101.132.97.241"
#define PORT  1883

#define KEEP_ALIVE 60
#define MSG_MAX_SIZE  512
#define SOCKET_RCV_BUFFER_SIZE	(1024 * 5)
#define MAX_NODE_NUM	6
#define MAX_RADIO_PKG_SIZE	256
typedef struct{
    uint8_t strDevEUI[8 * 2 + 1];
    uint8_t strmacaddr[6 * 2 + 1];
    uint32_t iDevAddr;
    bool isClassC;    
}st_ServerNodeDatabase,*pst_ServerNodeDatabase;

st_ServerNodeDatabase stServerNodeDatabase;
bool session = true;

void my_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	int len;
	struct json_object *pragma = NULL;
    struct json_object *obj = NULL;
	uint8_t *pstrchr;
	uint32_t sendlen;
	uint8_t stringformat[256 * 2];
	uint8_t datatosend[1024];
	uint8_t echodata[MAX_RADIO_PKG_SIZE * 2];
    uint8_t iPort = 0;
    bool isconfirmrequest = false;
    uint8_t strgatewaymacaddr[6 * 2 + 1] = {0};
    if(message->payloadlen > 0)
	{
	    printf("%s %s\r\n", message->topic, message->payload);
        pstrchr = message->payload;
        if(message->payload)
        {
            pragma = json_tokener_parse((const char *)pstrchr);
            if(pragma == NULL)
            {
                return;
            }

            json_object_object_get_ex(pragma,"FrameType",&obj);
            if(obj == NULL)
            {
				printf("Format eerror %d\r\n",__LINE__);
                json_object_put(pragma);
                return;
            }
            if(strcmp((const char*)json_object_get_string(obj),"UpData") == 0)
            {
				json_object_object_get_ex(pragma,"NetAddr",&obj);
            	if(obj == NULL)
            	{
                    printf("Format eerror %d\r\n",__LINE__);
                    json_object_put(pragma);
                    return;
            	}
                stServerNodeDatabase.iDevAddr = json_object_get_int(obj);

                
                json_object_object_get_ex(pragma,"Port",&obj);
                if(obj == NULL)
                {
                    printf("Format eerror %d\r\n",__LINE__);
                    json_object_put(pragma);
                    return;
                }
                iPort = json_object_get_int(obj);
                                
                json_object_object_get_ex(pragma,"DevEUI",&obj);
                if(obj == NULL)
                {
                    printf("Format eerror %d\r\n",__LINE__);
                    json_object_put(pragma);
                    return;
                }
                strcpy((char *)stServerNodeDatabase.strDevEUI,(const char*)json_object_get_string(obj));
                memcpy(stServerNodeDatabase.strmacaddr,message->topic + strlen("LoRaWAN/Up/"),6 * 2);
                json_object_object_get_ex(pragma,"NodeType",&obj);
                if(obj == NULL)
                {
                    printf("Format eerror %d\r\n",__LINE__);
                    json_object_put(pragma);
                    return;
                }
                if(strcmp("Class C",json_object_get_string(obj)))
                {
                    stServerNodeDatabase.isClassC = true;
                }
                else
                {
                    stServerNodeDatabase.isClassC = false;
                }
                json_object_object_get_ex(pragma,"ConfirmRequest",&obj);
	        	if(obj == NULL)
                {
                    printf("Format eerror %d\r\n",__LINE__);
                    json_object_put(pragma);
                    return;
                }
                isconfirmrequest = json_object_get_boolean(obj);
				
				json_object_object_get_ex(pragma,"Data",&obj);
				if(obj == NULL)
				{
					printf("Format eerror %d\r\n",__LINE__);
					json_object_put(pragma);
					return;
				}
				memset(echodata,0,sizeof(echodata));
				strcpy((char *)echodata,(const char*)json_object_get_string(obj));
                json_object_put(pragma);
				//for(int loop = 0;loop < MAX_NODE_NUM ;loop ++)
				{
                	pragma = json_object_new_object();
	                json_object_object_add(pragma,"FrameType",json_object_new_string("DownData"));
	                json_object_object_add(pragma,"NetAddr",json_object_new_int(stServerNodeDatabase.iDevAddr));
	                json_object_object_add(pragma,"Port",json_object_new_int(iPort));
	                json_object_object_add(pragma,"ConfirmRequest",json_object_new_boolean(1));
					json_object_object_add(pragma,"Confirm",json_object_new_boolean(isconfirmrequest));
                    if(iPort == 1)
                    {
                        if(strlen(echodata) == 2)
                        {
                            strcmp("00",echodata)?strcpy(echodata,"00"):strcpy(echodata,"01");
                            json_object_object_add(pragma,"Data",json_object_new_string(echodata));
                        }
                        else
                        {
                            json_object_object_add(pragma,"Data",json_object_new_string(""));
                        }
                    }
                    else
                    {
                        json_object_object_add(pragma,"Data",json_object_new_string(echodata));
                    }
	                
                    unsigned char topic[8 + 1 + 6 * 2 + 1 + 8 * 2 + 1 + 2 + 1 + 10] = {0};
					memset(datatosend,0,sizeof(datatosend));
                    memset(topic,0,sizeof(topic));
					strcpy(datatosend,json_object_to_json_string(pragma));
	                sendlen = strlen(datatosend);
					/*发布消息*/
					//sprintf(topic,"%s,%s,%s,%s","LoRaWAN/",strmacaddr,"/","0123456789ABCDEF");
					strcpy(topic,"LoRaWAN/Down/");
                    printf("gateway = %s\r\n",stServerNodeDatabase.strmacaddr);
					strcat(topic,stServerNodeDatabase.strmacaddr);
					strcat(topic,"/");
					strcat(topic,stServerNodeDatabase.strDevEUI);
					printf("topic = %s\r\n",topic);
                    printf("data = %s\r\n",datatosend);
                    mosquitto_publish(mosq,NULL,topic,strlen(datatosend)+1,datatosend,0,0);
					//mosquitto_publish(mosq,NULL,topic,strlen(datatosend)+1,sendlen,0,0);
	                json_object_put(pragma);
				}
            }
            else
            {
                printf("Format eerror %d\r\n",__LINE__);
                json_object_put(pragma);
            }
            //memset(buffer,0,1024 * 100);
        }
    }else{
        printf("%s (null)\n", message->topic);
    }
    fflush(stdout);
}

void my_connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
    int i;
    unsigned char topic[8 + 1 + 6 * 2 + 2 + 1] = {0};
    if(!result){
        /* Subscribe to broker information topics on successful connect. */
        strcpy(topic,"LoRaWAN/Up");
        strcat(topic,"/#");
        mosquitto_subscribe(mosq, NULL, topic, 2);
        printf("topic = %s\r\n",topic);
    }else{
        fprintf(stderr, "Connect failed\n");
    }
}
void my_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
    int i;
    printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
    for(i=1; i<qos_count; i++){
        printf(", %d", granted_qos[i]);
    }
    printf("\n");
}

void my_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
    /* Pring all log messages regardless of level. */
    printf("%s\n", str);
}

int main(void )
{
	uint8_t stringformat[256 * 2];
	int len;
    struct json_object *pragma = NULL;
    struct mosquitto *mosq = NULL;
    int err,i;
	uint8_t deveui[8 * 2 + 1] = {0};
	uint8_t senddata[1024] = {0};
    memset(&stServerNodeDatabase,0,sizeof(stServerNodeDatabase));
	init_mqtt:
	//libmosquitto 库初始化
	mosquitto_lib_init();
	//创建mosquitto客户端
	mosq = mosquitto_new(NULL,session,NULL);
	if(!mosq){
		printf("create client failed..\n");
		mosquitto_lib_cleanup();
		sleep(1);
		goto init_mqtt;
		//return 0;
	}
	//设置回调函数，需要时可使用
	//mosquitto_log_callback_set(mosq, my_log_callback);
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_message_callback_set(mosq, my_message_callback);
	//mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);
	mosquitto_username_pw_set(mosq,"MJ-Modem","www.colt.xin");
	//printf("%s, %d\r\n",__func__,__LINE__);
	//连接服务器
	if(mosquitto_connect(mosq, HOST, PORT, KEEP_ALIVE)){
		fprintf(stderr, "Unable to connect.\n");
		mosquitto_destroy(mosq);
        	mosquitto_lib_cleanup();
        	sleep(1);
        	goto init_mqtt;
		//return 0;
	}
	
	//循环处理网络消息
	mosquitto_loop_forever(mosq, -1, 1);
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        sleep(1);
        goto init_mqtt;
	return 0;
}


