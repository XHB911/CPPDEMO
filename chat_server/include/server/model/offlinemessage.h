#ifndef __ABOO_OFFLINEMSG_H__
#define __ABOO_OFFLINEMSG_H__

#include <string>
#include <queue>

class OfflineMsgModel {
public:
    void insert(int, std::string);
    void remove(int);
    std::vector<std::string> query(int);
};

#endif