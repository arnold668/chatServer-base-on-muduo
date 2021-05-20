#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "user.hpp"
#include "group.hpp"
#include "public.hpp"
#include <chrono>
#include <ctime>
#include <thread>
// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;
// 显示当前登录成功用户的基本信息
void showCurrentUserData();
//创建接收线程
void readTaskHandler(int clientfd);
//获取时间
// 控制主菜单页面程序,判断：登录成功后进入，退出后不进入
bool isMainMenuRunning = false;
// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int);

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }
    char *ip = argv[1];
    int port = atoi(argv[2]);

    //socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    for (;;)
    {
        // 显示首页面菜单 登录、注册、退出
        cout << "========================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "========================" << endl;
        cout << "choice:";
        int choice = 0;
        cin >> choice;
        cin.get();

        switch (choice)
        {
        case 1:
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "userid" << endl;
            cin >> id;
            cin.get();
            cout << "password" << endl;
            cin.getline(pwd, 50);

            json js;
            js["id"] = id;
            js["msgid"] = LOGIN_MSG;
            js["password"] = pwd;
            string request = js.dump();
            //sizeo不一样
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (-1 == len)
            {
                cerr << "send login msg error" << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (-1 == len)
                {
                    cerr << "recv login response error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    //然后判断js内的errno是否为1,1代表登录失败
                    if (0 != responsejs["errno"].get<int>())
                    {
                        cerr << " login  error，msgid：" << responsejs["errno"] << endl;
                    }
                    else
                    {
                        g_currentUser.setId(responsejs["id"].get<int>());
                        g_currentUser.setName(responsejs["name"]);
                        //friend list 记录
                        if (responsejs.contains("friends"))
                        {
                            g_currentUserFriendList.clear();
                            vector<string> vec = responsejs["friends"];
                            for (string &vecstr : vec)
                            {
                                json js = json::parse(vecstr);
                                User user;
                                user.setId(js["id"].get<int>());
                                user.setName(js["name"]);
                                user.setState(js["state"]);
                                g_currentUserFriendList.push_back(user);
                            }
                        }
                        if (responsejs.contains("groups"))
                        {
                            // 初始化
                            g_currentUserGroupList.clear();

                            vector<string> vec1 = responsejs["groups"];
                            for (string &groupstr : vec1)
                            {
                                json grpjs = json::parse(groupstr);
                                Group group;
                                group.setId(grpjs["id"].get<int>());
                                group.setName(grpjs["groupname"]);
                                group.setDesc(grpjs["groupdesc"]);

                                vector<string> vec2 = grpjs["users"];
                                for (string &userstr : vec2)
                                {
                                    GroupUser user;
                                    json js = json::parse(userstr);
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    user.setRole(js["role"]);
                                    group.getUsers().push_back(user);
                                }

                                g_currentUserGroupList.push_back(group);
                            }
                        }

                        //显示登录用户基本信息
                        showCurrentUserData();
                        //显示离线消息
                        if (responsejs.contains("offlinemsg"))
                        {
                            vector<string> vec = responsejs["offlinemsg"];
                            for (string s : vec)
                            {
                                json js = json::parse(s);
                                if (ONE_CHAT_MSG == js["msgid"].get<int>())
                                {
                                    cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                                         << " said: " << js["msg"].get<string>() << endl;
                                }
                                else 
                                {
                                    cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                                        << " said: " << js["msg"].get<string>() << endl;                              
                                }
                            }
                        }
                        //登录成功后，启动接收线程，通过static设置为一次
                        static int readthreadnumber = 0;
                        if (readthreadnumber == 0)
                        {
                            thread readTask(readTaskHandler, clientfd);
                            readTask.detach();
                            readthreadnumber++;
                        }

                        //进入聊天界面
                        isMainMenuRunning=true;
                        mainMenu(clientfd);
                    }
                }
            }
        }
        break;
        case 2:
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username" << endl;
            cin.getline(name, 50);
            cout << "userpassword" << endl;
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error:" << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (-1 == len)
                {
                    cerr << "recv reg response error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"].get<int>()) // 注册失败
                    {
                        cerr << name << " is already exist, register error!" << endl;
                    }
                    else // 注册成功
                    {
                        cout << name << " register success, userid is " << responsejs["id"]
                             << ", do not forget it!" << endl;
                    }
                }
            }
        }
        break;
        case 3:
            close(clientfd);
            exit(0);
        default:
            cerr << "invalid input!" << endl;
            break;
        }
    }
    return 0;
}

