#ifndef FRIENDMODL_H
#define FRIENDMODL_H

#include "user.hpp"
#include "db.h"
#include<vector>
using namespace std;
//维护好友信息的操作接口方法
class FriendModel
{
public:
    //添加好友关系
    void insert(int userid,int friendid);
    //返回好友信息
    vector<User> query(int userid);
private:
    /* data */
    User user;

};




#endif