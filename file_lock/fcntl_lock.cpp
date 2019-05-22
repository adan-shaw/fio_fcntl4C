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
//g++ -o x ./fcntl_lock.cpp -ggdb3

//file lock 文件锁基础理论:
//声明: fs = FILE*
/*
文件锁的类型, 大概有两种:
	*1.劝戒锁(进程内可以多次加锁, 多次释放锁, 但多进程间不能仍然共享, 线程可共享)
		fcntl()			行-劝戒锁
		flockfile()	整个文件-劝戒锁
		什么是劝戒锁？(也叫建议锁)
			劝戒锁就是, 你询问可不可以加锁, 内核会给你一个建议.
			你可以遵守这个建议, 选择操作, 或者不操作.
			你也可以不遵守这个建议, 不管冲突, 直接操作, 直接写盘.
			但是冲突产生的数据误差, 你自己负责.
	*2.强制锁(线程也不能共享, 一次加锁, 一次释放, 十分严格)
		flock()	整个文件-强制锁
		什么是强制锁?
			强制锁就是独占锁;在释放锁之前,其它进程/线程都不能使用,与互斥锁类似

文件锁api:
	1.用fcntl()对文件加锁,可以对文件的部分内容行加锁,这里称之为'行锁';
		fcntl() 加锁时, 需要输入起始位置, 终止位置.
		如果对整个文件进行加锁, 你还要获取整个文件的长度;
		起始位置=0, 终止位置=文件结尾处.

	2.用flock()对文件加锁,无论文件实体对应有多少个fs 文件描述符派生,
		统一锁死整个文件使用权, 给当前调用者fs, 是一种独占锁.

	3.用flockfile()对文件加锁,实际只能锁住该'fs 文件描述符'的使用权,
		如果该文件有其他'fs 文件描述符', 就不适合使用flockfile(),
		否则会有竞争冲突!!(race condition).
		但真实应用场景中, 一个文件也不应该有2个fs 文件描述,
		尤其是同一个进程内, 更不应该有2个fs 文件描述,
		有2个fs 文件描述, 一般是多进程共享一个文件的情况, 应该上强制互斥锁.
		注意:
		3.1：
			flockfile()加锁, 只是会在使用者lockcount中增加1;
			funlockfile()释放锁, 只是会在使用者lockcount中减少1;
			ftrylockfile()则是nonblocking版的flockfile(),
				询问内核加锁建议.
		3.2：
			内核会为每个flockfile()锁维护一些支撑数据,
			就算funlockfile() 看上去没啥作用, 但是用完锁一定要释放!!
			否则内存可能会出现膨胀不止!!内核维护flockfile()锁的内存不断膨胀
		3.3:
			flockfile()锁, 是锁‘整个文件’的.
			如果单次读写的数据长度不大, 那么可共享性就很强了,
			如果单次读写的时间过长, flockfile()锁也会阻塞很久.
			flockfile()锁的数据安全, 由程序员保证.
			不遵守加锁建议, 强行写盘/读盘, 导致的后果由程序员自己承担.(未知风险)
			所以,既然有必要‘要加flockfile()锁’的话, 就要遵守建议规则.
		3.4:
			flockfile() 锁的用途:
			flockfile() 锁实际适合'多线程'共享"读和写"的场合.
			共同写, 共同读, 在单一进程内, 多线程的情况, 都不需要加锁.

			共同写有顺序差别, 不知道哪个线程先, 哪个线程后,
			但是加锁也不能解决这个问题, 如果是'多线程共写', 不会读写冲突就行了.

			共读, 更简单, 没有写操作的话, 怎样读都行.



本文主要讲述fcntl() 函数的使用:
//int fcntl(int fd, int cmd, ... );
fcntl() 功能简单目录:
*1.复制一个现有的描述符		(cmd = F_DUPFD)
*2.获得／设置文件描述符标记(cmd = F_GETFD / F_SETFD)
*3.获得／设置文件状态标记	(cmd = F_GETFL / F_SETFL)
*4.获得／设置异步I/O所有权(cmd = F_GETOWN / F_SETOWN)
*5.获得／设置记录锁			(cmd = F_GETLK / F_SETLK / F_SETLKW)

F_DUPFD 返回一个如下描述的(文件)描述符:
(1)最小的大于或等于arg的一个可用的描述符
(2)与原始操作符一样的某对象的引用
(3)如果对象是文件(file)的话,返回一个新的描述符,
	 这个描述符与arg共享相同的偏移量(offset)
(4)相同的访问模式(读,写或读/写)
(5)相同的文件状态标志(如:两个文件描述符共享相同的状态标志)
(6)与新的文件描述符结合在一起的close-on-exec标志,
	 被设置成交叉式访问execve(2)的系统调用


************************************************************
关于close-on-exec标志声明:
fork()不是exec()!! fork()是写时cp,写时dup.
言外之意就是,如果你不写,fd 永远都一直只有一份!!无需要close-on-exec.
fork()之后无需操作任何fd,你操作了反而会dup多一份,操作就要自己手动关闭.

close-on-exec 标志是给exec() 新建全新进程系列函数使用的,
如果创建一个全新的函数肯定会涉及<资源争夺>, 关闭了比较好.
但是fork(), 你不去动fd 资源, 根本什么事情都不会发生, 一点资源争夺都没有.

这个操作少用.!! 并不是你设置错误
fcntl(fd_dup, F_SETFD, true);//启动close-on-exec
fcntl(fd_dup, F_SETFD, false);//关闭close-on-exec
F_GETFD 标志更少用


F_GETFD 取得与文件描述符fd联合close-on-exec标志, 类似FD_CLOEXEC.
				如果返回值和FD_CLOEXEC进行与运算结果是0的话,
				文件保持交叉式访问exec(),否则如果通过exec()运行的话,
				文件将被关闭(arg被忽略)

F_SETFD 设置close-on-exec旗标.
				该旗标以参数arg的FD_CLOEXEC位决定.
************************************************************


F_GETFL 取得fd的文件状态标志,如同下面的描述一样(arg被忽略)

F_SETFL 设置给arg描述符状态标志,可以更改的几个标志是：
				O_APPEND, O_NONBLOCK, O_SYNC和O_ASYNC.


**异步所有权** (不知道如何操作)
F_GETOWN 取得当前正在接收SIGIO或者SIGURG信号的进程id或进程组id,
				 进程组id返回成负值(arg被忽略)

F_SETOWN 设置将接收SIGIO和SIGURG信号的进程id或进程组id,
				 进程组id通过提供负值的arg来说明,否则,arg将被认为是进程id


*/



