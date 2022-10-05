#ifndef _LEPTJSON_H__
#define _LEPTJSON_H__

#include <string>
#include <stdlib.h>

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch) do { assert(*c.json == (ch)); c.json++; } while(0)
#define STRING_ERROR(ret) do { c.top = head; return ret; } while(0)
#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch) do { *(char*)lept_context::push(c, sizeof(char)) = (ch); } while(0)

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

struct lept_value {
	union {
		struct {
			char* s;
			size_t len;
		} s;
		double n;
	} u;
	lept_type type;
};

struct lept_context {
	const char* json;
	char* stack;
	size_t size, top;
	static void* push(lept_context& c, size_t size);
	static void* pop(lept_context& c, size_t size);
};

enum {
	LEPT_PARSE_OK = 0,
	LEPT_PARSE_EXPECT_VALUE,
	LEPT_PARSE_INVALID_VALUE,
	LEPT_PARSE_ROOT_NOT_SINGULAR,
	LEPT_PARSE_NUMBER_TOO_BIG,
	LEPT_PARSE_MISS_QUOTATION_MARK,
	LEPT_PARSE_INVALID_STRING_ESCAPE,
    LEPT_PARSE_INVALID_STRING_CHAR,
	LEPT_PARSE_INVALID_UNICODE_HEX,
	LEPT_PARSE_INVALID_UNICODE_SURROGATE
};

class lept_json {
public:
	static int parse(lept_value& v, const char* json);
	static const lept_type get_type(const lept_value& v);
	static void init(lept_value& v) { v.type = JSON_NULL; }
	static void set_null(lept_value& v) { return lept_free(v); }
	static int get_boolean(const lept_value& v);
	static void set_boolean(lept_value& v, int b);
	static double get_number(const lept_value& v);
	static void set_number(lept_value& v, double n);
	static const char* get_string(const lept_value& v);
	static size_t get_string_length(const lept_value& v);
	static void set_string(lept_value& v, const char* s, size_t len);
private:
	static void parse_whitespace(lept_context& c);
	static int parse_literal(lept_context& c, lept_value& v, const char* literal, lept_type type);
	static int parse_value(lept_context& c, lept_value& v);
	static int parse_number(lept_context& c, lept_value& v);
	static int parse_string(lept_context& c, lept_value& v);
	static const char* parse_hex4(const char* p, unsigned& u);
	static void encode_utf8(lept_context& c, unsigned u);
	static void lept_free(lept_value& v) {
		if (v.type == JSON_STRING) free(v.u.s.s);
		v.type = JSON_NULL;
	}

};

};

#endif
