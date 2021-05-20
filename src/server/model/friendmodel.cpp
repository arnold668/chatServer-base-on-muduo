#include "friendmodel.hpp"

void FriendModel::insert(int userid, int friendid)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d,%d)",userid,friendid);
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }

}

vector<User> FriendModel::query(int userid)
{
    char sql[1024]={0};
    sprintf(sql,"select a.id,a.name,a.state from user a inner join friend b on b.friendid=a.id where b.userid=%d"
        ,userid);
    vector<User> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            //mysql_fetch_row返回行
            MYSQL_ROW row ;
            //不为空代表有数据
            //while代表可能有多条信息
            while ((row = mysql_fetch_row(res))!= nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
                //需要释放资源，res为指针 代表内部通过动态开辟申请资源
            mysql_free_result(res);
            return vec;
            
        }
    }
    return vec;
}