/*

结构体flock的指针(仅fcntl 有效)：
struct flcok{
	short int l_type; // 锁定的状态

	//这三个参数用于分段对文件加锁, 若对整个文件加锁, 则:
	//l_whence=SEEK_SET,l_start=0,l_len=0;

	short int l_whence;//决定l_start位置
	off_t l_start; //锁定区域的开头位置
	off_t l_len; //锁定区域的大小
	pid_t l_pid; //锁定动作的进程
};

l_type 有三种状态:
F_RDLCK 建立一个供读取用的锁定
F_WRLCK 建立一个供写入用的锁定
F_UNLCK 删除之前建立的锁定

l_whence 也有三种方式:
SEEK_SET 以文件开头为锁定的起始位置.
SEEK_CUR 以目前文件读写位置为锁定的起始位置
SEEK_END 以文件结尾为锁定的起始位置.


//fcntl()参数使用说明:
//start = lseek() 开始, 往后len bit 字节的文件内存, 全部上锁.
//whence = 起始相对位, 和lseek() 一样.
//若对整个文件加锁, 则:
//l_whence=SEEK_SET,l_start=0,l_len=0;


//加锁, 就是对文件的一部分内容进行加锁, 防止这部分内容被修改
//这也是一种: 行锁, 或者叫区域锁.
//至于锁整个文件的那种, 可以用flock() 或者flockfile().


 * 如果像对.exe 可执行程序elf 文件加互斥锁,
 * 防止.exe 打开两个程序, 应该用flock()

*/





#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h> // for fcntl()
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

//***********************
// 设置一把读锁
bool readLock(int fd, short start, short whence, short len);
// 设置一把读锁, 不等待
bool readLocknb(int fd, short start, short whence, short len);
// 设置一把写锁
bool writeLock(int fd, short start, short whence, short len);
// 设置一把写锁, 不等待
bool writeLocknb(int fd, short start, short whence, short len);
// 解锁, 统一解锁方式
bool unlock(int fd, short start, short whence, short len);
//***********************





