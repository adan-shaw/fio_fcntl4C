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
//g++ -o x ./io_basic.cpp

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h> // for 原子操作O_RDONLY 之类的
#include <unistd.h>



//write()/read() 与send()/recv() 的区别--关键1: socket缓冲区
/*
	每个socket被创建后, 都会分配两个缓冲区, 输入缓冲区和输出缓冲区.
	write()/send() 并不立即向网络中传输数据, 而是先将数据写入缓冲区中,
	再由TCP协议将数据从缓冲区发送到目标机器.
	一旦将数据写入到缓冲区, 函数就可以成功返回,
	不管它们有没有到达目标机器, 也不管它们何时被发送到网络,
	这些都是TCP协议负责的事情.

	TCP协议独立于write()/send() 函数,
	数据有可能刚被写入缓冲区就发送到网络,
	也可能在缓冲区中不断积压,
	多次写入的数据被一次性发送到网络,
	这取决于当时的网络情况、当前线程是否空闲等诸多因素, 不由程序员控制.

	read()/recv() 函数也是如此, 也从输入缓冲区中读取数据,
	而不是直接从网络中读取.



	这些I/O缓冲区特性可整理如下：
		1.I/O缓冲区在每个TCP套接字中单独存在;
		2.I/O缓冲区在创建套接字时自动生成;
		3.即使关闭套接字也会'继续传送输出缓冲区'中遗留的数据;
		4.关闭套接字将'丢失输入缓冲区'中的数据.

	输入输出缓冲区的默认大小一般都是8K,可以通过 getsockopt()函数获取.


	如果write()写入的数据大于socket缓冲区,则:
	TCP:
		1,阻塞fd将会阻塞直到可以发出,
			如果超过MTU将会分片, 这些你都不需要关心, 你发多长的数据都行,
			不会因为数据长而导致致命的错误, 会自动分片.
		2,非阻塞fd将会返回-1,errno=EAGAIN=再次尝试可能会成功,网卡可能会空闲
	UDP:
		1,阻塞fd, 如果网卡满会阻塞, 如果超过UDP最大包限制将会出错返回-1.
		2,非阻塞fd, 网卡满返回-1,errno=EAGAIN. 超过UDP最大包限制一样
	另外:
		由于读写缓冲区, 是分立的,
		且默认一个socket 同一时间, 只能有一个io 操作在进行,
		所以, 理论上可以‘默认单次最大’写入8kb 数据,
		(但是如果写入时, 网卡未空闲, 上一次数据未发出去, 下一波数据又来了
			这种情况是需要程序员极力避免的'资源竞争冲突')
	粘包:
		epoll 有两种模式, ET模式
		ET模式:
			仅当状态发生变化的时候才获得通知,
			这里所谓的状态的变化并不包括缓冲区中还有未处理的数据,
			也就是说,如果要采用ET模式,需要一直read/write直到出错为止,
			很多人反映为什么采用ET模式只接收了一部分数据就再也得不到通知了;
		LT模式:
			是只要有数据没有处理就会一直通知下去的.

		但无论那种模式, 都可能出发'粘包问题';
		例如:
			用epoll监听read事件, 第一次收到epoll read通知, 再去读数据,
			数据未读完, 第二波数据又来了.
			通知仍然不断拥入, 但此时, 你还未退出读取循环.
			就会连第二波的数据, 也读进了装载第一次read 的缓冲区中.
		粘包解决办法1:
			固定每一次数据缓冲区的长度, 补足则填充.
		粘包解决办法2:(这个方法可行)
			分两次发送数据,
			第一次是固定的数据信息元, 格式, 长度固定.
			第二次是数据体, 长度由第一次固定信息元提供.
			传输保证, by来自tcp 协议对数据安全完整传输的可靠性!

*/
//通用io 的write()/read() 与send()/recv() 的区别.
void io_socket(void){
	int fd_srv,fd_cli,tmp;





}








