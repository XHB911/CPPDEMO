#ifndef __ABOO_CHATSERVER_H__
#define __ABOO_CHATSERVER_H__

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

using namespace muduo;
using namespace muduo::net;

class ChatServer {
public:
    ChatServer(EventLoop* loop, const InetAddress& listenAddr, const string& nameArg);
    void start();
private:
    void onConnection(const TcpConnectionPtr&);
    void onMessage(const TcpConnectionPtr&, Buffer*, Timestamp);

    TcpServer m_server;
    EventLoop* m_looop;
};

#endif