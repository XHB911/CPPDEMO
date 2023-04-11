#ifndef __ABOO_COMMON_H__
#define __ABOO_COMMON_H__

enum EnMsgType {
    LOGIN_MSG = 1,
    LOGIN_MSG_ACK,
    REG_MSG,
    RES_MSG_ACK,
    ONE_CHAT_MSG,
    ADD_FRIEND_MSG,

    CREATE_GROUP_MSG,
    ADD_GROUP_MSG,
    GROUP_CHAT_MSG,
};

#endif