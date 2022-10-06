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
		case '[': return parse_array(c, v);
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
	unsigned u, u2;
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
					case 'u':
					{
						if (!(p = parse_hex4(p, u))) {
							c.top = head;
							return LEPT_PARSE_INVALID_UNICODE_HEX;
						}
						if (u >= 0xD800 && u <= 0xDBFF) {
							if (*p++ != '\\') {
								c.top = head;
								return LEPT_PARSE_INVALID_UNICODE_SURROGATE;
							}
							if (*p++ != 'u') {
								c.top = head;
								return LEPT_PARSE_INVALID_UNICODE_SURROGATE;
							}
							if (!(p = parse_hex4(p, u2))) {
								c.top = head;
								return LEPT_PARSE_INVALID_UNICODE_HEX;
							}
							if (u2 < 0xDC00 || u2 > 0xDFFF) {
								c.top = head;
								return LEPT_PARSE_INVALID_UNICODE_SURROGATE;
							}
							u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
						}
						encode_utf8(c, u);
						break;
					}
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

const char* lept_json::parse_hex4(const char* p, unsigned& u) {
	u = 0;
	for (int i = 0; i < 4; ++i) {
		char ch = *p++;
		u <<= 4;
		if (ch >= '0' && ch <= '9') u |= ch - '0';
		else if (ch >= 'A' && ch <= 'F') u |= ch - ('A' - 10);
		else if (ch >= 'a' && ch <= 'f') u |= ch - ('a' - 10);
		else return nullptr;
	}
	return p;
}

void lept_json::encode_utf8(lept_context& c, unsigned u) {
	if (u <= 0x7F) PUTC(c, u & 0xFF);
	else if (u <= 0x7FF) {
		PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
		PUTC(c, 0x80 | ( u & 0x3F));
	} else if (u <= 0xFFFF) {
		PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
		PUTC(c, 0x80 | ((u >> 6) & 0x3F));
		PUTC(c, 0x80 | ( u & 0x3F));
	}
	else {
		assert(u <= 0x10FFFF);
		PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
		PUTC(c, 0x80 | ((u >> 12) & 0x3F));
		PUTC(c, 0x80 | ((u >> 6) & 0x3F));
		PUTC(c, 0x80 | ( u & 0x3F));
	}
}

int lept_json::parse_array(lept_context& c, lept_value& v) {
	size_t i, size = 0;
	int ret;
	EXPECT(c, '[');
	parse_whitespace(c);
	if (*c.json == ']') {
		c.json++;
		v.type = JSON_ARRAY;
		v.u.a.size = 0;
		v.u.a.e = nullptr;
		return LEPT_PARSE_OK;
	}

	for (;;) {
		lept_value e;
		init(e);
		if ((ret = parse_value(c, e)) != LEPT_PARSE_OK) break;
		memcpy(lept_context::push(c, sizeof(lept_value)), &e, sizeof(lept_value));
		++ size;
		parse_whitespace(c);
		if (*c.json == ',') {
			c.json++;
			parse_whitespace(c);
		} else if (*c.json == ']') {
			c.json++;
			v.type = JSON_ARRAY;
			v.u.a.size = size;
			size *= sizeof(lept_value);
			memcpy(v.u.a.e = (lept_value*)malloc(size), lept_context::pop(c, size), size);
			return LEPT_PARSE_OK;
		} else {
			ret = LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
			break;
		}
	}

	for (i = 0; i < size; ++i) {
		lept_free(*(lept_value*)lept_context::pop(c, sizeof(lept_value)));
	}
	return ret;
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

size_t lept_json::get_array_size(const lept_value& v) {
	assert(v.type == JSON_ARRAY);
	return v.u.a.size;
}

lept_value& lept_json::get_array_element(const lept_value& v, size_t index) {
	assert(v.type == JSON_ARRAY);
	assert(index < v.u.a.size);
	return v.u.a.e[index];
}

};
