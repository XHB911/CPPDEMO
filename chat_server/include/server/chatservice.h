#ifndef __ABOO_CHATSERVICE_H__
#define __ABOO_CHATSERVICE_H__

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>

#include "json.hpp"
#include "usermodel.h"
#include "offlinemessage.h"
#include "friendmodel.h"
#include "groupmodel.h"

using namespace muduo;
using namespace muduo::net;

using json = nlohmann::json;

// 处理消息的回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr& conn, json & js, Timestamp)>;

class ChatService {
public:
    static ChatService* instance();
    // 登录
    void login(const TcpConnectionPtr& conn, json & js, Timestamp tm);
    // 注册
    void reg(const TcpConnectionPtr& conn, json & js, Timestamp tm);
    void reset();
    void oneChat(const TcpConnectionPtr& conn, json & js, Timestamp tm);
    void addFriend(const TcpConnectionPtr& conn, json & js, Timestamp tm);
    void createGroup(const TcpConnectionPtr& conn, json & js, Timestamp tm);
    void addGroup(const TcpConnectionPtr& conn, json & js, Timestamp tm);
    void groupChat(const TcpConnectionPtr& conn, json & js, Timestamp tm);
    // 得到回调函数
    MsgHandler getHandler(int msgid);
    // 处理客户端异常退出
    void clientCloseExpection(const TcpConnectionPtr conn);
private:
    ChatService();
    std::unordered_map<int, TcpConnectionPtr> m_userConn_map;
    std::unordered_map<int, MsgHandler> m_msgHandler_map;
    UserModel m_usermodel;
    std::mutex m_mutex;
    OfflineMsgModel m_offlinemsg;
    FriendModel m_friendmodel;
    GroupModel m_groupmodel;
};

#endif