# C4编译器中文版

#### 介绍
C4编译器中文注释改进版，添加中文的注释目的是为了方便新手学习，理解编译器。

#### 软件架构
基于github 开源C4编译器，实现自举功能



#### 使用说明

建议使用gcc 版本：

gcc version 11.2.0 以上 64位欢迎环境下测试

(x86_64-posix-seh-rev1, Built by MinGW-W64 project) 

增加 help 帮助指令

启用命令：
gcc -o c4 c4.c
./c4 hello.c
./c4 -s hello.c

./c4 c4.c hello.c
./c4 c4.c c4.c hello.c


或者

gcc -o c4 c4.c
c4 hello.c
c4 -s hello.c

c4 c4.c hello.c
c4 c4.c c4.c hello.c

错误：如果提示apppath 不存在，怎说明gcc 编译器没有加入环境变量中

#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request


#### 特技

1.  使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md
2.  Gitee 官方博客 [blog.gitee.com](https://blog.gitee.com)
3.  你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解 Gitee 上的优秀开源项目
4.  [GVP](https://gitee.com/gvp) 全称是 Gitee 最有价值开源项目，是综合评定出的优秀开源项目
5.  Gitee 官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6.  Gitee 封面人物是一档用来展示 Gitee 会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)
