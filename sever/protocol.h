#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#define FTP_ROOT_DIR   "./"


//�����, �������


//�����->  ����
enum CMD_NO 
{
	FTP_CMD_LS = 1024, //
	FTP_CMD_GET,
	FTP_CMD_PUT,
	FTP_CMD_BYE,
	
	FTP_CMD_NUM // �������
};

//������
enum resp_result
{
	RESP_SUCCESS = 0, //�ɹ�
		
};


//����: �������ȣ� ��������



/*
	0xc0 : ��ͷ

	pkg_len ;//4bytes, С��ģʽ���������ݰ��ĳ���
				 4(pkg_len) + 4 (cmd_no) + arg_1 +...
	cmd_no // 4bytes, С��ģʽ

	arg_1;
		arg_1_len; //4bytes , С��ģʽ
		arg_1_data; len����
	arg_2:
		arg_2_len; //4bytes,С��ģʽ
		arg_2_data;
	....

	0xC0:��β
*/

// cmd: ls

//0xc0  ___������______    __�����()__             0xc0
// 0xc0  0x80 0x00 0x00 0x00 


//cmd: get
//0xc0 ____������(4)___��___�����(4)___   ___arg_1_Len(4)_   ___filename(r)___ 0xc0




#if 0
unsigned char cmd[1024];
int i = 0;

int pkg_len = 1024;

cmd[0] = 0xc0;

cmd[1] = pkg_len & 0xff;
cmd[2] = ( pkg_len >> 8) & 0xff;
cmd[3] = (pkg_len >> 16) & 0xff;
cmd[4] = (pkg_len >> 24) & 0xff;
#endif



//CMD_RESP
/*
	0xc0: ��ͷ

	pkg_len: �������ݰ��ĳ��ȣ�4bytes, С��ģʽ
	cmd_no :  ����ţ�4bytes, С��ģʽ����ʾ�ظ���������
	resp_len: �ظ����ݵĳ��ȣ�4bytes, С��ģʽ
	result:  ִ�гɹ���ʧ�ܣ�1 bytes , 0��ʾ�ɹ���������ʾʧ��
	resp_conent: �ظ�����
			�ɹ�:
				ls: �ļ�����
				get: �ļ���С��4bytes, С��ģʽ

			ʧ��:
				 ������

	0xc0:��β
*/


//ls �Ļظ�
// 0xc0  --pkg_Len(4)---   ---cmd_No(4)---  --resp_len(4)--  --result(1)--  --filenames(r, �����Կո�ֿ�)--- 0xc0


//get �Ļظ�
//0xc0  ---pkg_len(4)---  ----cmd_no(4)---  ---resp_len(4)---   ---result(1)--  --file_size(4, С��ģʽ)--����0xc0

#endif
