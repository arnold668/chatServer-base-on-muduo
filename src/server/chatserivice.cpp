#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <vector>
#include<map>
using namespace std;
using namespace muduo;
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    // _msgHandlerMap.insert({LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)});
        // 群组业务管理相关事件处理回调注册
    //连接redis服务器
    if(_redis.connect())
    {
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage,this,_1,_2));
    }
}

//优点，可以看日志，没有对应的业务号进程也不会挂掉
MsgHandler ChatService::getHandler(int msgid)
{
    //错误日志，查询map，通过find函数，不能用[],否则会自己先创建
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        //使用muduo库打印错误日志,include<muduo/base/Logging.h>,不需要加endl，已存在
        //网络模块必须得处理服务模块抛出的异常，不处理会结束进程,
        //返回一个默认的处理器，空操作 //优点，可以看日志，没有对应的业务号进程也不会挂掉
        // LOG_ERROR<<"msgid:"<<msgid<<"can not find handler";
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time) {
            LOG_ERROR << "msgid:" << msgid << "  can not find handler";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
    return _msgHandlerMap[msgid];
}
void ChatService::loginout(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // // 用户注销，相当于就是下线，在redis中取消订阅通道
    // _redis.unsubscribe(userid); 
    //向redis取消订阅
    _redis.unsubscribe(userid);
    // 更新用户的状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    /*
    分用户密码正确（考虑登录情况，已登录报错，未登录更新登录状态）
    用户不正确，（内包含用户不存在情况，未讨论）
    errno错误码 0：正常 1 ：用户名或密码错误 2：重复登录
    errmsg
    */
    // LOG_INFO<<"it is login service!";
    // int id = js["id"]; 需要将其转换成整形，接收的是字符串表示的id
    int id = js["id"].get<int>();
    string pwd = js["password"];
    User user = _userModel.query(id);
    //user.getId()!=-1表示查到此数据存在
    if (user.getId() == id && user.getPwd() == pwd)
    {
        //考虑重复登录
        if (user.getState() == "online")
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "该账户已登录";
            conn->send(response.dump());
        }
        else
        {
            //登录成功
            {
                //互斥锁
                lock_guard<mutex> lock(_connMutex);
                //记录用户连接状态
                _userConnMap.insert({id, conn});
            }

            //向redis订阅channel[id];
            _redis.subscribe(id);

            //更新用户状态
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0; //业务成功消息，如果为1不用读其他字段
            response["id"] = user.getId();
            response["name"] = user.getName();

            //查询并处理离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            //读取后删除消息
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                _offlineMsgModel.remove(id);
            }
            //查询该用户的好友信息并返回
            vector<User> userVec=_friendModel.query(id);
            if(!userVec.empty()){
               vector<string> vec2;
               for(User &user:userVec){
                   json js;
                   js["id"]=user.getId();
                   js["name"]=user.getName();
                   js["state"]=user.getState();
                   vec2.push_back(js.dump());
               }
               response["friends"]=vec2;
            }
            //查询用户的群组信息
            vector<Group> groupuserVec=_groupModel.queryGroups(id);
            if(!groupuserVec.empty())
            {
                //group:[groupid:[xxx,xx]]
                vector<string> groupV;
                for(Group &group:groupuserVec)
                {
                    json grpjs;
                    grpjs["id"]=group.getId();
                    grpjs["groupname"]=group.getName();
                    grpjs["groupdesc"]=group.getDesc();
                    vector<string> userV;
                    for(GroupUser &user:group.getUsers())
                    {
                        json js;
                        js["id"]=user.getId();
                        js["name"]=user.getName();
                        js["state"]=user.getState();
                        js["role"]=user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjs["users"]=userV;
                    groupV.push_back(grpjs.dump());
                }
                response["groups"]=groupV;
            }
            conn->send(response.dump());
        }
    }
    else
    {
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1; //业务失败消息，如果为1不用读其他字段
        response["errmsg"] = "用户名密码错误";
        // response["id"]=user.getId();
        conn->send(response.dump());
    }
}
//注册消息过来后，网络对其反序列化生成json对象，上报到注册业务
//name pwd
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // LOG_INFO<<"it is reg service!";
    string name = js["name"];
    string pwd = js["password"];
    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    if (state)
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0; //业务成功消息，如果为1不用读其他字段
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1; //业务失败消息，如果为1不用读其他字段
        // response["id"]=user.getId();
        conn->send(response.dump());
    }
}

void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{

    User user;
    {

        lock_guard<mutex> lock(_connMutex);
        // 换成auto unordered_map<int ,TcpConnectionPtr>::iterator it;
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // int id=it->first;
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    //如果是-1，不需要像数据库发送请求
    if (user.getId() != -1)
    {
        //更新用户状态信息，
        user.setState("offline");
        _userModel.updateState(user);
    }
    //向redis取消订阅
    _redis.unsubscribe(user.getId());
}

void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["to"].get<int>();
    //加锁，防止map内容被占用
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            it->second->send(js.dump());
            return;
        }
        
    }
    //查询数据库里id的state状态，如果是online代表在另一台服务器
    if(_userModel.query(toid).getState()=="online")
    {
        _redis.publish(toid,js.dump());
        return;
    }
    _offlineMsgModel.insert(toid, js.dump());
}

void ChatService::reset()
{
    //将onine状态用户设置为offline
    _userModel.resetState();
}
//添加好友
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int  userid=js["id"].get<int>();
    int friendid=js["friendid"].get<int>();
    //存储
    _friendModel.insert(userid,friendid);
}

//创建群组
void ChatService::createGroup(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid=js["id"].get<int>();
    string  groupName=js["groupname"];
    string  groupDesc=js["groupdesc"];
    Group group(-1,groupName,groupDesc);
    // group.setName(groupName);
    // group.setDesc(groupDesc);
    //创建完需要加入群组，role为creator
    //creator可以在model通过常量替换
    if(_groupModel.createGroup(group)) 
    {
        _groupModel.addGroup(userid,group.getId(),"creator");
    }
    
}
//加入群组
void ChatService::addGroup(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid=js["id"].get<int>();
    int groupid=js["groupid"].get<int>();
    // 此行错误，加入的角色必为normal，而不是发送的json获取，string role=js["grouprole"];
    _groupModel.addGroup(userid,groupid,"normal");

}
//查询组员
void ChatService::groupChat(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid=js["id"].get<int>();
    int groupid=js["groupid"].get<int>();
    vector<int> useridVec=_groupModel.queryGroupUsers(userid,groupid);
    //先拿到id，找到这个用户，如果找到说明在线则直接转发，不在线则离线保存
    lock_guard<mutex> lock(_connMutex);
    for(int id:useridVec)
    {
        auto it=_userConnMap.find(id);
        if(it!=_userConnMap.end())
        {
            it->second->send(js.dump());
        }
        else{
            if(_userModel.query(id).getState()=="online")
            {
                _redis.publish(id,js.dump());
                return;
            }
            else{
                _offlineMsgModel.insert(id,js.dump());
            }

        }
    }
}

//从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid,string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it =_userConnMap.find(userid);
    if(it!=_userConnMap.end())
    {
        it->second->send(msg);
        return;
    }
    _offlineMsgModel.insert(userid,msg);
}