void showCurrentUserData()
{
    cout << "--------------------login user------------------------" << endl;
    cout << "current login user =>id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << endl;
    cout << "--------------------friend list------------------------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "--------------------group list------------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState() << " " << user.getRole() << endl;
            }
        }
    }
    cout << "------------------------------------------------------" << endl;
}

void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if (-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }

        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        cout << endl
             << "msgtype" << msgtype << endl;
        //接收chatservice转达的信息
        if (msgtype == ONE_CHAT_MSG)
        {
            cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        else if(msgtype == GROUP_CHAT_MSG)
        {
            if (GROUP_CHAT_MSG == msgtype)
            {
                cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                    << " said: " << js["msg"].get<string>() << endl;
                continue;
            }
        }
    }
}

//系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群聊，格式groupchat:groupid:message"},
    {"loginout", "注销，格式loginout"}};
//句柄
// "help" command handler
void help(int fd = 0, string str = "");
// "chat" command handler
void chat(int, string);
// "addfriend" command handler
void addfriend(int, string);
// "creategroup" command handler
void creategroup(int, string);
// "addgroup" command handler
void addgroup(int, string);
// "groupchat" command handler
void groupchat(int, string);
// "loginout" command handler
void loginout(int, string);

unordered_map<string,function<void(int,string)>> commandHandlerMap={
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};


void mainMenu(int clientfd)
{
    help();
    char buffer[1024]={0};
    while(isMainMenuRunning)
    {
        cin.getline(buffer,1024);
        string commandbuf(buffer);
        string command;
        int idex=commandbuf.find(":");
        if(idex==-1)
        {
            command=commandbuf;
        }
        else{
            command=commandbuf.substr(0,idex);
        }
        auto it =commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
            continue;
        }

        it->second(clientfd,commandbuf.substr(idex+1,commandbuf.size()-idex));

    }
}

void help(int clientfd, string str)
{
    cout<<"show conmmand list"<<endl;
    for(auto &p:commandMap)
    {
        cout<<p.first<<":"<<p.second<<endl;
    }
    cout<<endl;
}

void chat(int clientfd, string str)
{
    
    int idx=str.find(':');
    //string find返回npos即-1，无符号数
    if(-1==idx)
    {
        cerr<<"chat command invalid!"<<endl;
        return;
    }
    int friendid=atoi(str.substr(0,idx).c_str());
    string message=str.substr(idx+1,str.size()-idx);
    json js;
    js["msgid"]=ONE_CHAT_MSG;
    js["id"]=g_currentUser.getId();
    js["name"]=g_currentUser.getName();
    js["to"]=friendid;
    js["time"]=getCurrentTime();
    js["msg"]=message;
    string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len)
    {
        cerr<<"send chat msg error->"<<buffer<<endl;
    }


}
// "addfriend" command handler
void addfriend(int clientfd, string str)
{
    
    int friendid =atoi(str.c_str());
    json js;
    js["msgid"]=ADD_FRIEND_MSG;
    js["id"]=g_currentUser.getId();
    js["friendid"]=friendid;
    string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len)
    {
        cerr<<"add friend error"<<endl;
    }
}
// "creategroup" command handler
void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "creategroup command invalid!" << endl;
        return;
    }

    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send creategroup msg error -> " << buffer << endl;
    }
}
// "addgroup" command handler
void addgroup(int clientfd, string str)
{
    int groupid=atoi(str.c_str());
    json js; 
    js["msgid"]=ADD_GROUP_MSG;
    js["groupid"]=groupid;
    js["id"]=g_currentUser.getId();
    string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if (-1 == len)
    {
        cerr << "send addgroup msg error -> " << buffer << endl;
    }   
}
// "groupchat" command handler
void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "groupchat command invalid!" << endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send groupchat msg error -> " << buffer << endl;
    }



}
// "loginout" command handler
void loginout(int clientfd, string str)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send loginout msg error -> " << buffer << endl;
    }
    else
    {
        isMainMenuRunning = false;
    }   

}
    // 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}