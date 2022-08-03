# C4编译器中文版

#### 介绍
C4编译器中文注释改进版，添加中文的注释目的是为了方便新手学习，理解编译器。

#### 软件架构
基于github 开源C4编译器，实现自举功能



#### 使用说明

建议使用gcc 版本：

gcc version 11.2.0 以上 64位环境下测试

(x86_64-posix-seh-rev1, Built by MinGW-W64 project) 

增加 help 帮助指令

启用命令：

```
gcc -o c4 c4.c
./c4 hello.c
./c4 -s hello.c

./c4 c4.c hello.c
./c4 c4.c c4.c hello.c

```


或者


```
gcc -o c4 c4.c
c4 hello.c
c4 -s hello.c

c4 c4.c hello.c
c4 c4.c c4.c hello.c

```

错误：如果提示apppath 不存在，怎说明gcc 编译器没有加入环境变量中




### 编译器说明

 c4.c - C 中的四个函数,



> 编译器支持类型：char、int 和 * 指针类型,
> 编译器支持语句：if、while、return 和表达式语句

- readNextToken()   词法解析，读下一个token
- parseStatement()  语法解析，分析句子
- parseExpr(level)  表达式解析，分析运算表达式
- main()  主函数 ，参数判断，



### 代码案例

hello.c
```
#include <stdio.h>

int main()
{
   printf("hello, world\n");
   return 0;
}

```


#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request