//主测试函数
int main(){
	int fd,fd_dup,tmp;
	int son_ret,son_pid;//for 失败测试
	int flags;//for fcntl() F_GETFL or F_SETFL 测试
	FILE* pfs;//for fcntl() 锁测试
	int pos;
	int son_tmp;



	//1.打印进程pid
	printf("main pid = %d\n", getpid());

	//2.打开测试文件
	fd = open("test.txt", O_RDWR | O_CREAT | O_APPEND, 0600);
	if(fd == -1){
		printf("open() fail, errno = %d\n", errno);
		return -1;
	}
	else
		printf("fd = %d\n", fd);

	//3.cp 一个fd做备份
	fd_dup = dup(fd);
	printf("dup() backup fd = %d\n",fd_dup);


	//4.测试fcntl F_DUPFD 成功
	tmp = fcntl(fd_dup, F_DUPFD, 0);//等价于dup()
	printf("fcntl() F_DUPFD: new fd = %d\n",tmp);
	tmp = close(tmp);
	assert(tmp != -1);//断言关闭fcntl dup fd 成功, 测试fcntl dup 能力



	//**************************************************
	//失败测试！！close-on-exec ==> exec() 之后自动关闭, fork != exec
	printf("****************失败测试开始****************\n");
	//**************************************************

	//5.
	//测试F_DUPFD_CLOEXEC( 在F_DUPFD 的基础上, 多加close-on-exec )
	//同样等价于dup()
	//注意: close-on-exec 之后, 不是<fd_dup> 会在fork() 之后关闭!!(父进程)
	//			是新生的<tmp> 会在fork() 之后关闭(父进程)
	tmp = fcntl(fd_dup, F_DUPFD_CLOEXEC, 0);
	printf("fcntl() F_DUPFD_CLOEXEC: new fd = %d\n",tmp);

	son_ret = -1;
	son_pid = fork();//fork != exec
	if(son_pid == 0){
		printf("i am son, pid = %d\n", getpid());
		//子进程close() 新dup 的fd
		tmp = close(tmp);
		assert(tmp != -1);//断言子进程关闭fcntl dup fd 成功
		return 0;
	}
	else{
		printf("son pid = %d, wait() son quit.\n", son_pid);
		wait(&son_ret);
		assert(son_ret == 0);//断言子进程正常结束
		tmp = close(tmp);
		//assert(tmp == -1);//断言F_DUPFD_CLOEXEC 成功
	}
	//ps: close-on-exec 真难用, 唉. 自己手动关闭比较靠谱
	close(tmp);//父亲进程手动关闭dup fd tmp...
	printf("****************失败测试结束****************\n");



	//6.
	//F_GETFL or F_SETFL
	//F_SETFL 设置给arg描述符状态标志,可以更改的几个标志是：
	//O_APPEND, O_NONBLOCK, O_SYNC和O_ASYNC.
	//?? O_NONBLOCK 和 O_ASYNC ?? 有什么区别？？
	//*********************************
	//设置fd_dup 为非阻塞fd
	flags = -1;
	flags = fcntl(fd_dup, F_GETFL, 0);
	if(flags == -1){
		printf("fcntl() fail, errno = %d\n", errno);
		return -1;
	}

	flags |= O_NONBLOCK;//fcntl()标记 或运算, 表示多加O_NONBLOCK
	tmp = fcntl(fd_dup, F_SETFL, flags);
	if(tmp == -1){
		printf("fcntl() fail, errno = %d\n", errno);
		return -1;
	}
	//*********************************

	//*********************************
	//取消fd_dup 非阻塞
	flags &= O_NONBLOCK;//fcntl()标记 与运算, 表示取消O_NONBLOCK
	tmp = fcntl(fd_dup, F_SETFL, flags);
	if(tmp == -1){
		printf("fcntl() fail, errno = %d\n", errno);
		return -1;
	}
	//*********************************
	printf("非阻塞测试okay\n");



	//7.fcntl() 加锁测试:
	printf("\n\nfcntl() 加锁测试start\n");
	//7.1 将fd 转换为FILE* 流
	pfs = fdopen(fd,"a+");
	//准备测试数据
	fputs("sdjhfjkshfsdrngdufgndfugdnfghdfjghdfjghdfjgndjkfgn",pfs);
	fputs("sdjhfjkshfsdrngdufgndfugdnfghdfjghdfjghdfjgndjkfgn",pfs);
	fputs("sdjhfjkshfsdrngdufgndfugdnfghdfjghdfjghdfjgndjkfgn",pfs);
	fputs("sdjhfjkshfsdrngdufgndfugdnfghdfjghdfjghdfjgndjkfgn",pfs);
	fflush(pfs);//确保测试数据写入
	rewind(pfs);//重置到文件头


	//对文件头的10 字节加读锁
	if(!readLock(fd,SEEK_SET,SEEK_END,32))//加读锁-劝戒锁
		printf("father readLock fail()\n");

	if(!readLocknb(fd,SEEK_SET,SEEK_END,32))//加读锁-nonblocking-劝戒锁
		printf("father readLocknb fail()\n");

	unlock(fd,SEEK_SET,SEEK_END,10);//解锁
	unlock(fd,SEEK_SET,SEEK_END,10);//解锁多次解锁2
	unlock(fd,SEEK_SET,SEEK_END,10);//解锁多次解锁3

	if(!writeLock(fd,SEEK_SET,SEEK_END,32))//加写锁-劝戒锁
		printf("father writeLock fail()\n");

	if(!writeLocknb(fd,SEEK_SET,SEEK_END,32))//加写锁-nonblocking-劝戒锁
		printf("father writeLocknb fail()\n");
	//测试加锁的准确性
	tmp = fork();
	if(tmp == -1){
		printf("fork() fail, errno = %d\n", errno);
		return -1;
	}
	if(tmp == 0){
		//子进程, 读取前10 字节测试
		pos = ftell(pfs);
		printf("pos=%d\n",pos);

		if(!readLocknb(fd,SEEK_SET,SEEK_END,32))//加写锁-劝戒锁
			printf("son readLocknb fail()\n");

		if(!writeLocknb(fd,SEEK_SET,SEEK_END,32))//加写锁-nonblocking-劝戒锁
			printf("son writeLocknb fail()\n");

		son_tmp = fgetc(pfs);
		if(tmp == EOF)
			printf("fgetc(): return = EOF = -1\n");
		else
			printf("fgetc(): %c\n",(char)son_tmp);

		//等父进程结束, 再读一次
		usleep(2000);
		son_tmp = fgetc(pfs);
		if(tmp == EOF)
			printf("fgetc() again: return = EOF = -1\n");
		else
			printf("fgetc() again: %c\n",(char)son_tmp);
	}
	usleep(1000);//父进程sleep(1)

	unlock(fd,SEEK_SET,SEEK_END,10);//解锁, 等待子进程测试结束

	usleep(3000);



	return 0;
}











