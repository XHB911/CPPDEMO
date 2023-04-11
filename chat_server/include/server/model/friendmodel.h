#ifndef __ABOO_FRINEDMODEL_H__
#define __ABOO_FRINEDMODEL_H__

#include "user.hpp"

#include <vector>

class FriendModel {
public:
    void insert(int userid, int friendid);
    std::vector<User> query(int userid);
};

#endif