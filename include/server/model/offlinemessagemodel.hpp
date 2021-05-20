#ifndef OFFLINEMESSAGEMODLE_H
#define OFFLINEMESSAGEMODLE_H

#include<string>
#include<vector>
using namespace std;
class OfflineMsgModel
{

public:
    void insert(int userid,string msg);
    void remove(int userid);
    //查询
    vector<string> query(int userid);
private:
    /* data */
    // OfflineMsgModel(/* args */);
};



#endif