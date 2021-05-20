#ifndef PUBLIC_H
#define PUBLIC_H

//enum ，默认从第一个号继续，REG_MSG为2
enum EnMsgType
{
    LOGIN_MSG=1,
    LOGIN_MSG_ACK,
    LOGINOUT_MSG,
    REG_MSG,//注册消息,客户端
    REG_MSG_ACK,//注册响应消息，服务器
    ONE_CHAT_MSG,//聊天消息
    ADD_FRIEND_MSG,//添加好友
    CREATE_GROUP_MSG,
    ADD_GROUP_MSG,
    GROUP_CHAT_MSG,


};
#endif