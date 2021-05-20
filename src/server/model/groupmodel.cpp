#include "groupmodel.hpp"
#include "db.h"

bool GroupModel::createGroup(Group &group)
{
    char sql[1024]={0};
    sprintf(sql,"insert into allgroup(groupname,groupdesc) values('%s','%s')"
        ,group.getName().c_str(),group.getDesc().c_str());
    MySQL mysql;
    if(mysql.connect()){
        if (mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }

    return false;
    
}
void GroupModel::addGroup(int userid, int groupid, string role)
{
    char sql[1024]={0};
    sprintf(sql,"insert into groupuser values(%d,%d,'%s')"
        ,groupid,userid,role.c_str());
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }


}
//查询用户所在群组信息,登录成功返回群组信息，加入哪些群各个有哪些成员
vector<Group> GroupModel::queryGroups(int userid)
{
    char sql[1024]={0};
    sprintf(sql,"select a.id,a.groupname,a.groupdesc from allgroup a inner join groupuser b on b.groupid=a.id where b.userid=%d"
        ,userid);
    vector<Group> groupVec;
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
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);
            }
                //需要释放资源，res为指针 代表内部通过动态开辟申请资源
            mysql_free_result(res);           
        }
    }
       // 查询群组的用户信息
    for (Group &group : groupVec)
    {
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a \
            inner join groupuser b on b.userid = a.id where b.groupid=%d",
                group.getId());

        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                //存入vector<GroupUser> users;当前group里的users
                group.getUsers().push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}
//根据groupid查询用户id列表，除userid自己，主要用户群聊业务给群组其它成员群发消息,拿到群里用户id，可以转发消息
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid = %d and userid != %d", groupid, userid);

    vector<int> idVec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;

}