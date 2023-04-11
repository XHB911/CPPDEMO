#ifndef __ABOO_GROUPUSER_H__
#define __ABOO_GROUPUSER_H__

#include "user.hpp"

class GroupUser : public User {
public:
    void setRole(std::string role) { m_role = role; }
    std::string getRole() { return m_role; }
private:
    std::string m_role;
};

#endif