// 设置一把读锁
bool readLock(int fd, short start, short whence, short len){
	struct flock lock;



	//初始化fcntl() 锁属性
	lock.l_type = F_RDLCK;
	lock.l_start = start;
	lock.l_whence = whence;//SEEK_CUR,SEEK_SET,SEEK_END
	lock.l_len = len;
	lock.l_pid = getpid();

	//阻塞方式加锁
	if(fcntl(fd, F_SETLKW, &lock) == 0)
		return true;
	else{
		printf("fcntl() F_RDLCK fail, errno = %d\n", errno);
		return false;
	}
}

// 设置一把读锁, 不等待
bool readLocknb(int fd, short start, short whence, short len){
	struct flock lock;



	//初始化fcntl() 锁属性
	lock.l_type = F_RDLCK;
	lock.l_start = start;
	lock.l_whence = whence;//SEEK_CUR,SEEK_SET,SEEK_END
	lock.l_len = len;
	lock.l_pid = getpid();

	//非阻塞方式加锁
	if(fcntl(fd, F_SETLK, &lock) == 0)
		return true;
	else{
		printf("fcntl() F_RDLCK nonblocking fail, errno = %d\n", errno);
		return false;
	}
}

// 设置一把写锁
bool writeLock(int fd, short start, short whence, short len){
	struct flock lock;



	//初始化fcntl() 锁属性
	lock.l_type = F_WRLCK;
	lock.l_start = start;
	lock.l_whence = whence;
	lock.l_len = len;
	lock.l_pid = getpid();

	//阻塞方式加锁
	if(fcntl(fd, F_SETLKW, &lock) == 0)
		return true;
	else{
		printf("fcntl() F_WRLCK fail, errno = %d\n", errno);
		return false;
	}
}

// 设置一把写锁, 不等待
bool writeLocknb(int fd, short start, short whence, short len){
	struct flock lock;



	//初始化fcntl() 锁属性
	lock.l_type = F_WRLCK;//short int
	lock.l_start = start;//long int
	lock.l_whence = whence;//short int
	lock.l_len = len;//long int
	lock.l_pid = getpid();

	//非阻塞方式加锁
	if(fcntl(fd, F_SETLK, &lock) == 0)
		return true;
	else{
		printf("fcntl() F_WRLCK nonblocking fail, errno = %d\n", errno);
		return false;
	}
}

// 解锁 --统一解锁方式
bool unlock(int fd, short start, short whence, short len){
	struct flock lock;



	//初始化fcntl() 锁属性
	lock.l_type = F_UNLCK;
	lock.l_start = start;
	lock.l_whence = whence;
	lock.l_len = len;
	lock.l_pid = getpid();

	//执行解锁
	if(fcntl(fd, F_SETLKW, &lock) == 0)
		return true;
	else{
		printf("fcntl() fail, errno = %d\n", errno);
		return false;
	}
}











