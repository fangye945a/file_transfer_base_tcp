#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <fcntl.h>
#include <sys/select.h>
#include <sys/stat.h>

#include <sys/ioctl.h>  
#include <net/if.h>  

#include <sys/time.h>
#include <unistd.h>

#include "protocol.h"
#define begin "Welcome to my Server! What can I do for you?"

int terminate = 0;//ȫ�ֱ����������˳���־��1��ʾ�����˳���0��ʾ���˳�

  
char* GetLocalIp()    
{          
    int MAXINTERFACES=16;    
    char *ip = NULL;    
    int fd, intrface, retn = 0;      
    struct ifreq buf[MAXINTERFACES];      
    struct ifconf ifc;      
  
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)      
    {      
        ifc.ifc_len = sizeof(buf);      
        ifc.ifc_buf = (caddr_t)buf;      
        if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc))      
        {      
            intrface = ifc.ifc_len / sizeof(struct ifreq);      
  
            while (intrface-- > 0)      
            {      
                if (!(ioctl (fd, SIOCGIFADDR, (char *) &buf[intrface])))      
                {      
                    ip=(inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr));      
                    break;    
                }                          
            }    
        }      
        close (fd);      
        return ip;      
    }    
}

int create_init_socket(const char *ip,  short port)
{
	int sock; //�׽���������
	int r;

	/*
	step 1: ����һ���׽���.
	*/
	sock = socket(AF_INET, SOCK_STREAM, 0);

	/*step 2: ��һ����ַ*/

	struct sockaddr_in  servIP;
	memset(&servIP, 0, sizeof(servIP));

	servIP.sin_family = AF_INET;
	servIP.sin_port = htons(  port ); //�˿ں�
	//inet_aton("192.168.1.4", &(servIP.sin_addr));
	servIP.sin_addr.s_addr = inet_addr(ip);

	//servIP.sin_addr.s_addr = htonl(INADDR_ANY); //�ò���ϵͳ�Զ�ѡ��һ��������ip

	
	r = bind(sock, (struct sockaddr *)&servIP, sizeof(servIP));
	if (r == -1)
	{
		perror("\033[;31mbind error\033[0m");
		return -1;
	}

	/*
	step 3: listen
	*/
	listen(sock, 10);


	return sock;

}


/*
	read_cmd:�������׽��������������ȡ
		һ������������
	@cmd:������������������ȡ������
			������Ҫ�㹻��
	@len:������������ĳ���
	@connfd: �׽���������
	����ֵ:
		���������

*/

int  read_cmd(unsigned char  cmd[], int *len,  int connfd)
{
	//char cmd[1024] ;// �ֲ���������ջ�ռ䣬
	unsigned char ch;
	int i = 0;

	//0xc0 xxxxxx 0xc0  0xc0 xxxxxx 0xc0 

	
	do
	{
		read(connfd, &ch, 1);
	}
	while (ch != 0xc0);

	while (ch == 0xc0)
	{
		read(connfd, &ch, 1);
	} // -> ch����������ĵ�һ���ֽ�

	do
	{
		cmd[i++] = ch;
		read(connfd, &ch, 1);
	}
	while (ch != 0xc0);// cmd����i���ֽھ�һ������������()

	*len = i;

	int pkg_len = (cmd[0] & 0xff) |
				((cmd[1] & 0xff ) << 8 ) |
				((cmd[2] & 0xff ) << 16 ) |
				((cmd[3] & 0xff ) << 24 )  ;
	if (pkg_len != i)
	{
		//printf("pkg_len[%d] != i[%d]\n", pkg_len, i);
		return -1;
	}

	int cmd_no =  (cmd[4] & 0xff) |
				((cmd[5] & 0xff ) << 8 ) |
				((cmd[6] & 0xff ) << 16 ) |
				((cmd[7] & 0xff ) << 24 )  ;


	//printf("[%s ]%s L_%d\n",__FILE__,  __FUNCTION__, __LINE__);
/*
	int j;
	for (j = 0; j < i; j++)
	{
		printf("%02x ", cmd[j] & 0xff);
	}
	printf("\n");
*/


	return cmd_no;
}


