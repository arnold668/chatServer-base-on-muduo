#include "offlinemessagemodel.hpp"
#include "db.h"

void OfflineMsgModel::insert(int userid, string msg)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values(%d,'%s')",userid,msg.c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
void OfflineMsgModel::remove(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid=%d", userid);
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
//查询
vector<string> OfflineMsgModel::query(int userid)
{
    char sql[1024]={0};
    sprintf(sql,"select message from offlinemessage where userid=%d",userid);
    vector<string> vec;
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
                vec.push_back(row[0]);
            }
                //需要释放资源，res为指针 代表内部通过动态开辟申请资源
            mysql_free_result(res);
            return vec;
            
        }
    }
    return vec;
}