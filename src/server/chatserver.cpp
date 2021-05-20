#include "chatserver.hpp"
#include "json.hpp"
#include <functional>
#include <string>
#include "chatservice.hpp"
using namespace std;
using namespace placeholders;
using json = nlohmann::json;
ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr,
                       const string &nameArg) : _server(loop, listenAddr, nameArg)
{
    _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));
    _server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));
    _server.setThreadNum(4);
}
void ChatServer::start()
{
    _server.start();
}
//连接创建断开回调
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    //客户端断开连接
    if (!conn->connected())
    {
        //shutdown？？
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
        
    }
}
//读写回调
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    // Buffer数据缓冲区
    string buf = buffer->retrieveAllAsString();
    //数据反序列化
    json js = json::parse(buf);
    //里面的不是整形，要转换.get<int>()
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    msgHandler(conn, js, time);
}


