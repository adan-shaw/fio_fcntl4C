//111111111111111111111111111111111111111111111111111111111111111111111111
//格式测试:
//邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵邵
//起始日期:
//完成日期:
//************************************************************************
//修改日志:
//	2019-05-13: 新增'tty 文本标准格式'风格
//, , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , , ,, , ,


//编译:
//g++ -o x ./test_dup_dup2.cpp -ggdb3


/*

这样有两个问题:
1.你要学会关闭多余的fd 文件描述符, 防止无效的fd 文件描述符泄漏,
	因为这些fd 一般都有描述结构体的, 一占内存,
	二:如果fd 不归0,内核是不释放'文件实体'的
	用完不释放fd 比用完不释放内存, 更恶心.
	如果泄漏, 会直到进程结束之后, 才会被释放.


2.shutdown() socket 之后可以视作socket 已经关闭了,
	但fd 描述符可以重复利用.
	shutdown() 主要是socket 的tcp/ip 特性, 协议4次挥手操作.
	socket fd 本身, 是一个类似fio file 之类的缓冲型fd 属性.


3.概念纠正: 0 1 2 是标准stdin stdout stderr, 是对于进程来说的,
	那么, 每个进程的fd 必然都是从3 开始的,
	不同的进程,第一个socket fd 必然也是3.
	每个进程的fd 文件描述符的'编号'都是独立的,
	所以才会有fd_max 限制,
	实际就是限制每个进程打开的fd 文件数量,
	并不是限制系统打开文件的数量, 理论上: 系统可以打开无限个文件.

	当然系统socket 通信的数量, 受port 数量的限制, 自然也是有极限的,
	一个网卡一个2^16 极限限制(65 536)

*/

//int dup(int oldfd);
//int dup2(int oldfd, int newfd);//指定新cp fd 的值

#include <sys/types.h>
#include <sys/socket.h>

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/file.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/ftp.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>


//创建一个1024 的socket fd poor, 
//8kb * 2 + 2kb 左右一个socket 句柄的内存,
//每个socket 句柄dup2 一次,
//一个socket 句柄会有2 个fd 文件句柄描述符号.
int sock_poor_1024[1024];
int sock_poor_1024_cur_pos = 0;


int main(void){
	int fd1,fd2,fd3,fd4,fd5,fd6;
	struct sockaddr_in addr_tmp,addr_tmp2;
	int dup_1,dup_2;
	int tmp;
	int send_size,recv_size;
	char buf[32];
	int addr_len;



	fd1 = socket(AF_INET,SOCK_DGRAM,0);
	if(fd1 == -1)
		printf("socket fail");
	fd2 = socket(AF_INET,SOCK_DGRAM,0);
	if(fd2 == -1)
		printf("socket fail");
	fd3 = socket(AF_INET,SOCK_DGRAM,0);
	if(fd3 == -1)
		printf("socket fail");
	fd4 = socket(AF_INET,SOCK_DGRAM,0);
	if(fd4 == -1)
		printf("socket fail");
	fd5 = socket(AF_INET,SOCK_DGRAM,0);
	if(fd5 == -1)
		printf("socket fail");
	fd6 = socket(AF_INET,SOCK_DGRAM,0);
	if(fd6 == -1)
		printf("socket fail");


	memset(&addr_tmp, 0, sizeof(struct sockaddr_in));
	addr_tmp.sin_addr.s_addr = inet_addr("0.0.0.0");
	addr_tmp.sin_family = AF_INET;
	addr_tmp.sin_port = htons(9213);

	//udp 绑定dup_1
	if(bind(fd6, (struct sockaddr *) &addr_tmp,\
		sizeof(struct sockaddr_in)) == -1)
		printf("bind() fail!! errno: %d\n", errno);

	printf("\
fd1 = %d\n\
fd2 = %d\n\
fd3 = %d\n\
fd4 = %d\n\
fd5 = %d\n\
fd6 = %d\n",fd1,fd2,fd3,fd4,fd5,fd6);





	//1.dup 拷贝fd6, 测试拷贝socket fd.
	dup_1 = dup(fd6);
	printf("dup(): dup_1 = %d, old_fd = fd6 = %d\n", dup_1, fd6);


	//2.dup2 拷贝fd2, 指定拷贝的fd 值 = 100,
	//如果fd = 100 文件描述符已经存在, 则将已打开的资源关闭, 再dup2
	//但是这样做可能有未知bug, 因为fd=100可能被其它进程使用, 这样做不妥
	dup_2 = dup2(fd2, 100);
	printf("dup(): dup_2 = %d, old_fd = fd2 = %d\n", dup_2, fd2);


	//3.关闭原先的fd1 <-> fd6, 证明拷贝后的fd 可用!
	//用dup_1,dup_2 来操作socket,
	//证明：只有当'文件/socket'的'所有描述符指向'数量都为0 的时候,
	//系统才会将'文件/socket'句柄实体close and free
	//
	//注意: 这里的dup_1 拷贝的是fd6, 但是fd6 是bind() -> 9213 的fd,
	//			也就是说fd6 应该只能有一个实体, 但是fd 描述符号却可以有多个.
	//			证明：'文件/socket'实体, 和fd 是不一样的.
	//						'文件/socket'实体只能有一个;
	//						同一个'文件/socket实体', 可以有多个fd 指向它
	tmp = close(fd1);
	assert(tmp != -1);
	tmp = close(fd2);
	assert(tmp != -1);
	tmp = close(fd3);
	assert(tmp != -1);
	tmp = close(fd4);
	assert(tmp != -1);
	tmp = close(fd5);
	assert(tmp != -1);
	tmp = close(fd6);
	assert(tmp != -1);


	//4.1 用udp_2 socket 发送数据到dup_1,
	//		测试dup() 后的socket 仍然能工作.
	send_size = sendto(dup_2, "hello dup_1",\
								sizeof("hello dup_1"), 0, \
								(sockaddr*)&addr_tmp, \
								sizeof(struct sockaddr_in));
	if(send_size == -1)
		printf("sendto() fail, errno: %d\n", errno);
	else
		printf("sendto() ok, data: hello dup_1, send_size = %d\n",\
			send_size);


	//4.2 用udp_1 socket 接收dup_2 的数据
	addr_len = sizeof(struct sockaddr_in);
	recv_size = recvfrom(dup_1, &buf, 32, 0, \
									(sockaddr*)&addr_tmp2, (socklen_t*)&addr_len);
	if(recv_size == -1)
		printf("recvfrom() fail, errno: %d\n", errno);
	else
		printf("recvfrom() ok, data: %s, recv_size = %d\n",\
			buf, recv_size);


	//关闭dup_2
	tmp = close(dup_2);
	assert(tmp != -1);
	//关闭dup_1
	tmp = close(dup_1);
	assert(tmp != -1);


	//关闭dup_1后, 再次绑定dup_1
	if(bind(dup_1, (struct sockaddr *) &addr_tmp,\
		sizeof(struct sockaddr_in)) == -1)
		printf("bind() again fail!! errno: %d\n", errno);


	//再次关闭dup_1 文件描述符号, 触发错误关闭
	tmp = close(dup_1);
	assert(tmp != -1);

	return 0;
}