void change_chmod(char *filename)
{
	int chmod = 777;
	char *p=filename;

//	printf("%d %s\n",__LINE__,__FUNCTION__);
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
	
//	printf("%d %s\n",__LINE__,__FUNCTION__);
	sprintf(str,"%s %d %s","chmod",chmod,filename);
//	printf("%d %s\n",__LINE__,__FUNCTION__);
//	printf("str = %s\n",str);
	system(str);
}

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

	//ch��ǰ��Ŀ϶���oxc0�����Ǹ���Ϊ0xc0���Ǹ��ַ�
	//���ĵ�һ���ֽ�

	do
	{
		resp[i++] = ch;
		read(sock, &ch, 1);
	}
	while (ch != 0xc0);

	//��һЩ���������Լ��
	int pkg_len = (resp[0] & 0xff) |
				((resp[1] & 0xff) << 8)|
				((resp[2] & 0xff) << 16)|
				((resp[3] & 0xff) << 24) ;

	if (pkg_len != i)
	{
//		printf("pkg_len != i \n");
		return -1;
	}

	*len = i;

/*
	int j;

	for (j = 0; j < i; j++)
	{
		printf("%02x ", resp[j] & 0xff);
	}
	printf("\n");

*/

	return 0;
	
}
/*
	ftp_cmd_ls:�����ظ�ls����
	@cmd:��������
	@len:�����
	@connfd:�׽���������
	����ֵ:
		��
*/
void ftp_cmd_ls(char *cmd, int len, int connfd)
{
	/*
	step 1: ��ȡftp_root_dirĿ¼���棬���е�
	�ļ�����������������
	*/
	unsigned char filenames[4096];
	
	DIR *dir = NULL;
	dir = opendir(FTP_ROOT_DIR);
	if (dir == NULL)
	{
		perror("opendir error:");
		return ;
	}

	int r = 0;
	struct dirent *dirp = NULL;
	struct stat st;
	int ret;
	while (dirp = readdir(dir))
	{
		ret = stat(dirp->d_name,&st);//�ж��ļ�����
		if(ret <0 )
		{
			perror("stat error");
			continue;
		}
		if(S_ISDIR(st.st_mode))
			continue;
		r += sprintf(filenames + r, "%s\t", dirp->d_name);  
	}
	closedir(dir);


	/*
	step 2: ���һ���ظ������ٷ���
	*/

	// pkg_len(4) + cmd_no(4) + resp_len(4) + result(1) + resp_content(r)	
	int pkg_len =  4 + 4 + 4 + 1 + r; //�����ܳ���
	int cmd_no = FTP_CMD_LS; //�����
	int resp_len =  1 + r ;//�ظ����ݵĳ���(���ݳ���+ result(1))
	char result = 0;
	int i = 0;
	char *resp = malloc(pkg_len + 2);

	resp[i++] = 0xc0;

	//�����ȣ���С��ģʽ�����
	resp[i++] = pkg_len & 0xff;
	resp[i++] = (pkg_len >> 8 ) & 0xff;
	resp[i++] = (pkg_len >> 16 ) & 0xff;
	resp[i++] = (pkg_len >> 24 ) & 0xff;

	//����ţ���С��ģʽ�����
	resp[i++] = cmd_no& 0xff;
	resp[i++] = (cmd_no >> 8 ) & 0xff;
	resp[i++] = (cmd_no >> 16 ) & 0xff;
	resp[i++] = (cmd_no >> 24 ) & 0xff;

	//�ظ����ȣ���С��ģʽ�����
	resp[i++] = resp_len& 0xff;
	resp[i++] = (resp_len >> 8 ) & 0xff;
	resp[i++] = (resp_len >> 16 ) & 0xff;
	resp[i++] = (resp_len >> 24 ) & 0xff;

	//result
	resp[i++] = 0;


	int j;
	for (j = 0; j < r; j++)
	{
		resp[i++] = filenames[j];
	}
	resp[i++] = 0xc0;


	write(connfd, resp , i);

}


