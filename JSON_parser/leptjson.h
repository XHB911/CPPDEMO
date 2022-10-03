#ifndef _LEPTJSON_H__
#define _LEPTJSON_H__

#include <string>
#include <stdlib.h>

#define EXPECT(c, ch) do { assert(*c.json == (ch)); c.json++; } while(0)
#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')

namespace leptjson{

typedef enum {
	JSON_NULL,
	JSON_FALSE,
	JSON_TRUE,
	JSON_NUMBER,
	JSON_STRING,
	JSON_ARRAY,
	JSON_OBJECT
} lept_type;

typedef struct {
	double n;
	lept_type type;
} lept_value;

typedef struct {
	const char* json;
} lept_context;

enum {
	LEPT_PARSE_OK = 0,
	LEPT_PARSE_EXPECT_VALUE,
	LEPT_PARSE_INVALID_VALUE,
	LEPT_PARSE_ROOT_NOT_SINGULAR,
	LEPT_PARSE_NUMBER_TOO_BIG
};

class lept_json {
private:

public:
	static int parse(lept_value& v, const char* json);
	static const lept_type get_type(const lept_value& v);
	static double get_number(const lept_value& v);
private:
	static void parse_whitespace(lept_context& c);
	static int parse_literal(lept_context& c, lept_value& v, const char* literal, lept_type type);
	static int parse_value(lept_context& c, lept_value& v);
	static int parse_number(lept_context& c, lept_value& v);
};

};

#endif
