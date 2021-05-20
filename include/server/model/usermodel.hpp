#ifndef USERMODEL_H
#define USERMODEL_H
#include"user.hpp"
#include"db.h"
class UserModel
{
public:
  bool insert(User &user);
  //根据主键信息查询用户
  User query(int id);
  //更新状态信息
  bool updateState(User user);
  //重置为默认状态
  void resetState();
};

#endif