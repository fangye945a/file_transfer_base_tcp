#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
//#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <fcntl.h>
#include <sys/select.h>


#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
//#include <signal.h>
#include "protocol.h"

void change_chmod(char *);
volatile int terminate = 0 ;

int read_resp(int sock, unsigned char resp[], int *len)
{
	unsigned char ch;
	int i = 0;

	do
	{
		read(sock, &ch, 1);
	}
	while (ch != 0xc0);

	while (ch == 0xc0)
	{
		read(sock, &ch, 1);
	}

	//ch当前存的肯定是oxc0后面那个不为0xc0的那个字符
	//包的第一个字节

	do
	{
		resp[i++] = ch;
		read(sock, &ch, 1);
	}
	while (ch != 0xc0);

	//做一些包的完整性检测
	int pkg_len = (resp[0] & 0xff) |
				((resp[1] & 0xff) << 8)|
				((resp[2] & 0xff) << 16)|
				((resp[3] & 0xff) << 24) ;

	if (pkg_len != i)
	{
		printf("pkg_len != i \n");
		return -1;
	}

	*len = i;


	int j;

	//for (j = 0; j < i; j++)
	//{
	//	printf("%02x ", resp[j] & 0xff);
	//}
	//printf("\n");



	return 0;
	
}


int cmd_ls(const char *cmd, int len, int sock)
{
	unsigned char send_cmd[512];
	int i =0;
	int pkg_len  = 8;// pkg_len  + cmd_no

	send_cmd[i++] = 0xc0; //包头

	//pkg_len
	send_cmd[i++] = pkg_len & 0xff;
	send_cmd[i++] = (pkg_len >> 8) & 0xff;
	send_cmd[i++] = (pkg_len >> 16) & 0xff;
	send_cmd[i++] = (pkg_len >> 24) & 0xff;


	//cmd_no
	send_cmd[i++] = FTP_CMD_LS& 0xff;
	send_cmd[i++] = (FTP_CMD_LS >> 8) & 0xff;
	send_cmd[i++] = (FTP_CMD_LS >> 16) & 0xff;
	send_cmd[i++] = (FTP_CMD_LS >> 24) & 0xff;

	send_cmd[i++] = 0xc0; //包尾

	int r = write(sock, send_cmd, i);
	if (r == -1)
	{
		perror("write error:");
		return -1;
	}

	//

	unsigned char *resp = malloc(8192);
	int resp_len;

	if (read_resp(sock, resp , &resp_len) < 0 )
	{
		return -1;
	}
	printf("%s\n", resp + 13);
	
	free(resp);
	resp = NULL;
	return 0;
}