void ftp_cmd_get(char *cmd, int len, int connfd)
{
	char filename[512];
	char *p = cmd + 12;


	sprintf(filename, "%s%s", FTP_ROOT_DIR, p);

	int filesize;
	int fd = open(filename, O_RDONLY);
	if (fd == -1)
	{
	//	printf("The file is not exist.\n");
		char cmd[64]={0};
		sprintf(cmd,"touch %s",filename);
		system(cmd);
//		printf("File have created.\n");

		fd = open(filename, O_RDONLY);
		if (fd == -1)
		{
			perror("open fail");
			return;
		}
	}
	filesize = lseek(fd, 0L, SEEK_END);
	printf("Send %d bytes file.\n",filesize);
	printf("file name:%s\n",filename+2);

	lseek(fd, 0L, SEEK_SET);

	/*
	step 2: ���һ���ظ������ٷ���
	*/

	// pkg_len(4) + cmd_no(4) + resp_len(4) + result(1) + resp_content(r)	
	int pkg_len =  4 + 4 + 4 + 1 + 4; //�����ܳ���
	int cmd_no = FTP_CMD_GET; //�����
	int resp_len =  1 + 4;//�ظ����ݵĳ���(���ݳ���+ result(1))
	char result = 0;
	int i = 0;
	char *resp = malloc(pkg_len + 2);

	resp[i++] = 0xc0;

	//�����ȣ���С��ģʽ�����
	resp[i++] = pkg_len & 0xff;
	resp[i++] = (pkg_len >> 8 ) & 0xff;
	resp[i++] = (pkg_len >> 16 ) & 0xff;
	resp[i++] = (pkg_len >> 24 ) & 0xff;

	//����ţ���С��ģʽ�����
	resp[i++] = cmd_no& 0xff;
	resp[i++] = (cmd_no >> 8 ) & 0xff;
	resp[i++] = (cmd_no >> 16 ) & 0xff;
	resp[i++] = (cmd_no >> 24 ) & 0xff;

	//�ظ����ȣ���С��ģʽ�����
	resp[i++] = resp_len& 0xff;
	resp[i++] = (resp_len >> 8 ) & 0xff;
	resp[i++] = (resp_len >> 16 ) & 0xff;
	resp[i++] = (resp_len >> 24 ) & 0xff;

	//result
	resp[i++] = 0;


	//�ļ�����
	resp[i++] = filesize & 0xff;
	resp[i++] = (filesize >> 8 ) & 0xff;
	resp[i++] = (filesize >> 16 ) & 0xff;
	resp[i++] = (filesize >> 24) & 0xff;


	resp[i++] = 0xc0;


	write(connfd, resp , i);

	int r = 0;
	while (1)
	{	
		int l;
		char buf[4096];

		l = read(fd, buf, 4096);
		if (l > 0)
		{
			write(connfd, buf, l);
		} else if (l == 0)
		{
			break;
		}
	}
	close(fd);
	char del_cmd[64]={0};
	sprintf(del_cmd,"rm %s",filename+2);
//	printf("del_cmd = %s\n",del_cmd);
	system(del_cmd);
}


