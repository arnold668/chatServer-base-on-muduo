#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include"group.hpp"
#include<string>
#include<vector>
using namespace std;
class GroupModel
{
public:
    bool createGroup(Group &group);
    void addGroup(int userid,int groupid,string role);
    //查询用户所在群组信息,登录成功返回群组信息，加入哪些群各个有哪些成员
    vector<Group> queryGroups(int userid);
    //根据groupid查询用户id列表，,拿到群里用户id，可以转发消息
    vector<int> queryGroupUsers(int userid,int groupid);
private:
};



#endif