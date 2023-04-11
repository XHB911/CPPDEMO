#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <mysql/mysql.h>
#include <string>
#include "./util.h"

using namespace std;

class MySQL {
public:
	MySQL() {
		_conn = mysql_init(nullptr);
	}
	~MySQL() {
		if (_conn != nullptr) {
			mysql_close(_conn);
		}
	}
	bool connect(string ip, unsigned short port, string user, string password, string dbname) {

	}
	bool update(string sql) {
		if (mysql_query(_conn, sql.c_str())) {
			LOG("更新失败:" + sql);
			return false;
		}
		return true;
	}
	MYSQL_RES* query(string sql) {
		if (mysql_query(_conn, sql.c_str())) {
			LOG("查询失败" + sql);
			return nullptr;
		}
		return mysql_use_result(_conn);
	}
private:
	MYSQL* _conn;
};

class Connection {
public:
};

#endif
