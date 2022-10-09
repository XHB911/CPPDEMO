#ifndef _LEPTJSON_H__
#define _LEPTJSON_H__

#include <string>
#include <stdlib.h>
#include <assert.h>

#ifndef LEPT_KEY_NOT_EXIST
#define LEPT_KEY_NOT_EXIST (sizeof(size_t) - 1)
#endif


#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#ifndef LEPT_PARSE_STRINGIFY_INIT_SIZE
#define LEPT_PARSE_STRINGIFY_INIT_SIZE 256
#endif

#define EXPECT(c, ch) do { assert(*c.json == (ch)); c.json++; } while(0)
#define STRING_ERROR(ret) do { c.top = head; return ret; } while(0)
#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch) do { *(char*)lept_context::push(c, sizeof(char)) = (ch); } while(0)
#define PUTS(c, s, len) memcpy(lept_context::push(c, len), s, len)

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

struct lept_member;

struct lept_value {
	union {
		struct {
			char* s;
			size_t len;
		} s;	// 字符串
		struct {
			lept_value* e;
			size_t size;	// 元素个数
			size_t capacity;
		} a;	// 数组
		struct {
			lept_member* m;
			size_t size;
			size_t capacity;
		} o;	// 对象
		double n;	// 数值
	} u;
	lept_type type;
};

struct lept_member {
	char* k;
	size_t klen;
	lept_value v;
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
	LEPT_PARSE_INVALID_UNICODE_SURROGATE,
	LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
	LEPT_PARSE_MISS_KEY,
	LEPT_PARSE_MISS_COLON,
	LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET
};

class lept_json {
public:
	static int parse(lept_value& v, const char* json);
	static char* stringify(const lept_value& v, size_t& length);

	static const lept_type get_type(const lept_value& v);
	static void init(lept_value& v) { v.type = JSON_NULL; }
	static void copy(lept_value& dst, const lept_value& src);
	static void move(lept_value& dst, lept_value& src);
	static void swap(lept_value& lhs, lept_value& rhs);
	static bool is_equal(const lept_value& lhs, const lept_value& rhs);

	static void set_null(lept_value& v) { return lept_free(v); }

	static int get_boolean(const lept_value& v);
	static void set_boolean(lept_value& v, int b);

	static double get_number(const lept_value& v);
	static void set_number(lept_value& v, double n);

	static const char* get_string(const lept_value& v);
	static size_t get_string_length(const lept_value& v);
	static void set_string(lept_value& v, const char* s, size_t len);

	static size_t get_array_size(const lept_value& v);
	static size_t get_array_capacity(const lept_value& v);
	static void reserve_array(lept_value& v, size_t capacity);
	static void shrink_array(lept_value& v);
	static void clear_array(lept_value& v);
	static lept_value& get_array_element(const lept_value& v, size_t index);
	static lept_value& pushback_array_element(lept_value& v);
	static void popback_array_element(lept_value& v);
	static lept_value& insert_array_element(lept_value& v, size_t index);
	static void erase_array_element(lept_value& v, size_t index, size_t count);
	static void set_array(lept_value& v, size_t capacity);

	static void set_object(lept_value& v, size_t capacity);
	static size_t get_object_size(const lept_value& v);
	static size_t get_object_capacity(const lept_value& v);
	static void reserve_object(lept_value& v, size_t capacity);
	static void shrink_object(lept_value& v);
	static void clear_object(lept_value& v);
	static const char* get_object_key(const lept_value& v, size_t index);
	static size_t get_object_key_length(const lept_value& v, size_t index);
	static lept_value& get_object_value(const lept_value& v, size_t index);
	static size_t find_object_index(const lept_value& v, const char* key, size_t klen);
	static lept_value& find_object_value(lept_value& v, const char* key, size_t klen);
	static lept_value& set_object_value(lept_value& v, const char* key, size_t klen);
	static void remove_object_value(lept_value& v, size_t index);
private:
	static void parse_whitespace(lept_context& c);
	static int parse_literal(lept_context& c, lept_value& v, const char* literal, lept_type type);
	static int parse_value(lept_context& c, lept_value& v);
	static int parse_number(lept_context& c, lept_value& v);
	static int parse_string(lept_context& c, lept_value& v);
	static int parse_string_raw(lept_context& c, char** str, size_t& v);	// 解析 JSON 字符串，把结果写入 str 和 len，str 指向 c.stack中的元素
	static int parse_array(lept_context& c, lept_value& v);
	static int parse_object(lept_context& c, lept_value& v);
	static const char* parse_hex4(const char* p, unsigned& u);
	static void encode_utf8(lept_context& c, unsigned u);
	static void lept_free(lept_value& v);
	static void stringify_string(lept_context& c, const char* s, size_t len);
	static void stringify_value(lept_context& c, const lept_value& v);
private:
	static constexpr char hex_digits[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
};

};

#endif
