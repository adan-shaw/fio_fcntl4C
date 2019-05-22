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
//g++ -o x ./fin_getline.cpp -ggdb3

//getline 可以指定缓冲区, 不指定缓冲区, 默认分配255 大小的缓冲区...
//超出缓冲区截断, 但是仍然会返回截断后读取到的字符串, 和移动光标.
//跟fgets() 差不多, 只是会报告一行是否读完.
//fgets() 不知道一行是否读完, fgets() 同样不会触发cpu 溢出寄存器.



#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <string.h>
using namespace std;

#define BASE_BUFF_MAX_LEN 10
//测试字符串一行未读完, getline() 和c++ 流可以收到通知.
//也可以清楚流错误.(或者你用c 获取流错误, 应该也能获取相关信息)
void test_str_line_overflow(void);

int main(void){
	FILE *fp = fopen("./test_data", "r");
	if(fp == NULL)
		exit(EXIT_FAILURE);

	ssize_t read = 0;
	char *line = NULL;
	size_t len = 0;

	//读一行
	read = getline(&line, &len, fp);
	printf("fight line:%s",line);
	printf("\n\n");

	//将整个文件按照'行读'的方式, 逐行读取&&打印
	while((read = getline(&line, &len, fp)) != -1){
		printf("Retrieved line of length %zu :\n", read);
		printf("%s", line);
	}

	//getline 返回的是malloc 分配的字符串, 需要手动释放
	//但是只需要释放一次, 缓冲区会自动重复利用
	free(line);

	fclose(fp);

	printf("\n\n\n<< test_str_line_overflow start >>\n\n");
	//测试字符串一行未读完, getline() 和c++ 流可以收到通知.
	test_str_line_overflow();
	exit(EXIT_SUCCESS);
}



//测试字符串一行未读完, getline() 和c++ 流可以收到通知.
void test_str_line_overflow(void){
	char *filePath = "./test_data";
	int bufLen = BASE_BUFF_MAX_LEN;
	char *buf = new char[bufLen];

	ifstream fin;
	fin.open(filePath,ios::in);
	fin.getline(buf,bufLen);
	if(fin.fail())
	//while(fin.fail())
	{
		cout<<buf<<endl;
		fin.clear(ios::goodbit);
		fin.getline(buf,bufLen);
		cout<<buf<<endl;
	}
	//cout<<buf<<endl;
	return;
}
