#include <stdio.h>
#include<string.h>


#include<sys/types.h>
#include<sys/stat.h>
//����ͨ����Ҫ������ͷ�ļ�����Ҫ���صĿ��ļ�
#include <WinSock2.h>
#pragma comment(lib,"WS2_32.lib")
#define PRINTF(str) printf("[%s - %d]"#str"=%s\n", __func__, __LINE__, str);



void error_die(const char* str) {
	perror(str);
	exit(1);
}

//ʵ������ĳ�ʼ��
//����ֵ���׽���
//�������˿�
int startup(unsigned short* port)
{
	//����ͨ�ŵĳ�ʼ��
	WSADATA data;
	int ret = WSAStartup(MAKEWORD(1, 1), &data); //1.1�汾��Э��
	if (ret)
	{
		return -1;
	}
	//�����׽���
	int server_socket = socket(PF_INET,//�׽��ֵ�����
		SOCK_STREAM,//������
		IPPROTO_TCP);
	if (server_socket == -1) {
		//��ӡ������Ϣ
		error_die("WSAStartup");
	}
	//���ö˿ڿɸ���
	int opt = 1;
	ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt,
		sizeof(opt));
	if (ret == -1) {
		error_die("set socket opt");
	}

	//���÷������˵������ַ
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(*port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//���׽���
	if (0 > bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)))
	{
		error_die("bind");
	}
	//��̬����˿�
	int nameLen = sizeof(server_addr);
	if (*port == 0) 
	{
		if (getsockname(server_socket,
			(struct sockaddr*)&server_addr,
			&nameLen) < 0)
		{
			error_die("getsockname");
		}
		*port = server_addr.sin_port;
	}

	//������������
	if (listen(server_socket, 5) < 0)
	{
		error_die("listen");
	}

	return server_socket;
}

//��ָ���Ŀͻ����׽���sock����ȡһ�����ݣ����浽buff��
int get_line(int sock, char* buff, int size)
{
	char c = 0;//'\0'
	int index = 0;
	// \r \n
while (index < size - 1 && c != '\n')
{
	int n = recv(sock, &c, 1, 0);//���ض��˼����ֽ�,
	if (n > 0)
	{
		if (c == '\r')
		{
			n = recv(sock, &c, 1, MSG_PEEK);
			if (n > 0 && c == '\n')
			{
				recv(sock, &c, 1, 0);
			}
			else
			{
				c = '\n';
			}
		}
		buff[index++] = c;
	}
	else
	{
		// to do
		c = '\n';
	}

}
buff[index] = 0; //'0' ������
return index;
}
void unimplement(int client)
{
	//��ָ�����׽��֣�����һ��������ʾҳ��

}
void not_found(int client)
{
	//to do

}
void headers(int client)
{
	//������Ӧ����ͷ��Ϣ
	char buff[1024];
	strcpy_s(buff, "HTTP/1.0 200 OK\r\n");
	send(client, buff, strlen(buff), 0);
	strcpy_s(buff, "Server: GengHttpd/0.1\r\n");
	send(client, buff, strlen(buff), 0);
	strcpy_s(buff, "Content-type:text/html\n");
	send(client, buff, strlen(buff), 0);
	strcpy_s(buff, "\r\n");
	send(client, buff, strlen(buff), 0);

}
void cat(int client,FILE* resource)
{
	char buff[4096];
	int count = 0;

	while (1)
	{
		int ret = fread(buff, sizeof(char), sizeof(buff), resource);
		if (ret <= 0)
		{
			break;
		}
		send(client, buff, ret, 0);
		count += ret;
	}
	printf("һ������[%d]�ֽڸ������\n", count);


}

void server_file(int client, const char* fileName)
{
	int numchars = 1;
	char buff[1024];
	while (numchars > 0 && strcmp(buff, "\n"))
	{
		numchars = get_line(client, buff, sizeof(buff));
		PRINTF(buff);
	}

	FILE* resource = NULL;
	if (strcmp(fileName, "htdocs/index.html") == 0)
	{
		resource = fopen(fileName, "r");
	}
	else {
		resource = fopen(fileName, "rb");
	}
		
	if (resource == NULL) 
	{
		not_found(client);
	}
	else
	{
		//��ʽ������Դ�������
		headers(client);
		//�����������Դ��Ϣ
		cat(client, resource);
		PRINTF("��Դ�������\n");
	}
	if (resource != NULL) fclose(resource);

}

DWORD WINAPI accept_request(LPVOID arg)
{
	int client = (SOCKET)arg;
	char buff[1024];
	//��ȡһ������
	int numchars = get_line(client, buff, sizeof(buff));
	PRINTF(buff);

	char method[255];
	int j = 0;
	int i = 0;
	while (!isspace(buff[j]) && i < sizeof(method) - 1)
	{
		method[i++] = buff[j++];
	}
	method[i] = 0;//'\0'
	PRINTF(method);
	//�������ķ��������������Ƿ�֧��
	if (_stricmp(method, "GET") && _stricmp(method, "POST"))
	{
		//�����������һ������ҳ��
		//todo
		unimplement(client);
	}

	//������Դ�ļ���·��
	//127.0.0.1/adb/test.html
	//GET /abc/test.html HTTP/1.1\n

	char url[255];//����������Դ������·��
	i = 0;
	while (isspace(buff[j]) && j < sizeof(buff))
	{
		j++;
	}
	while (!isspace(buff[j]) && i < sizeof(url) - 1 && j < sizeof(buff))
	{
		url[i++] = buff[j++];
	}
	url[i] = 0;
	PRINTF(url);

	char path[512] = "";
	sprintf_s(path, "htdocs%s", url);
	if (path[strlen(path) - 1] == '/')
	{
		strcat_s(path, "index.html");
	}
	PRINTF(path);

	struct stat status;
	if (stat(path, &status) == -1)
	{
		while (numchars > 0 && strcmp(buff, "\n"))
		{
			numchars = get_line(client, buff, sizeof(buff));
		}
		not_found(client);
	}
	else {
		if ((status.st_mode & S_IFMT) == S_IFDIR)
		{
			strcat_s(path, "/index.html");
		}
		server_file(client,path);
	}

	closesocket(client);

	return 0;
}

int main()
{
	unsigned short port = 8000;
	int server_socket = startup(&port);
	printf("http�����Ѿ����������ڼ��� %d �˿ڡ�",port);

	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
	// to do
	while (1)
	{
		//����ʽ�ȴ��û�ͨ��������������
		int client_sock = accept(server_socket,(struct sockaddr*) & client_addr, &client_addr_len);
		if (client_sock == -1)
		{
			error_die("accept");
		}
		
		//ʹ��client_sock ���û����з���
		//to do...
		//ʹ���߳�

		//����һ���µ��߳�
		printf("Start a new Thread!\n");
		DWORD threadId = 0;
		CreateThread(0, 0, accept_request,
			(void*)client_sock,
			0, &threadId);
	}
	// "/" ��վ��������ԴĿ¼�µ�index.html

	closesocket(server_socket);
	//system("pause");
	return 0;
}
