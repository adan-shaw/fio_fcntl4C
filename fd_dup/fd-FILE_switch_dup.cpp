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
//g++ -o x fd-FILE_switch_dup.cpp -ggdb


#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h> // for 原子操作O_RDONLY 之类的
#include <unistd.h>
#include <sys/stat.h>//for fchmod()

//带权限操作的fio 文件关闭方式
inline void fcloseEx(FILE* retFs);
//带权限操作的fio 文件打开方式
inline bool fopenEx(FILE** retFs, \
		const char* path, int auth, const char* mode);





//fd to FILE* 测试
void fd2FILE_test(void){
	FILE *pfs;
	int fd;



	printf("fd2FILE_test()\n");

	fd = open("./test_data", \
			O_RDWR | O_CREAT | O_APPEND, 0600);
	if(fd == -1){
		printf("open(\"./test_data\") fail! errno = %d\n",errno);
		return;
	}


	//直接fd to FILE*
	//由于FILE* 不能控制文件的权限, 所以其实一般情况下,
	//用open()控制打开权限, 再用fdopen() 打开fio 是比较适合的做法
	pfs = fdopen(fd,"a+");


	//清空资源
	fclose(pfs);
	close(fd);

	return;
}



//FILE* to fd 测试
void FILE2fd_test(void){
	FILE *pfs_test;
	int tmp;



	printf("FILE2fd_test()\n");

	pfs_test = fopen("./test_data","a+");
	if(!pfs_test){
		printf("fopen(\"./test_data\",\"r\") fail! errno = %d\n",errno);
		return;
	}

	//FILE* to int
	tmp = fileno(pfs_test);

	//直接FILE* to fd
	printf("fileno(pfs_test)=%d\n",tmp);

	//释放资源
	fclose(pfs_test);
	close(tmp);
	return ;
}


//FILE* 拷贝
//(注意: 任何时候, 一个文件在一个进程中, 只需要有一个FILE*,
//			所以基本上, 拷贝FILE* 本身就是愚蠢的做法!!)
//
//	无论哪一种FILE* 拷贝方式, 只要创建多了一个FILE* 实体,
//	那么内核就会分配多一份FILE* 实体的'描述结构'+'fio 缓冲区'??
//	但是你不应该拷贝FILE* !!
//	(多个FILE* 冲突问题尤为严重, 这是一个愚蠢的做法.)
//	况且:
//	在释放资源的时候, 只要其中一个FILE* 调用fclose(),
//	那么目标fd 被释放, 其余的FILE* 再调用fclose() 就会崩溃.
//	不调用fclose() 就会泄漏内存
//
void cpFILE_test(void){
	FILE *pfs_test,*pfs_test2,*pfs_dup,*pfs_dup_err;
	int tmp,tmp2;
	char buf[32];



	printf("cpFILE_test()\n");

	if(!fopenEx(&pfs_test,"./test_data",\
			O_RDWR | O_CREAT | O_APPEND,"a+"))
		printf("cpFILE_test()->fopenEx() fail\n");



	//1.'兜弯子'的方式, 拷贝FILE*.(愚蠢的做法)
	//	ps: 这种方式拷贝的FILE* 基本不能操作!!
	//			fclose() 会崩溃, 内存越界.
	tmp = fileno(pfs_test);//FILE* to int
	tmp2 = dup(tmp);//先cp 一个fd 做备用
	pfs_dup_err = fdopen(tmp2,"a+");//int to FILE*
	//**************************************************


	//2.重新调用fopen()(愚蠢的做法)
	//	你很有可能因为权限问题, 打不开第二个FILE*
	//	如果第一个打开的FILE* 以独占方式打开的话, 你就打不开第二个.
	pfs_dup = fopen("./test_data","a+");


	//3.可以被接受的做法, 两个FILE* 指针, 指向同一个FILE*
	//	但实际pfs_test, pfs_test2 只有一个实体.
	//	两个指针, 一个实体
	pfs_test2 = pfs_test;


	//清空所有FILE* 实体资源.
	//注意: 你多创建一个FILE*, 就需要手动清空一次!!
	//			我的妈呀, 求你别创建多个FILE* 了,
	//			一旦调用fclose() 就会崩溃!!
	if(!pfs_dup_err)
		fclose(pfs_dup_err);
	else
		printf("pfs_dup_err = NULL\n");
	if(!pfs_dup)
		fclose(pfs_dup);
	else
		printf("pfs_dup = NULL\n");
	if(!pfs_test)
		fclose(pfs_test);
	else
		printf("pfs_test = NULL\n");
}


//fd dup
//注意: fd 描述符号的拷贝, 是被允许的,
//			同一进程内可以有多个相同指向的fd,
//			但不应该有多个相同指向的FILE*
void dupfd_test(void){
	//请直接用dup() 系列api 进行fd 拷贝
	int idup,idup1;
	int fd;



	printf("dupfd_test()\n");

	fd = open("./test_data", \
			O_RDWR | O_CREAT | O_APPEND, 0600);
	if(fd == -1){
		printf("open(\"./test_data\") fail! errno = %d\n",errno);
		return;
	}


	//1.dup()
	idup = dup(fd);
	printf("dup(fd)=%d\n",idup);


	//2.dup2()
	idup1 = dup2(fd,555);//默认fd max=1024,可修改linux 内核配置增大范围
	printf("dup2(fd,9213)=%d\n",idup1);


	//清空所有fd, 不清空fd 也是内存泄漏的一种
	close(fd);
	close(idup);
	close(idup1);
	return;
}



int main(void){
	//1.fd to FILE* 测试
	fd2FILE_test();
	printf("\n\n");

	//2.FILE* to fd 测试
	FILE2fd_test();
	printf("\n\n");

	//3.FILE* 拷贝(愚蠢的做法)
	cpFILE_test();
	printf("\n\n");

	//4.fd dup
	dupfd_test();
	printf("\n\n");

	return 0;
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
inline bool fopenEx(FILE** retFs, \
		const char* path, int auth, const char* mode){
	//先解决文件权限问题
	int fd = open(path,auth,0600);//读写



	if(fd == -1){
		printf("fopenEx()->open() fail, errno = %d\n",errno);
		return false;
	}

	//修改文件的访问权限, 不让其他人访问(很有用!!)
	fchmod(fd,0600);

	//再创建fio 流-缓冲区
	*retFs = fdopen(fd, mode);
	fd = ferror(*retFs);
	if(fd != 0){
		printf("ferror() = %d\n",fd);
		return false;
	}
	return true;
}

//带权限操作的fio 文件关闭方式
inline void fcloseEx(FILE* retFs){
	//int fd = fileno(retFs);
	fclose(retFs);
	//close(fd);
}
