#include "chatservice.h"
#include "common.h"
#include "user.hpp"
#include "usermodel.h"
#include "json.hpp"

#include <string>
#include <muduo/base/Logging.h>

using json = nlohmann::json;

ChatService* ChatService::instance() {
    static ChatService service;
    return &service;
}

ChatService::ChatService() {
    m_msgHandler_map.insert({LOGIN_MSG, std::bind(&ChatService::login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    m_msgHandler_map.insert({REG_MSG, std::bind(&ChatService::reg, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    m_msgHandler_map.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    m_msgHandler_map.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    m_msgHandler_map.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    m_msgHandler_map.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    m_msgHandler_map.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
}

MsgHandler ChatService::getHandler(int msgid) {
    auto it = m_msgHandler_map.find(msgid);
    if (it == m_msgHandler_map.end()) {
        return [=](const TcpConnectionPtr& conn, json & js, Timestamp) ->void {
            LOG_ERROR << "msgid:" << msgid << " cannot find handler!";
        };
    } else {
        return m_msgHandler_map[msgid];
    }
}

void ChatService::login(const TcpConnectionPtr& conn, json & js, Timestamp tm) {
    int id = js["id"].get<int>();
    std::string pwd = js["password"];

    User user = m_usermodel.query(id);
    if (user.getId() == id && user.getPwd() == pwd) {
        if (user.getState() == "online") {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "该账号已登录，请重新输入。";
            conn->send(response.dump());
        } else {
            user.setState("online");
            m_usermodel.updateState(user);
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_userConn_map.insert({id, conn});
            }

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询账号拥有离线消息
            std::vector<std::string> vec = m_offlinemsg.query(id);
            if (!vec.empty()) {
                response["offlinemsg"] = vec;
                m_offlinemsg.remove(id);
            }

            std::vector<User> userVec = m_friendmodel.query(id);
            if (!userVec.empty()) {
                std::vector<std::string> vec2;
                for (User& user : userVec) {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            conn->send(response.dump());
        }
    } else {
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户不存在或者密码错误。";
        conn->send(response.dump());
    }
}

void ChatService::reg(const TcpConnectionPtr& conn, json & js, Timestamp tm) {
    std::string name = js["name"];
    std::string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = m_usermodel.insert(user);
    if (state) {
        json response;
        response["msgid"] = RES_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    } else {
        json response;
        response["msgid"] = RES_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

void ChatService::clientCloseExpection(const TcpConnectionPtr conn) {
    User user;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto it = m_userConn_map.begin(); it != m_userConn_map.end(); ++it) {
            if (it->second == conn) {
                user.setId(it->first);
                m_userConn_map.erase(it);
                break;
            }
        }
    }
    if (user.getId() != -1) {
        user.setState("offline");
        m_usermodel.updateState(user);
    }
}

void ChatService::oneChat(const TcpConnectionPtr& conn, json & js, Timestamp tm) {
    int toid = js["to"].get<int>();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_userConn_map.find(toid);
        if (it != m_userConn_map.end()) {
            // 转发消息
            it->second->send(js.dump());
            return;
        }
    }

    m_offlinemsg.insert(toid, js.dump());
}

void ChatService::reset() {
    m_usermodel.resetState();
}

void ChatService::addFriend(const TcpConnectionPtr& conn, json & js, Timestamp tm) {
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    m_friendmodel.insert(userid, friendid);
}

void ChatService::createGroup(const TcpConnectionPtr& conn, json & js, Timestamp tm) {
    int userid = js["id"].get<int>();
    std::string name = js["groupname"];
    std::string desc = js["groupdesc"];

    Group group(-1, name, desc);
    if (m_groupmodel.createGroup(group)) m_groupmodel.addGroup(userid, group.getId(), "creator");
}

void ChatService::addGroup(const TcpConnectionPtr& conn, json & js, Timestamp tm) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    m_groupmodel.addGroup(userid, groupid, "normal");
}

void ChatService::groupChat(const TcpConnectionPtr& conn, json & js, Timestamp tm) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>(); 

    std::vector<int> useridVec = m_groupmodel.queryGroupUsers(userid, groupid); 
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto id : useridVec) {
        auto it = m_userConn_map.find(id);
        if (it != m_userConn_map.end()) it->second->send(js.dump());
        else m_offlinemsg.insert(id, js.dump());
    }
}
