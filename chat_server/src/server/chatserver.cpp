#include "chatserver.h"
#include "json.hpp"
#include "chatservice.h"

#include <functional>
#include <string>

using json = nlohmann::json;

ChatServer::ChatServer(EventLoop* loop, const InetAddress& listenAddr, const string& nameArg)
    : m_server(loop, listenAddr, nameArg) {
    m_server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, std::placeholders::_1));
    m_server.setMessageCallback(std::bind(&ChatServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    m_server.setThreadNum(4);
}
void ChatServer::start() {
    m_server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr& conn) {
    if (!conn->connected()) {
        ChatService::instance()->clientCloseExpection(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp tm) {
    string buf = buffer->retrieveAllAsString();
    // 数据的反序列化
    json js = json::parse(buf);
    // 解耦网络模块与业务模块代码
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 消息回调，处理对应的业务
    msgHandler(conn, js, tm);
}