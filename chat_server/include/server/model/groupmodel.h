#ifndef __ABOO_GROUPMODEL_H__
#define __ABOO_GROUPMODEL_H__

#include "group.h"

#include <string>
#include <vector>

class GroupModel {
public:
    bool createGroup(Group& group);
    void addGroup(int userid, int groupid, std::string role);
    std::vector<Group> queryGroups(int userid);
    std::vector<int> queryGroupUsers(int userid, int groupid);
};

#endif