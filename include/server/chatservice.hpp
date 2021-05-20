#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include "json.hpp"
#include"public.hpp"
#include"user.hpp"
#include<mutex>
#include"offlinemessagemodel.hpp"
#include"friendmodel.hpp"
using namespace std;
using namespace muduo;
using namespace muduo::net;

#include"groupmodel.hpp"
#include"usermodel.hpp"
#include"redis.hpp"
// #include<functional>
// #include<string>
// using namespace std;
// using namespace placeholders;
using json = nlohmann::json;
//用using给一个存在的类型定义新的类型名称
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp time)>;
//做业务，采用单例模式设计聊天服务业务类
class ChatService
{
public:
    //单例对象接口函数
    static ChatService *instance();
    MsgHandler  getHandler(int msgid);
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time) ;
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time) ;
    //客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    //一对一聊天
    void oneChat(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //服务器重置异常
    void reset();
    //添加好友
    void addFriend(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //创建群组
    void createGroup(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //加入群组
    void addGroup(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //查询组员
    void groupChat(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //查询组员
    void loginout(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int ,string);
private : 
    ChatService();
    //存储消息id和其对应的业务处理方法 
    unordered_map<int, MsgHandler> _msgHandlerMap;

    //存储在线用户的通信连接
    unordered_map<int ,TcpConnectionPtr> _userConnMap;
    //互斥锁
    mutex _connMutex;
    //数据model类
    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;
    Redis _redis;

};

#endif