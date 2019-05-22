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
//g++ -o x ./dir_basic.cpp -ggdb3


#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h> // for 原子操作O_RDONLY 之类的
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>


//遍历某个目录下的所有文件-线程安全版本
void listFiles_r(const char *dirpath);

//遍历某个目录下的所有文件
//因为内部使用了静态数据, 所以readdir被认为不是线程安全的函数
void listFiles(const char *dirpath);



int main(void){
	int tmp;



	//1.重命名-文件
	tmp = rename("./x","./x_new");
	if(tmp == -1)
		printf("rename() fail, errno = %d\n", errno);

	//2.创建link(目标, ‘生成的link 存放位置’和名称)
	tmp = link("./test_data","./link2test_data");
	if(tmp == -1)
		printf("link() fail, errno = %d\n", errno);

	//3.移除link
	tmp = unlink("./link2test_data");
	if(tmp == -1)
		printf("unlink() fail, errno = %d\n", errno);

	//4.创建文件夹
	tmp = mkdir("./.myMkdir",0600);
	if(tmp == -1)
		printf("mkdir() fail, errno = %d\n", errno);

	//5.移除文件夹
	tmp = rmdir("./.myMkdir");
	if(tmp == -1)
		printf("rmdir() fail, errno = %d\n", errno);

	//6.删除-文件
	tmp = remove("./x_new");
	if(tmp == -1){
		printf("remove() fail, errno = %d\n", errno);
		return -1;
	}

	//7.读取目录(均以getdents() 为基础, 但为纳入SUSv3)
	//读取目录流DIR*
	//	DIR* opendir(char* path)
	//	DIR* fdopendir(int fd)

	//解析目录流DIR*(只保留demo function)
	printf("遍历某个目录下的所有文件-线程安全版本:\n");
	listFiles_r(".");
	printf("\n\n");

	printf("遍历某个目录下的所有文件:\n");
	listFiles(".");
	printf("\n\n");

	//重置DIR* 目录流的pos 游标(readdir() 每次读取, 都会移动一次目录流pos游标)
	//void rewinddir(DIR*)

	//void closedir(DIR*)//关闭目录流

	//8.遍历文件树	passed()


	//9.1: 获取进程当前的工作目录
	char cwdbuf[4096];
	getcwd(cwdbuf,4096);
	printf("进程当前的工作目录:\n%s\n\n",cwdbuf);
	//9.2: 更改进程当前的工作目录
	chdir(".");
	//fchdir()
	//10.解析路径名
	//9.3：更改进程根目录
	chroot(".");


	return 0;
}







#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>//for readdir()
#include <stddef.h>
#include <unistd.h>

//遍历某个目录下的所有文件-线程安全版本
void listFiles_r(const char *dirpath){
	DIR *dirp;
	struct dirent *result, *entryp;
	int nameMax;



	//获取路径的特定配置信息
	nameMax = pathconf(dirpath, _PC_NAME_MAX);
	if(nameMax == -1)
		nameMax = 255;

	//分配所有struct dirent 所需的内存
	entryp = (struct dirent *)malloc(\
			offsetof(struct dirent, d_name) + nameMax + 1);
	if(entryp == NULL){
		printf("malloc() fail,errno=%d\n",errno);
		return;
	}

	//获取目录流DIR*
	dirp = opendir(dirpath);
	if(dirp == NULL) {
		printf("opendir failed on '%s'\n", dirpath);
		free(entryp);
		return;
	}

	//解析目录流DIR*
	for(;;) {
		errno = readdir_r(dirp, entryp, &result);
		if(errno != 0){
			printf("readdir_r() fail, errno=%d\n",errno);
			break;
		}

		if(result == NULL)
			break;

		//跳过目录自身和父目录
		if(strcmp(entryp->d_name, ".") == 0 ||
						strcmp(entryp->d_name, "..") == 0)
				continue;

		//打印目录内的所有信息
		printf("%s\n", entryp->d_name);
	}

	if(closedir(dirp) == -1)
		printf("closedir() fail,errno=%d\n",errno);

	free(entryp);

	return;
}



//遍历某个目录下的所有文件
//因为内部使用了静态数据, 所以readdir被认为不是线程安全的函数
void listFiles(const char *dirpath){
	DIR *dirp;
	struct dirent *dp;



	//获取目录流DIR*
	dirp = opendir(dirpath);
	if(dirp  == NULL){
		printf("opendir failed on '%s'\n", dirpath);
		return;
	}

	//解析目录流DIR*
	for(;;){
		dp = readdir(dirp);
		if(dp == NULL)
			break;

		//跳过目录自身和父目录??
		//if(strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
			//continue;

		printf("%s\n", dp->d_name);//打印目录名

		//给所有文件, 加上固定的前缀
		//(请谨慎操作, linux 文件操作一定程度上很难复原!!)
		/*
		char *target_string = "S01E";//给当前文件夹下所有文件, 加上x前缀
		char buf[256];
		memset(buf,'\0',256);
		strncpy(buf,target_string,256);
		strncat(buf,dp->d_name,sizeof(buf));
		rename(dp->d_name,buf);
		*/
	}

	if(closedir(dirp) == -1)
		printf("closedir() fail,errno=%d\n",errno);
}
