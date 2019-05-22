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
//g++ -o x ./flock.cpp -ggdb3


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





本文主要讲述: flock() 对文件加锁的使用.

定义函数 int flock(int fd,int operation);
参数 operation有下列四种情况:
	LOCK_SH 建立共享锁定. 多个进程可同时对同一个文件作共享锁定.
	LOCK_EX 建立互斥锁定. 一个文件同时只有一个互斥锁定.
	LOCK_UN 解除文件锁定状态.
	LOCK_NB 无法建立锁定时, 此操作可不被阻断, 马上返回进程.
					通常与LOCK_SH或LOCK_EX 做OR(|)组合.
返回0表示成功, 若有错误则返回-1, 错误代码存于errno.

单一文件‘无法同时建立’ 共享锁定 和 互斥锁定,
而当使用dup()或fork()时, ‘文件描述符’不会继承此种锁定.
即:
	不同的'文件描述符', 虽然可以加锁, 但是实际还是要等待, 阻塞,
	直到获取到使用权为止.
	一个文件描述符, 可以加一次锁, 排队由内核决定.
	但一个文件描述符不应该加锁两次, 这是程序员的低级错误.
	哪个进程的哪个描述符加锁,必须由这个进程,这个描述符自己解锁,别人没办法解锁.

	(阻塞用法)
	进程调用flock()尝试锁文件时,
	如果文件已经被其他进程锁住, 进程会被阻塞直到锁被释放掉;
	(非阻塞用法)
	在调用flock()的时候, 如果添加LOCK_NB参数,
	则: 在尝试锁住该文件的时候, 发现已经被其他服务锁住, 会返回错误.

	(释放锁)
	flock锁的释放非常具有特色, 调用flock(fd,LOCK_UN)参数即可释放文件锁,
	也可以通过‘关闭fd的方式’来释放文件锁,
	意味着flock会随着进程的关闭而被自动释放掉.


 * 如果像对.exe 可执行程序elf 文件加互斥锁,
 * 防止.exe 打开两个程序, 可以用flock()

*/


#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>


//1.子进程执行的测试函数
void test1(void){
	FILE *fs;
	int fd,tmp;



	//1.以FILE* 文件流的方式, 打开文件
	fs = fopen("temp", "w+");
	if(fs == NULL){
		printf("test1() fopen() fail,errno=%d\n",errno);
		return ;
	}

	//2.将FILE* 文件流, 转化为fd 文件描述符
	fd = fileno(fs);

	//3.尝试进行加锁操作
	//tmp = flock(fd, LOCK_EX | LOCK_NB);//加文件-互斥锁-异步
	tmp = flock(fd, LOCK_EX );//加文件-互斥锁-同步
	if(tmp == 0){
		printf("test1() flock()->LOCK_EX okay\n");
		getchar();
		sleep(1);
		flock(fd, LOCK_UN);//解锁
	}
	else
		//失败原因可能是: 文件已经加锁
		printf("test1() flock()->LOCK_EX failed, errno = %d\n",errno);

	fclose(fs);//关闭fd 会自动释放flock()
	return ;
}







//2.父进程执行的测试函数
void test2(void){
	FILE *fs;
	char text[]="this is a test!";
	int fd,tmp;



	//1.以FILE* 文件流的方式, 打开文件
	fs = fopen("temp", "w+");
	if(fs == NULL){
		printf("test2() fopen() fail,errno=%d\n",errno);
		return ;
	}

	//2.将FILE* 文件流, 转化为fd 文件描述符
	fd = fileno(fs);

	//3.尝试进行加锁操作
	//LOCK_NB = nonblocking 操作, 非阻塞操作, 是联用原子操作
	//LOCK_SH 文件-共享锁, LOCK_EX 文件-互斥锁 都可以用LOCK_NB.
	//如果不用LOCK_NB, 则会陷入阻塞, 直至获取到使用权, 然后锁住文件
	//tmp = flock(fd, LOCK_SH | LOCK_NB);//加文件-共享锁-异步
	tmp = flock(fd, LOCK_SH);//加文件-共享锁-同步
	if(tmp == 0){
		printf("test2() flock()->LOCK_SH okay\n");
		fwrite(text, strlen(text), 1, fs);
		sleep(1);
		flock(fd, LOCK_UN);//解锁
	}
	else{
		//失败原因可能是: 文件已经加锁
		printf("test2() flock()->LOCK_SH failed, errno = %d\n",errno);
		printf("test2() tmp=%d\n", tmp);
		flock(fd, LOCK_UN);//强行解其它进程的锁-测试
		fwrite(text, strlen(text), 1, fs);
	}

	fclose(fs);//关闭fd 会自动释放flock()

	return ;
}



int main(void){
	int tmp;

	tmp = fork();
	if(tmp == -1){
		printf("fork() fail, errno = %d\n", errno);
		return -1;
	}
	if(tmp == 0){
		//1.子进程执行的测试函数
		test1();
	}


	//2.父进程执行的测试函数
	test2();

	return 0;
}
