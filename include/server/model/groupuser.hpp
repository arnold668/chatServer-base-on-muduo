#ifndef GROUPUSER_H
#define GROUPUSER_H

#include"user.hpp"

//继承 添加在群组中role信息
class GroupUser:public User
{
public:
    void setRole(string role){this->role=role;}
    string getRole(){return this->role;}
private:
    string role;
};



#endif