void ftp_cmd_put(char *cmd, int len, int connfd)
{
//	printf("haha i go to the put func\n");
	char filename[512];
	char *p = cmd + 16;

	sprintf(filename, "%s%s", FTP_ROOT_DIR, p);

	
	
	int filename_len = 	(cmd[8] & 0xff ) |
						((cmd[9] & 0xff) << 8 ) |
						((cmd[10] & 0xff) << 16 ) |
						((cmd[11] & 0xff) << 24 ) ;
//	printf("filename_len = %d\n",filename_len);
	
	
	
	int fileSize = 	(cmd[12] & 0xff ) |
					((cmd[13] & 0xff) << 8 ) |
					((cmd[14] & 0xff) << 16 ) |
					((cmd[15] & 0xff) << 24 ) ;
	printf("Recieve %d bytes file.\n",fileSize);
	printf("file name:%s\n",filename);
	

	int fd = open(filename, O_WRONLY | O_TRUNC |O_CREAT, 0660);
	if (fd == -1)
	{
		perror("open error:");
		return ;
	}

	
	int r  = 0;
	while (r < fileSize)
	{
		char buf[4096];
		int l = read(connfd,buf, 4096);
		if (l > 0)
		{
			write(fd, buf, l);
			r += l;
		}
	}

	close(fd);
	change_chmod(filename);
}
void ftp_cmd_bye(char *cmd, int len, int connfd)
{
//	printf("hello to bye func\n");
		//Ҫ���͵Ľ����ַ���
	unsigned char filenames[4096] = "GoodBye!";
	
		// pkg_len(4) + cmd_no(4) + resp_len(4) + result(1) + resp_content(r)	
	int r=strlen(filenames);
	int pkg_len =  4 + 4 + 4 + 1 + r; //�����ܳ���
	int cmd_no = FTP_CMD_BYE; //�����
	int resp_len =  1 + r ;//�ظ����ݵĳ���(���ݳ���+ result(1))
	char result = 0;
	int i = 0;
	char *resp = malloc(pkg_len + 2);

	resp[i++] = 0xc0;

	//�����ȣ���С��ģʽ�����
	resp[i++] = pkg_len & 0xff;
	resp[i++] = (pkg_len >> 8 ) & 0xff;
	resp[i++] = (pkg_len >> 16 ) & 0xff;
	resp[i++] = (pkg_len >> 24 ) & 0xff;

	//����ţ���С��ģʽ�����
	resp[i++] = cmd_no& 0xff;
	resp[i++] = (cmd_no >> 8 ) & 0xff;
	resp[i++] = (cmd_no >> 16 ) & 0xff;
	resp[i++] = (cmd_no >> 24 ) & 0xff;

	//�ظ����ȣ���С��ģʽ�����
	resp[i++] = resp_len& 0xff;
	resp[i++] = (resp_len >> 8 ) & 0xff;
	resp[i++] = (resp_len >> 16 ) & 0xff;
	resp[i++] = (resp_len >> 24 ) & 0xff;

	//result
	resp[i++] = 0;


	int j;
	for (j = 0; j < r; j++)
	{
		resp[i++] = filenames[j];
//		printf("%02x",resp[i]);
		//printf("[%s] massage %d\n",__FUNCTION__,__LINE__);
	}
	resp[i++] = 0xc0;

	write(connfd, resp , i);
}
//������:
//  ���϶���	: 

// ��������Ĺ���ģ���ʵ��


/*
	handle_ftp_connection:�������ղ�����ӿͻ��˷�������
		���
	@connfd: �����׽���������
	����ֵ:
		��
*/
void handle_ftp_connection(int connfd)
{
	while (!terminate)
	{
		char cmd[1024];
		int len = 0;
		int cmd_no;//�����
	
		/*
		step 1: ��������
		*/
		cmd_no = read_cmd(cmd , &len, connfd);	
//		printf("cmd_no=%d\n",cmd_no);	

		/*
		step 2: ��������
		*/
		switch (cmd_no)
		{
			case FTP_CMD_LS:
				ftp_cmd_ls(cmd, len, connfd);
				break;
			case FTP_CMD_GET:
				ftp_cmd_get(cmd, len, connfd);
				break;
			case FTP_CMD_PUT:
				ftp_cmd_put(cmd, len, connfd);
				break;
			case FTP_CMD_BYE:
				ftp_cmd_bye(cmd, len, connfd);
				terminate = 1; 
				break;
			default:
				printf("unkown cmd\n");
				break;
		}
		
		
	}
}


