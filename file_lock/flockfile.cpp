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
//g++ -o x ./flockfile.cpp -lpthread
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>//for dup(),usleep()
#include <string.h>//for memset()
#include <pthread.h>

//编译:
//g++ -o x ./flockfile.cpp -ggdb3 -lpthread


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





本文主要讲述: flockfile() 对文件加锁的使用.
	flockfile() 系列的api 主要包括:
	void flockfile(FILE *filehandle);//对文件加'安全共享'锁
	int ftrylockfile(FILE *filehandle);//nonblocking 版flockfile()
	void funlockfile(FILE *filehandle);//释放文件加'安全共享'锁

	以上3 个函数都是线程安全的, 放心使用.



本文还涉及: int to FILE* 互转系列函数的使用
	FILE *fdopen(int fd, const char *mode);
	int fileno(FILE *stream);

*/







//1.同一个进程, 多线程的环境下, 测试多线程共享flockfile() 锁
//线程同步全局变量:
pthread_cond_t cond;
pthread_mutex_t mutex;


//	flockfile_test 测试所需的线程函数-前置声明
void* flockfile_test_pthread_fun(void *arg);
//读写测试-前置声明[rw=0=r, rw=1=w]
void rw_test(FILE *pfs,bool rw,bool lock_cond);


void flockfile_test(void){
	//以读写权限打开file
	FILE *pfs_test = fopen("./lock_file_test_data","rw");
	FILE *pfs_dup = fopen("./lock_file_test_data","rw");
	char buf[32];
	int n;
	pthread_t tid;
	//注意, 这种拷贝方式是错误的!!
	//FILE* 不需要拷贝, 而是可以直接再次创建FILE*, 打开同一个文件.
	//**************************************************
	//这是一种错误的dup(FILE*) 方法, 就算拷贝出来的FILE* 也不能用.
	int tmp = fileno(pfs_test);//FILE* to int
	int tmp2 = dup(tmp);//先cp 一个fd 做备用
	FILE *pfs_dup_err = fdopen(tmp2,"rw");//int to FILE*
	//**************************************************
	//初始化全局变量
	pthread_mutex_init(&mutex,NULL);//默认属性初始化
	pthread_cond_init(&cond,NULL);//默认属性初始化




	//1.创建子线程, flockfile() 锁竞争读写测试
	printf("1.创建子线程, flockfile() 锁竞争读写测试.\n\
			at %d\n",time(NULL));
	pthread_create(&tid,NULL,flockfile_test_pthread_fun,pfs_test);
	rw_test(pfs_test,1,0);//读写竞争测试-写
	printf("\n\n\n");


	//2.不顾规则, 不听建议, 强行用锁测试.
	printf("2.不顾规则, 不听建议, 强行用锁测试.\n");
	flockfile(pfs_test);//先加锁
	printf("ftrylockfile(pfs_test) = %d,\n\
			等于0即: 建议=可以使用文件;\n\
			其他值=不建议使用文件\n",ftrylockfile(pfs_test));//遵守建议

	flockfile(pfs_test);
	printf("不遵守协议, 二次加锁, 但不影响程序继续运行\n");

	funlockfile(pfs_test);//解锁

	//3.释放资源
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
	//funlockfile(pfs_test);//错误释放操作!
	fclose(pfs_test);
	fclose(pfs_dup);

	return;
}
//	flockfile_test 测试所需的线程函数-函数体定义
void* flockfile_test_pthread_fun(void *arg){
	FILE *pfs = (FILE*)arg;
	char buf[32];
	char *ptmp = NULL;



	printf("flockfile_test_pthread_fun() start at %d\n",time(NULL));
	rw_test(pfs,0,1);//读写竞争测试-读
	printf("flockfile_test_pthread_fun() finish at %d\n\n\n",time(NULL));

	return 0;
}



//读写测试-函数体定义
//rw=0=r, rw=1=w
void rw_test(FILE *pfs,bool rw,bool lock_cond){
	int n,test_count=10;
	char buf[16];
	struct timespec abstime;



	//等待主进程同步信息
	if(lock_cond){
		abstime.tv_sec = 0;//等待timeout 时间 = 1s
		abstime.tv_nsec = 100000;
		pthread_mutex_lock(&mutex);
		n = pthread_cond_timedwait(&cond, &mutex,&abstime);//等待超时阻塞
		pthread_mutex_unlock(&mutex);
		printf("lock_cond=%d\n",lock_cond);
	}
	else{
		printf("lock_cond=%d\n",lock_cond);
		//pthread_mutex_lock(&mutex);
		pthread_cond_signal(&cond);//通知子线程开始跑, 这里访问不应该加锁
		//pthread_mutex_unlock(&mutex);
	}
	printf("rw=%d\n",rw);
	if(rw){
		//写
		for(n=0;n<test_count;n++){
			//flockfile(pfs);//直接加锁
			if(!ftrylockfile(pfs)){//询问加锁建议
				fputc('*',pfs);
				funlockfile(pfs);
				printf("写___%d-fputc()\n",n);
				usleep(1);
			}
			else{
				printf("写ftrylockfile() wait\n");
				usleep(1);//如果得不到文件锁使用权, 则等待1us 再尝试
			}
		}
	}
	else{
		//读
		for(n=0;n<test_count;n++){
			//flockfile(pfs);//直接加锁
			if(!ftrylockfile(pfs)){//询问加锁建议
				fgets(buf,16,pfs);
				funlockfile(pfs);
				printf("读*%d-fgets(): %s\n",n,buf);
				usleep(1);
			}
			else{
				printf("读ftrylockfile() wait\n");
				usleep(1);
			}
		}
	}
	return;
}





int main(void){
	//1.同一个进程, 多线程的环境下, 测试flockfile() 的有效性
	flockfile_test();

	//2.fork() 多进程环境下, 测试flockfile() 的有效性(passed!!)
	//	由于flockfile()在多进程环境中,使用可能性不高,这里passed不做demo

	return 0;
}