int cmd_get(char *cmd, int len, int sock)
{
	unsigned char send_cmd[1024];
	int i = 0;
	int pkg_len; // ==4 + 4 + 4 + r(文件名的长度)
	int arg_1_len;
	unsigned char filename[512];

//	printf("[%s] %s L_%d\n", __FILE__, __FUNCTION__, __LINE__);

	char *p = cmd + 3;
	unsigned char *pl;
	while (*p == ' ') p++;
	if(*p == '\0' || *p == 0x0a || *p == 0x0b)
	{
		printf("\033[;31mDownload fail!\033[0m\n");
		printf("Plese appoint a file name after \"get\".\n");
		return 0;
	}
	pl = p ;
	while (*pl !=' '  &&  *pl != '\0' && *pl != '\n') pl++;
	*pl = '\0';

//	printf("[%s] %s L_%d\n", __FILE__, __FUNCTION__, __LINE__);
	//get      main.c   1.c
	arg_1_len = sprintf(filename, "%s", p) + 1;

//	printf("filename = %s\n", filename);
	
	pkg_len = 4 + 4 + 4 + arg_1_len;


	send_cmd[i++] = 0xc0;

		//pkg_len
	send_cmd[i++] = pkg_len & 0xff;
	send_cmd[i++] = (pkg_len >> 8) & 0xff;
	send_cmd[i++] = (pkg_len >> 16) & 0xff;
	send_cmd[i++] = (pkg_len >> 24) & 0xff;


	//cmd_no
	send_cmd[i++] = FTP_CMD_GET& 0xff;
	send_cmd[i++] = (FTP_CMD_GET >> 8) & 0xff;
	send_cmd[i++] = (FTP_CMD_GET >> 16) & 0xff;
	send_cmd[i++] = (FTP_CMD_GET >> 24) & 0xff;

	
		//arg_1_len
	send_cmd[i++] = arg_1_len & 0xff;
	send_cmd[i++] = (arg_1_len >> 8) & 0xff;
	send_cmd[i++] = (arg_1_len >> 16) & 0xff;
	send_cmd[i++] = (arg_1_len >> 24) & 0xff;


	//strcpy(send_cmd + i, filename);
	int j;
	for (j = 0; j < arg_1_len; j++)
	{
		send_cmd[i++] = filename[j];
	}

	send_cmd[i++] = 0xc0;//包尾

	//printf("[%s] %s L_%d\n", __FILE__, __FUNCTION__, __LINE__);
	int r = write(sock, send_cmd, i);

	//printf("[%s] %s L_%d\n", __FILE__, __FUNCTION__, __LINE__);
	unsigned char *resp = malloc(8192);
	int resp_len;

	if (read_resp(sock, resp , &resp_len) < 0 )
	{
		return -1;
	}
	//printf("[%s] %s L_%d\n", __FILE__, __FUNCTION__, __LINE__);

	unsigned char result = resp[12];
	if (result != 0)
	{
		printf("result != 0\n");
		return -1;
	}

	int fileSize = (resp[13] & 0xff ) |
				((resp[14] & 0xff) << 8 ) |
				((resp[15] & 0xff) << 16 ) |
				((resp[16] & 0xff) << 24 ) ;
	printf("The fileSize is %d bytes..\n",fileSize);


	int fd = open(filename, O_WRONLY | O_TRUNC |O_CREAT, 0660);
	if (fd == -1)
	{
		perror("open error:");
		return -1;
	}

	r  = 0;
	while (r < fileSize)
	{
		char buf[4096];
		int l = read(sock,buf, 4096);
		if (l > 0)
		{
			write(fd, buf, l);
			r += l;
		}
	}
	close(fd);
//  printf("%d %s\n",__LINE__,__FUNCTION__);
	change_chmod(filename);
//	printf("%d %s\n",__LINE__,__FUNCTION__);
	printf("\033[;32mDownload finish...\033[0m\n");
	return 0;
}
void change_chmod(char *filename)
{
	int chmod = 777;
	char *p=filename;

	//printf("%d %s\n",__LINE__,__FUNCTION__);
/*	while(*p != '\0')
	{
		if(*p == '.')
		{
			if(strncmp(p,".out",2) != 0)
				chmod = 644;
		}
		p++;
	}
*/
	char str[4096]={0};
	
	//printf("%d %s\n",__LINE__,__FUNCTION__);
	sprintf(str,"%s %d %s","chmod",chmod,filename);
	//printf("%d %s\n",__LINE__,__FUNCTION__);
	//printf("str = %s\n",str);
	system(str);
}

