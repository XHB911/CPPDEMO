# CPP_DEMO

学习 C/C++ 开发的一些小 DEMO, 以及抄并改造的开源项目

## DEMO1：

实现多进程copy文件，先用简单方式实现出来，等学完网络知识，再优化并添加多进程下载网络文件功能。

## DEMO2：

多进程、线程的简单TCP网络通信实现

## DEMO3：

多路I/O转接模型：select、epoll、poll

利用UDP进行C/S模型、广播通信

实现本地套接字通信

## DEMO4

epoll反应堆模型的简单实现，利用回调函数进行数据的传输

## DEMO5

线程池的简单实现

## DEMO6

Windows下利用Qt框架写的翻转金币的小游戏

## DEMO7

利用 poll 模型简易的实现了多人聊天的 C-S 模型

## CGIServer

实现简易的CGI服务器，利用进程池、epoll 模型实现

> 参照《Linux 高性能服务器编程》

## Timer

实现简易的定时器，有遍历时间器、时间轮、时间堆的简易实现

> 参照《Linux 高性能服务器编程》

## JSON_parser

JSON 格式的解析器

> [教程源于知乎—————从零开始的 JSON 库教程](https://zhuanlan.zhihu.com/json-tutorial)

## WebServer

实现简易的Web服务器，利用线程池、 epoll 模型实现

> 参照《Linux 高性能服务器编程》

## JSON_parser

JSON 格式的解析器

> [教程源于知乎—————从零开始的 JSON 库教程](https://zhuanlan.zhihu.com/json-tutorial)

## nginx_memory_pool

用 C++、OOP 重新实现 nginx 内存池

## mysql_connection_pool

实现一个 mysql 数据库连接池

## chat_server

集群聊天服务器

UNDO：

- 客户端实现
- 集群实现
- 负载均衡、服务器中间件实现

## muduo

改造的 muduo 库，基于 C++11 实现，脱离 boost 库

还是喜欢造轮子，不喜欢写业务