//*************************
//带权限操作的fio 文件打开方式
//*************************
//O_RDONLY = 只读
//O_WRONLY = 只写
//O_RDWR = 读写独占
//O_APPEND = 追加
//O_TRUNC = 截断模式-若文件存在,则长度被截为0,属性不变(即清空文件内容)
//O_CREAT = 不存在则创建
//
//fopen() mode 与 open() flags 关系对照表
//			r					O_RDONLY
//			w					O_WRONLY | O_CREAT | O_TRUNC
//			a					O_WRONLY | O_CREAT | O_APPEND
//			r+				O_RDWR
//			w+				O_RDWR | O_CREAT | O_TRUNC
//			a+				O_RDWR | O_CREAT | O_APPEND
//
//0777 = 最高权限打开该文件
//0666 = 读写权限
//0555 = 读, 执行
//0444 = 只读取
//0222 = 只写
//
//在POSIX 系统, 包含Linux下都会忽略b 字符.
//(也就是遵循posix的系统,都不允许b 二进制读取文件,只允许t 文本式读取文件)
//
//由fopen()所建立的新文件会具有:
//	S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH(0666)权限
//此文件权限也会参考umask 值


//fdopen()mode 字符串则代表着文件指针的流形态,
//此形态必须和原先open()文件时, 描述词的'读写模式'相同.


//2.普通file 直写型io(但实际也不是真的立刻直写,还要看内核调度,只是尽快直写)
//	内核对write()/read() 仍然有一定的缓冲调度机制, 但是这个机制是不可控的.
//	fio 是程序员可控的io流机制.
//	(可以避免频繁调用write()/read() api, 这两个api 速度很慢!!
//		相比fio 的api, write/read 实在太慢了, 可能涉及一些磁盘信息获取操作.)
void io_basic(void){
	int fd,tmp;
	char buf[32];



	//1.open() 打开文件-直写无缓冲
	fd = open("./test_data", \
			O_RDWR | O_CREAT | O_APPEND, 0600);
	if(fd == -1){
		printf("open(\"./test_data\") fail! errno = %d\n",errno);
		return ;
	}

	//2.write() 直写文件
	strncpy(buf,"adan shaw",32);
	tmp = write(fd,buf,strlen(buf)+1);
	printf("write() len = %d\n",tmp);//写入数据的长度

	//3.read() 直读文件
	lseek(fd,10,SEEK_SET);//以追加文件打开,不重置到文件头,只能读取到EOF
	memset(buf,'\0',32);
	tmp = read(fd,buf,31);
	//tmp = read(fd,buf,32);记住要保留多一字节,用来存放'\0',否则乱码
	printf("read() len=%d, %s\n",tmp,buf);

	//4.lseek() -- 单位是字节!!
	lseek(fd,10,SEEK_CUR);//文件游标的当前pos, 向后偏移10 字节
	lseek(fd,10,SEEK_SET);//文件游标重置到文件头, 向后偏移10 字节
	lseek(fd,10,SEEK_END);//文件游标重置到文件尾, 向前偏移10 字节

	//5.lseek() + io 的原子操作
	//写
	tmp = pwrite(fd,"1234567890",11,SEEK_SET);
	printf("pwrite() len = %d\n",tmp);
	//读
	memset(buf,'\0',32);
	pread(fd,buf,31,(SEEK_SET+10));//从SEEK_SET后移10字节, 再读取31字节
	printf("pread() = %s\n",buf);

	//6.立即截断文件(超出部分清除)
	//	注意, 不能对socket fd 进行截断, 有明文规定
	ftruncate(fd,32);//如果是fd 截断, fd 已经打开, 肯定成功.
	//不一定成功, 权限, 存在不存在等问题, 需要加if 判断
	//truncate("./test_data",32);

	//7.立即执行磁盘同步(这个才是真正的直写)
	fsync(fd);//立刻同步fd 所有数据到磁盘
	fdatasync(fd);//同步fd 差异数据到磁盘
	sync();//立刻刷新内核所有的缓冲块

	//8.close()
	close(fd);

	return;
}





int main(void){
	//1.通用io 的write()/read() 与send()/recv() 的区别.
	io_socket();

	//2.普通file 直写型io(但实际也不是真的立刻直写,还要看内核调度,只是尽快直写)
	io_basic();

	return 0;
}

