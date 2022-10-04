#include "leptjson.h"

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace leptjson {

void* lept_context::push(lept_context& c, size_t size) {
	void* ret;
	assert(size > 0);
	if (c.top + size >= c.size) {
		if (c.size == 0) c.size = LEPT_PARSE_STACK_INIT_SIZE;
		while (c.top + size >= c.size) c.size += c.size >> 1;
		c.stack = (char*)realloc(c.stack, c.size);
	}
	ret = c.stack + c.top;
	c.top += size;
	return ret;
}

void* lept_context::pop(lept_context& c, size_t size) {
	assert(c.top >= size);
	return c.stack + (c.top -= size);
}

const lept_type lept_json::get_type(const lept_value& v) {
	return v.type;
}

int lept_json::parse(lept_value& v, const char* json) {
	lept_context c;
	int ret;
	c.json = json;
	c.stack = nullptr;
	c.size = c.top = 0;
	init(v);
	parse_whitespace(c);
	if ((ret = parse_value(c, v)) == LEPT_PARSE_OK) {
		parse_whitespace(c);
		if (*c.json != '\0') {
			v.type = JSON_NULL;
			ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
		}
	}
	assert(c.top == 0);
	free(c.stack);
	return ret;
}

void lept_json::parse_whitespace(lept_context& c) {
	const char* p = c.json;
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') ++p;
	c.json = p;
}

int lept_json::parse_literal(lept_context& c, lept_value& v, const char* literal, lept_type type) {
	size_t i;
	EXPECT(c, literal[0]);
	for (i = 0; literal[i + 1]; ++i) {
		if (c.json[i] != literal[i + 1])
			return LEPT_PARSE_INVALID_VALUE;
	}
	c.json += i;
	v.type = type;
	return LEPT_PARSE_OK;
}

int lept_json::parse_value(lept_context& c, lept_value& v) {
	switch (*c.json) {
		case 't': return parse_literal(c, v, "true", JSON_TRUE);
		case 'f': return parse_literal(c, v, "false", JSON_FALSE);
		case 'n': return parse_literal(c, v, "null", JSON_NULL);
		case '\0': return LEPT_PARSE_EXPECT_VALUE;
		case '"': return parse_string(c, v);
		default: return parse_number(c, v);
	}
}

int lept_json::parse_number(lept_context& c, lept_value& v) {
	const char* p = c.json;
	if (*p == '-') ++p;
	if (*p == '0') ++p;
	else {
		if (!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
		for (++p; ISDIGIT(*p); ++p);
	}
	if (*p == '.') {
		++p;
		if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
		for (++p; ISDIGIT(*p); ++p);
	}
	if (*p == 'e' || *p == 'E') {
		++p;
		if (*p == '+' || *p == '-') ++p;
		if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
		for (++p; ISDIGIT(*p); ++p);
	}
	errno = 0;
	v.u.n = strtod(c.json, nullptr);
	if (errno == ERANGE && (v.u.n == HUGE_VAL || v.u.n == -HUGE_VAL))
		return LEPT_PARSE_NUMBER_TOO_BIG;
	v.type = JSON_NUMBER;
	c.json = p;
	return LEPT_PARSE_OK;
}

int lept_json::parse_string(lept_context& c, lept_value& v) {
	size_t head = c.top, len;
	const char* p;
	EXPECT(c, '\"');
	p = c.json;
	for (;;) {
		char ch = *p++;
		switch (ch) {
			case '\"' :
			{
				len = c.top - head;
				set_string(v, (const char*)lept_context::pop(c, len), len);
				c.json = p;
				return LEPT_PARSE_OK;
			}
			case '\0' :
			{
				c.top = head;
				return LEPT_PARSE_MISS_QUOTATION_MARK;
			}
			case '\\' :
			{
				switch (*p++)
				{
					case '\"': PUTC(c, '\"'); break;
					case '\\': PUTC(c, '\\'); break;
					case '/':  PUTC(c, '/' ); break;
					case 'b':  PUTC(c, '\b'); break;
					case 'f':  PUTC(c, '\f'); break;
					case 'n':  PUTC(c, '\n'); break;
					case 'r':  PUTC(c, '\r'); break;
					case 't':  PUTC(c, '\t'); break;
					default:
						c.top = head;
						return LEPT_PARSE_INVALID_STRING_ESCAPE;
				}
				break;
			}
			default :
			{
				if ((unsigned char)ch < 0x20) {
					c.top = head;
					return LEPT_PARSE_INVALID_STRING_CHAR;
				}
				PUTC(c, ch);
			}
		}
	}
}

int lept_json::get_boolean(const lept_value& v) {
	assert(v.type == JSON_TRUE || v.type == JSON_FALSE);
	return v.type == JSON_TRUE;
}

void lept_json::set_boolean(lept_value& v, int b) {
	set_null(v);
	v.type = b ? JSON_TRUE : JSON_FALSE;
}

double lept_json::get_number(const lept_value& v) {
	assert(v.type == JSON_NUMBER);
	return v.u.n;
}

void lept_json::set_number(lept_value& v, double n) {
	set_null(v);
	v.u.n = n;
	v.type = JSON_NUMBER;
}

const char* lept_json::get_string(const lept_value& v) {
	assert(v.type == JSON_STRING);
	return v.u.s.s;
}

size_t lept_json::get_string_length(const lept_value& v) {
	assert(v.type == JSON_STRING);
	return v.u.s.len;
}

void lept_json::set_string(lept_value& v, const char* s, size_t len) {
	assert(s != nullptr || len == 0);
	set_null(v);
	v.u.s.s = (char*)malloc(len + 1);
	memcpy(v.u.s.s, s, len);
	v.u.s.s[len] = '\0';
	v.u.s.len = len;
	v.type = JSON_STRING;
}

};