int cmd_put(char *cmd, int len, int sock)
{
	unsigned char send_cmd[1024];
	int i = 0,r;
	int pkg_len; // ==4 + 4 + 4 + r(文件名的长度)
	int arg_1_len;
	unsigned char filename[512];

	//printf("[%s] %s L_%d\n", __FILE__, __FUNCTION__, __LINE__);

	char *p = cmd + 3;
	unsigned char *pl;
	while (*p == ' ') p++;

	//printf("%c\n", *p);
	if(*p == '\0' || *p == 0x0a || *p == 0x0b)
	{
		printf("\033[;31mUpload fail!\033[0m\n");
		printf("Plese appoint a file name after \"put\".\n");
		return 0;
	}

	pl = p ;
	while (*pl !=' '  &&  *pl != '\0' && *pl != '\n') pl++;
	*pl = '\0';

	//printf("[%s] %s L_%d\n", __FILE__, __FUNCTION__, __LINE__);
	//get      main.c   1.c
	arg_1_len = sprintf(filename, "%s", p) + 1;
	
	//printf("[%s] %s L_%d\n", __FILE__, __FUNCTION__, __LINE__);
	//printf("filename = %s\n", filename);
	
	pkg_len = 4 + 4 + 4 + arg_1_len + 4;//4代表文件长度


	send_cmd[i++] = 0xc0;

		//pkg_len
	send_cmd[i++] = pkg_len & 0xff;
	send_cmd[i++] = (pkg_len >> 8) & 0xff;
	send_cmd[i++] = (pkg_len >> 16) & 0xff;
	send_cmd[i++] = (pkg_len >> 24) & 0xff;


	//cmd_no
	send_cmd[i++] = FTP_CMD_PUT& 0xff;
	send_cmd[i++] = (FTP_CMD_PUT >> 8) & 0xff;
	send_cmd[i++] = (FTP_CMD_PUT >> 16) & 0xff;
	send_cmd[i++] = (FTP_CMD_PUT >> 24) & 0xff;

	
	//arg_1_len //文件名长度
	send_cmd[i++] = arg_1_len & 0xff;
	send_cmd[i++] = (arg_1_len >> 8) & 0xff;
	send_cmd[i++] = (arg_1_len >> 16) & 0xff;
	send_cmd[i++] = (arg_1_len >> 24) & 0xff;
	//printf("arg_1_len=%d\n",arg_1_len);
	int filesize;

	int fd = open(filename, O_RDONLY);
	if (fd == -1)
	{
		perror("\033[;31mopen failed\033[0m");
		return -1;
	}
	filesize = lseek(fd, 0L, SEEK_END);
	printf("The fileSize is %d bytes..\n",filesize);
	
	send_cmd[i++] = filesize & 0xff;
	send_cmd[i++] = (filesize >> 8) & 0xff;
	send_cmd[i++] = (filesize >> 16) & 0xff;
	send_cmd[i++] = (filesize >> 24) & 0xff;
	
	
	int j;
	for (j = 0; j < arg_1_len; j++)//把文件名发过去
	{
		send_cmd[i++] = filename[j];
	}

	send_cmd[i++] = 0xc0;//包尾

	//printf("[%s] %s L_%d\n", __FILE__, __FUNCTION__, __LINE__);
	r=write(sock, send_cmd, i);
	if(r<0)
	{
	//	printf("[%s] massage %d\n", __FUNCTION__, __LINE__);
		perror("put write error");
		return -1;
	}
/**************************打包完成*********************************/


	lseek(fd, 0L, SEEK_SET);
	while (1)
	{	
		int l;
		char buf[4096];

		l = read(fd, buf, 4096);
		if (l > 0)
		{
			write(sock, buf, l);
		} else if (l == 0)
		{
			break;
		}
	}
	close(fd);	
	printf("\033[;32mUpload finish...\033[0m\n");
	return 0;
}

int cmd_bye(char *cmd, int len, int sock)
{
	
	unsigned char send_cmd[512];
	int i =0;
	int pkg_len  = 8;// pkg_len  + cmd_no

	send_cmd[i++] = 0xc0; //包头

	//pkg_len
	send_cmd[i++] = pkg_len & 0xff;
	send_cmd[i++] = (pkg_len >> 8) & 0xff;
	send_cmd[i++] = (pkg_len >> 16) & 0xff;
	send_cmd[i++] = (pkg_len >> 24) & 0xff;


	//cmd_no
	send_cmd[i++] = FTP_CMD_BYE& 0xff;
	send_cmd[i++] = (FTP_CMD_BYE >> 8) & 0xff;
	send_cmd[i++] = (FTP_CMD_BYE >> 16) & 0xff;
	send_cmd[i++] = (FTP_CMD_BYE >> 24) & 0xff;

	send_cmd[i++] = 0xc0; //包尾
	
	int r = write(sock, send_cmd, i);
	if (r == -1)
	{
		perror("write error:");
		return -1;
	}
	
	int ret;
	int j = 0,t;
	char name[100]={0};
	

	//printf("LS:%s,%d\n",__FUNCTION__,__LINE__);
	unsigned char ch ;

	
	do
	{
		read(sock,&ch,1);
	}while(ch != 0xc0);
	
	while(ch == 0xc0)
	{
		read(sock,&ch,1);
	}//ch 就是这个包的第一个字节
	
	
	for(t=0;t<8;t++)
	{
		read(sock,&ch,1);			
	}
	
	do
	{
		read(sock,&ch,1);
		name[j] = ch;
	//	printf("%c",name[j]);
		j++;
	}while(ch != 0xc0);
	//printf("%s\n",name);
	//putchar('\n');
	return 0;
}

