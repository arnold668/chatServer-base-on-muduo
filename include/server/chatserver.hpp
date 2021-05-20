#ifndef CHATSERVER_H
#define CHATSERVER_H

#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>

using namespace muduo;
using namespace muduo::net;

class ChatServer
{
public:
    ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg);
    void start();
private:
    void onConnection(const TcpConnectionPtr&);//连接创建断开回调
    //读写回调
    void onMessage(const TcpConnectionPtr&,
                            Buffer*,
                            Timestamp);
    //TcpServer 实现服务器功能的类对象
    TcpServer _server;
    //指向事件循环对象的指针
    EventLoop*_loop;
};

#endif