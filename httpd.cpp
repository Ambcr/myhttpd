#include <stdio.h>
#include<string.h>


#include<sys/types.h>
#include<sys/stat.h>
//网络通信需要包含的头文件、需要加载的库文件
#include <WinSock2.h>
#pragma comment(lib,"WS2_32.lib")
#define PRINTF(str) printf("[%s - %d]"#str"=%s\n", __func__, __LINE__, str);



void error_die(const char* str) {
	perror(str);
	exit(1);
}

//实现网络的初始化
//返回值：套接字
//参数：端口
int startup(unsigned short* port)
{
	//网络通信的初始化
	WSADATA data;
	int ret = WSAStartup(MAKEWORD(1, 1), &data); //1.1版本的协议
	if (ret)
	{
		return -1;
	}
	//创建套接字
	int server_socket = socket(PF_INET,//套接字的类型
		SOCK_STREAM,//数据流
		IPPROTO_TCP);
	if (server_socket == -1) {
		//打印错误信息
		error_die("WSAStartup");
	}
	//设置端口可复用
	int opt = 1;
	ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt,
		sizeof(opt));
	if (ret == -1) {
		error_die("set socket opt");
	}

	//配置服务器端的网络地址
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(*port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//绑定套接字
	if (0 > bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)))
	{
		error_die("bind");
	}
	//动态分配端口
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

	//创建监听队列
	if (listen(server_socket, 5) < 0)
	{
		error_die("listen");
	}

	return server_socket;
}

//从指定的客户端套接字sock，读取一行数据，保存到buff中
int get_line(int sock, char* buff, int size)
{
	char c = 0;//'\0'
	int index = 0;
	// \r \n
while (index < size - 1 && c != '\n')
{
	int n = recv(sock, &c, 1, 0);//返回读了几个字节,
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
buff[index] = 0; //'0' 结束符
return index;
}
void unimplement(int client)
{
	//向指定的套接字，发送一个错误提示页面

}
void not_found(int client)
{
	//to do

}
void headers(int client)
{
	//发送响应包的头信息
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
	printf("一共发送[%d]字节给浏览器\n", count);


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
		//正式发送资源给浏览器
		headers(client);
		//发送请求的资源信息
		cat(client, resource);
		PRINTF("资源发送完毕\n");
	}
	if (resource != NULL) fclose(resource);

}

DWORD WINAPI accept_request(LPVOID arg)
{
	int client = (SOCKET)arg;
	char buff[1024];
	//读取一行数据
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
	//检查请求的方法，本服务器是否支持
	if (_stricmp(method, "GET") && _stricmp(method, "POST"))
	{
		//向浏览器返回一个错误页面
		//todo
		unimplement(client);
	}

	//解析资源文件的路径
	//127.0.0.1/adb/test.html
	//GET /abc/test.html HTTP/1.1\n

	char url[255];//存放请求的资源的完整路径
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
	printf("http服务已经启动，正在监听 %d 端口。",port);

	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
	// to do
	while (1)
	{
		//阻塞式等待用户通过浏览器发起访问
		int client_sock = accept(server_socket,(struct sockaddr*) & client_addr, &client_addr_len);
		if (client_sock == -1)
		{
			error_die("accept");
		}
		
		//使用client_sock 对用户进行访问
		//to do...
		//使用线程

		//创建一个新的线程
		printf("Start a new Thread!\n");
		DWORD threadId = 0;
		CreateThread(0, 0, accept_request,
			(void*)client_sock,
			0, &threadId);
	}
	// "/" 网站服务器资源目录下的index.html

	closesocket(server_socket);
	//system("pause");
	return 0;
}