void sig_handler(int signum)
{
	switch(signum)
	{
		case SIGINT:
			terminate = 1;
			break;
		default:
			break;
	}
}
void printhelp()
{
	printf("\033[;35mYou can use the following command.\033[0m\n");
	printf("\033[;35mCMD:\t\t\tFUNCTION:\033[0m\n");
	printf("\033[;35mls\t\t\tList the server file.\033[0m\n");
	printf("\033[;35mlc\t\t\tList the client(local) file.\033[0m\n");
	printf("\033[;35mclear\t\t\tClear the screen.\033[0m\n");
	printf("\033[;35mget [server filename]\tDownload server file.\033[0m\n");
	printf("\033[;35mput [client filename]\tUpload client file.\033[0m\n");
	printf("\033[;35mbye\t\t\tExit the client.\033[0m\n");
	printf("\033[;35mhelp\t\t\tGet help message.\033[0m\n");
}

int main(int argc, char *argv[])
{
	int sock;
	char ipaddr[16] = {0};
	char portnum[16] = {0};
	char begin_massage[512]={0};
	if(argc == 2)
	{
		strcpy(ipaddr,argv[1]);
		strcpy(portnum,"1883");
	}
	else if(argc == 3)
	{
		strcpy(ipaddr,argv[1]);
		strcpy(portnum,argv[2]);
	}	
	else
	{
		printf("\033[;31mPlease run the client with IPaddr and Port!\033[0m\n");
		printf("\033[;31mExample1: ./client 192.168.1.80 8080\033[0m\n");
		printf("\033[;31mExample2: ./client 192.168.1.80 (Port default is 1883)\033[0m\n");
		return 0;
	}
	signal(SIGINT, sig_handler);
	/*
		socket

		connect

	*/


	/*
	step 1:创建一个套接字(SOCK_STREAM)
	*/
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		perror("socket error:");
		return -1;
	}


	/*
	step 2: 绑定一个"众所周知"的端口
	*/
	struct sockaddr_in sAddr;
	memset(&sAddr, 0, sizeof(sAddr));
	sAddr.sin_family = AF_INET;
			//atoi把一个数字字符串变成一个整数
			//atoi("1234") =>  1234
	sAddr.sin_port = htons ( atoi(portnum) );
	sAddr.sin_addr.s_addr = inet_addr(ipaddr); 
	//sAddr.sin_addr.s_addr = htonl(  INADDR_ANY );
	
	int r = connect(sock, (struct sockaddr*) &sAddr, sizeof(sAddr));
	if (r == -1)
	{
		perror("\033[;31mconnect error\033[0m");
		return -1;
	}
	
	recv(sock,begin_massage,512,0);
	printf("\033[;32m%s\033[0m\n",begin_massage);
	//fprintf(stdout,"\033[;32m%s\033[0m",begin_massage);
	//main loop
	while (!terminate)
	{
		/*
		从命令行得到用户输入的ftp命令
		*/
		fprintf(stdout,"\033[;34mftp >\033[0m");
		
		unsigned char cmd[512]={0};
		fgets(cmd, 512, stdin);
		/*
		处理的用户命令
		*/
		if(*cmd == 0x0d || *cmd == ' ' || *cmd == 0x0a)
			continue;
		if (strncmp(cmd, "ls", 2) == 0) //是ls命令
		{
			printf("These file in the sever:\n");
			cmd_ls(cmd, strlen(cmd), sock);
		}else if(strncmp(cmd, "lc", 2) == 0)
		{
			printf("These file in the client:\n");
			system("ls");
		}else if(strncmp(cmd, "help", 4) == 0)
		{
			printhelp();
		}else if(strncmp(cmd, "clear", 5) == 0)
		{
			system("clear");
		}else if (strncmp(cmd, "get", 3) == 0) //get命令
		{
			printf("Downloading the file ...\n");
			cmd_get(cmd, strlen(cmd), sock);
		}else if (strncmp(cmd, "put", 3) == 0) //put命令
		{
			printf("Uploading the file ...\n");
			cmd_put(cmd, strlen(cmd), sock);
		}else if (strncmp(cmd, "bye", 3) == 0) //get命令
		{
			printf("\033[;32mYou have disconnected from the server,Goodbye!\033[0m\n");
			cmd_bye(cmd, strlen(cmd), sock);
			return 0;
		}
		else printf("\033[;31mcmd error! You can get help from cmd: \"help\"\033[0m\n");		
		
	}
	return 0;
}
