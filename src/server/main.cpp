#include <iostream>
#include "chatserver.hpp"
#include"chatservice.hpp"
#include<signal.h>
using namespace std;
//ctrl+c结束重置user 的state信息
void resetHandler(int){
    ChatService::instance()->reset();
    exit(0);
}
int main(int argc,char**argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }
    char*ip=argv[1];
    uint16_t port= atoi(argv[2]);
    signal(SIGINT,resetHandler);
    EventLoop loop;
    InetAddress addr(ip,port);
    // InetAddress addr("127.0.0.1",6000);
    ChatServer server(&loop,addr,"ChateServer");
    server.start();
    loop.loop();
    return 0;
}