/*
	sig_handler:�źŴ�����
*/
void sig_handler(int signo)
{
	switch(signo)
	{
		case SIGINT:
//			printf("received SIGINT\n");
			terminate = 1;
			break;
		default:
			break;
	}
}



// ftp_server  ipaddr  port
int main(int argc, char *argv[])
{
	signal(SIGINT, sig_handler);
	char ipaddr[16]= {0};;
	char port[16] = {0}; 
	/* create and bind a socket*/
	if(argc == 3)
	{
		strcpy(ipaddr,argv[1]);
		strcpy(port,argv[2]);
	}
	else if(argc == 1)
	{
		strcpy(ipaddr,GetLocalIp());
		strcpy(port,"1883");
	}
	else if(argc == 2)
	{
		strcpy(ipaddr,GetLocalIp());
		strcpy(port,argv[1]);
	}
    else
	{
		printf("\033[;31mParam error!Please run the client with IPaddr and Port!\033[0m\n");
		printf("\033[;31mExample1: ./server (Auto get IPaddr and port is 1883.)\033[0m\n");
		printf("\033[;31mExample2: ./server 192.168.1.80 8080 (Specify the IP and port numbers.)\033[0m\n");
		printf("\033[;31mExample3: ./server 8080 (Auto get IPaddr.)\033[0m\n");
		return 0;
	}

	printf("IP = %s,Port = %s\n",ipaddr,port);
	int sock = create_init_socket(ipaddr, atoi(port));
	if(sock < 0)
	{
		perror("create_init_socket fail");
		return 0;
	}
	//unsigned long flags = fcntl(sock, F_GETFL);
	//flags |= O_NONBLOCK;
	//fcntl(sock, F_SETFL, flags);

	int maxfd = sock + 1; //�����Ǹ��ļ�������+1
	fd_set rfds;
	struct timeval timeout;
	

	printf("\033[;32mWaiting client connect!\033[0m\n");
	while (!terminate)
	{
		struct sockaddr_in rAddr;
		memset(&rAddr, 0, sizeof(rAddr));
		socklen_t addrlen = sizeof(rAddr); //NOTE:	

		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);

		timeout.tv_sec = 2;
		timeout.tv_usec = 0;

		int r = select(maxfd, &rfds, NULL, NULL, &timeout);
		if (r  < 0)
		{
			if (errno == EINTR) //���ź��ж���
			{
				continue;
			}
		
			perror("select error:");
			break;
		} else if (r == 0)
		{
			continue;
		}



		if (FD_ISSET(sock, &rfds))
		{

			/*����һ���ӿͻ��˷��͹�������������*/
			int connfd = accept(sock, (struct sockaddr *) &rAddr, &addrlen);
			/*if (connfd == -1)
			{
				printf("errno = %d\n", errno );

				perror("");
			}*/

			

			if (connfd > 0) //���ӳɹ�
			{
				printf("\033[;32mA connection from %s:%d.\033[0m\n",
						inet_ntoa(rAddr.sin_addr),   ntohs (rAddr.sin_port));
				//ѡ�񴴽�һ�����̻�һ���߳�ȥ�����½���������
				//printf("[%s] massage %d\n",__FUNCTION__,__LINE__);
				
				r = send(connfd,begin,strlen(begin),0);
				if(r == -1)
				{
					perror("send begin massage error");
					return -1;
				}
				
				/*���ӽ���ȥ��������ӵ�һ������*/

				pid_t pid = fork();
				if (pid  == 0) //�ӽ���
				{
					handle_ftp_connection(connfd);
					printf("\033[;31mThe connection %s:%d exit.\033[0m\n",
					inet_ntoa(rAddr.sin_addr),   ntohs (rAddr.sin_port));
					exit(0);
				} 
				else if (pid > 0) //������
				{
					close(connfd);
				}
			}
		}

	}

	close(sock);

}
