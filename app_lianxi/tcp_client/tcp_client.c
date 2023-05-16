#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <wchar.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/time.h>
#include <sys/stat.h>


#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib,"ws2_32.lib")


#define portnumber 5555 //550
#define server_ip "127.0.0.1"

int check_buf_data(char *buf,int data_len)
{
    int ret=0;
    int i=0;
    for(i=0;i<data_len;i++)
    {
        if(buf[i]!=i%99)
        {
            printf("data did not match\n");
            return -1;
        }
    }
    return 0;
}
#define MSG_LEN (1*1024)
char buf[12000*1024];


#define BUFFER_SIZE    2048
#define PACK_NAME_SIZE    256
struct FilePackage {
    char cmd;                   /* 操作命令 */
    int filesize;                /* 每次传输数据包大小 */
    int ack;                    /* 应答标记位 */
    int model;                  /* 升级模式 */
    char filename[PACK_NAME_SIZE];     /* 文件名 */
    char buf[BUFFER_SIZE];            /* 文件源数据 */
} ;

int  unpack(int client_fd, struct FilePackage *rPackage)
{
    int ret=0;
    static int data_len=0;
    if(2 == rPackage->ack)
    {
        FILE *fp=NULL;
        if(0 == data_len)
        {
            fp=fopen(rPackage->filename,"wb");
        }
        else
            fp=fopen(rPackage->filename,"ab");
        data_len+=rPackage->filesize;
        printf("recv %d,total:%d\n",rPackage->filesize,data_len);
        
        fwrite(rPackage->buf,rPackage->filesize,1,fp);
        fflush(fp);
        fclose(fp);
    }
    else
    {
        printf("model:%d\n",rPackage->model);
        printf("filename:%s,filesize:%d\n",rPackage->filename,rPackage->filesize);
        if(3 == rPackage->ack)
        {
            printf("md5:%s\n",rPackage->buf);
        }
        data_len=0;
    }
    
    
    return ret;
}
struct FilePackage pack(char tCmd, char *tBuf, char *tFilename, int tFilesize,
        int tAck, int tCount, int model)
{
    struct FilePackage tPackage;
    tPackage.cmd = tCmd;
    memset(tPackage.buf, 0, sizeof(tPackage.buf));  // add by lhj 20190806
    memcpy(tPackage.buf, tBuf, tCount);
    memset(tPackage.filename,0,sizeof(tPackage.filename));
    strcpy(tPackage.filename, tFilename);
    tPackage.filesize = tFilesize;
    tPackage.ack = tAck;

    //dbg_out(DBG_INFO, "buf: %s, len: %d, tCount: %d\n", tPackage.buf, strlen(tPackage.buf), tCount);
    //dbg_out(DBG_INFO, "filename: %s, len: %d\n", tPackage.filename, strlen(tPackage.filename));

    return tPackage;
}


int main(int argc ,char **argv)
{

	int sockfd;
	struct sockaddr_in server_addr;
    WSADATA wsadata;
	WSAStartup(MAKEWORD(2,2),&wsadata);
      
	struct hostent* host;
	if(argc!=2)
	{
		printf("need to enter server ip\n");
		exit(0);
	}
    if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0)
	{
        int i=WSAGetLastError();
		printf("socket erro:%s %d\n",strerror(errno),i);
        
		exit(-1);
	}

    //bzero(&server_addr,sizeof(struct sockaddr_in));
	memset(&server_addr,0,sizeof(struct sockaddr_in));
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	//server_addr.sin_addr=*((struct in_addr*)host->h_addr);
	server_addr.sin_port=htons(portnumber);
	if((host =gethostbyname(argv[1]))==NULL)
	{
		printf("get host erro\n");
		//server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        inet_pton(AF_INET, "192.168.13.159", &server_addr.sin_addr);
	}
    else
    {
        server_addr.sin_addr=*((struct in_addr*)host->h_addr);
    }
	

	



	if((connect(sockfd,(struct sockaddr*)&server_addr,sizeof(struct sockaddr)))==-1)
	{
		printf("connect error\n");
		exit(-1);
	}

	printf(" connnected\n ");
    fd_set read_fds = {{0}};
    fd_set except_fds = {{0}};


    {
        struct FilePackage sendPack;
        char tmpstr[] = "coco";
        sendPack = pack('L', tmpstr, tmpstr, strlen(tmpstr), 1, strlen(tmpstr), 0X0F59);
        send(sockfd, (char *)&sendPack, sizeof(struct FilePackage),0);
    }
    int data_len=0;
    int data_len_temp=0;

	while(1)
	{

		FD_ZERO(&read_fds);
    	FD_ZERO(&except_fds);
    	FD_SET(sockfd, &read_fds);
    	FD_SET(sockfd, &except_fds);
    	int st = select(sockfd+1,&read_fds,NULL,&except_fds,NULL);
    	if( st == -1 ){
    	    printf("fd:%d select fail!", sockfd);
    	    break;
    	}
    	if( FD_ISSET(sockfd,&except_fds )){
    	    printf("fd:%d select exception!", sockfd);
    	    break;
    	}
    	
        if( FD_ISSET(sockfd,&read_fds ))
        {
            data_len = recv(sockfd, (char*)buf, sizeof(struct FilePackage), MSG_WAITALL);
        	if(data_len<0)
        	{
                printf("get data <0 \n");
        	    break;
        	}
        	else if(data_len==0)
        	{
                printf("get data 0\n");
        	    break;
        	}
            if(data_len != sizeof(struct FilePackage) )
            {
                printf("should not happen...");
                break;
            }
            st=unpack(sockfd, buf);
            if(st<0)
            {
                break;
            }
        }
	}
    sleep(2);
	close(sockfd);
	exit(0);

}