//
//<< 英文版 from man 手册 >>
//
//宏定义for fcntl() -- 操作于fd
//int fcntl(int fd, int cmd, ... /* arg */ );
/*
F_DUPFD - Duplicate file descriptor.
					[复制并返回新的文件描述符,与dup 类似,passed!!]

*FD_CLOEXEC - Close the file descriptor upon execution of
						 an exec family function.
						 [只要fork(),父进程就关闭自己的文件描述符,即close-on-exec]

*F_DUPFD_CLOEXEC - Duplicate file descriptor with
									the close-on-exec flag FD_CLOEXEC set.
									[只要复制之后, 就关闭原文件描述符]

F_GETFD - Get file descriptor flags.
F_SETFD - Set file descriptor flags.
F_GETFL - Get file status flags and file access modes.
F_SETFL - Set file status flags.
F_GETLK - Get record locking information.
F_SETLK - Set record locking information.
F_SETLKW - Set record locking information; wait if blocked.
F_GETOWN - Get process or process group ID to
					 receive SIGURG signals.
F_SETOWN - Set process or process group ID to
					 receive SIGURG signals.

//宏定义for fcntl() 锁
F_RDLCK - Shared or read lock.[读文件锁-可分享, 多个]
F_UNLCK - Unlock.[解文件锁]
F_WRLCK - Exclusive or write lock.[写文件锁-唯一, 可保证进程唯一]

//宏定义for fcntl() seek()
SEEK_SET [文件头]
SEEK_CUR [当前文件位置]
SEEK_END [文件尾]

//宏定义for open() and openat().
O_CLOEXEC	 - The FD_CLOEXEC flag associated with
							the new descriptor shall be set to
							close the file descriptor upon execution of
							an exec family function.
O_CREAT		 - Create file if it does not exist.[当该文件不存在时创建]
O_DIRECTORY - Fail if file is a non-directory file.
O_EXCL			- Exclusive use flag.
							[O_CREAT和O_EXCL同时设置, 当文件已存在时, open()失败,
							保证不破坏已存在的文件]
O_NOCTTY		- Do not assign controlling terminal.
O_NOFOLLOW	- Do not follow symbolic links.
O_TRUNC		 - Truncate flag.
O_TTY_INIT	- Set the termios structure terminal parameters to
							a state that provides conforming behavior;

//宏定义for open(), openat(), and fcntl() -- 异步同步控制
O_APPEND	 - Set append mode.[追加模式]
O_NONBLOCK - Non-blocking mode.[异步模式]
O_DSYNC		- Write according to synchronized I/O data
						 integrity completion.
						 [同步到disk-写]
O_RSYNC		- Synchronized read I/O operations. [同步到disk-读]
O_SYNC		 - Write according to synchronized I/O file
						 integrity completion.
						 [同步读写模式]

//动态访问设备
O_ACCMODE	- Mask for file access modes.

//权限控制
O_EXEC	 - Open for execute only (non-directory files).
					 The result is unspecified if this flag is applied to
					 a directory.
					 [可执行]
O_RDONLY - Open for reading only.[只读]
O_RDWR	 - Open for reading and writing.[读写]
O_SEARCH - Open directory for search only.
					 The result is unspecified if this flag is applied to
					 a non-directory file.
					 [允许可以搜索, 仅文件夹有效]
O_WRONLY - Open for writing only.[只写]


AT_FDCWD - Use the current working directory to
					 determine the target of relative file paths.
					 [以当前目录作为相对路径的参照物, 即'.']
AT_EACCESS - Check access using effective user and group ID.
						 [检查用户对文件的访问权]

AT_SYMLINK_NOFOLLOW - Do not follow symbolic links.
AT_SYMLINK_FOLLOW	 - Follow symbolic link.


//宏定义for posix_fadvise():
POSIX_FADV_DONTNEED	- The application expects that it will not
											 access the specified data in the near future.
POSIX_FADV_NOREUSE	 - The application expects to access
											 the specified data once and then
											 not reuse it thereafter.
POSIX_FADV_NORMAL		- The application has no advice to give on
											 its behavior with respect to
											 the specified data.
											 It is the default characteristic
											 if no advice is given for an open file.
POSIX_FADV_RANDOM		- The application expects to access
											 the specified data in a random order.
POSIX_FADV_SEQUENTIAL- The application expects to access
											 the specified data sequentially from
											 lower offsets to higher offsets.
POSIX_FADV_WILLNEED	- The application expects to access
											 the specified data in the near future.

*/


