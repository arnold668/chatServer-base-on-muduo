#include "usermodel.hpp"
#include "db.h"
#include <iostream>
using namespace std;
bool UserModel::insert(User &user)
{
    //1.组装sql
    char sql[1024] = {0};
    //string转字符数组

    sprintf(sql, "insert into user(name,password,state) values('%s','%s','%s')",
            user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            //獲取插入主鍵id插入成功的用戶
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}
User UserModel::query(int id)
{
    char sql[1024] = {0};
    //string转字符数组

    sprintf(sql, "select *from user where id=%d", id);
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            //mysql_fetch_row返回行
            MYSQL_ROW row = mysql_fetch_row(res);
            //不为空代表有数据
            if (row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                //需要释放资源，res为指针 代表内部通过动态开辟申请资源
                mysql_free_result(res);
                return user;
            }
        }
    }
    //如果未发现，直接返回User，其默认id为-1，-1代表出错
    //所以在chatservice里加入判断是否id为-1，因为默认pwd为空
    return User();
}

bool UserModel::updateState(User user)
{
        //1.组装sql
    char sql[1024] = {0};
    //string转字符数组
    //第一次报错原因，user.getState()为转char*
    sprintf(sql, "update  user set state='%s' where id=%d", user.getState().c_str(),user.getId());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}

void UserModel::resetState()
{
    //1.组装sql
    char sql[1024] = {0};
    //string转字符数组
    //第一次报错原因，user.getState()为转char*
    sprintf(sql, "update  user set state='offline' where state='online'");

    MySQL mysql;
    if (mysql.connect())
        mysql.update(sql);
    
}