#include "leptjson.h"

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

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
		case '"': return parse_string(c, v);
		case '[': return parse_array(c, v);
		case '{': return parse_object(c, v);
		case '\0': return LEPT_PARSE_EXPECT_VALUE;
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

int lept_json::parse_string_raw(lept_context& c, char** str, size_t& len) {
	size_t head = c.top;
	unsigned u, u2;
	const char* p;
	EXPECT(c, '\"');
	p = c.json;
	for (;;) {
		char ch = *p++;
		switch (ch) {
			case '\"' :
			{
				len = c.top - head;
				*str = (char*)lept_context::pop(c, len);
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

int lept_json::parse_string(lept_context& c, lept_value& v) {
	int ret;
	char* s;
	size_t len;
	if ((ret = parse_string_raw(c, &s, len)) == LEPT_PARSE_OK) {
		set_string(v, s, len);
	}
	return ret;
}

int lept_json::parse_object(lept_context& c, lept_value& v) {
	size_t i, size;
	lept_member m;
	int ret;
	EXPECT(c, '{');
	parse_whitespace(c);
	if (*c.json == '}') {
		c.json++;
		v.type = JSON_OBJECT;
		v.u.o.m = nullptr;
		v.u.o.size = 0;
		return LEPT_PARSE_OK;
	}
	m.k = nullptr;
	size = 0;
	for (;;) {
		char* str;
		init(m.v);
		if (*c.json != '"') {
			ret = LEPT_PARSE_MISS_KEY;
			break;
		}
		if ((ret = parse_string_raw(c, &str, m.klen)) != LEPT_PARSE_OK) break;
		memcpy(m.k = (char*)malloc(m.klen + 1), str, m.klen);
		m.k[m.klen] = '\0';
		parse_whitespace(c);
		if (*c.json != ':') {
			ret = LEPT_PARSE_MISS_COLON;
			break;
		}
		c.json++;
		parse_whitespace(c);
		if ((ret = parse_value(c, m.v)) != LEPT_PARSE_OK) break;
		memcpy(lept_context::push(c, sizeof(lept_member)), &m, sizeof(lept_member));
		++ size;
		m.k = nullptr;
		parse_whitespace(c);
		if (*c.json == ',') {
			c.json++;
			parse_whitespace(c);
		} else if (*c.json == '}') {
			size_t s = sizeof(lept_member) * size;
			c.json++;
			v.type = JSON_OBJECT;
			v.u.o.size = size;
			memcpy(v.u.o.m = (lept_member*)malloc(s), lept_context::pop(c, s), s);
			return LEPT_PARSE_OK;
		} else {
			ret = LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
			break;
		}
	}

	free(m.k);
	for (i = 0; i < size; ++i) {
		lept_member* m = (lept_member*)lept_context::pop(c, sizeof(lept_member));
		free(m->k);
		lept_free(m->v);
	}

	v.type = JSON_NULL;
	return ret;
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

size_t lept_json::get_object_size(const lept_value& v) {
	assert(v.type == JSON_OBJECT);
	return v.u.o.size;
}

const char* lept_json::get_object_key(const lept_value& v, size_t index) {
	assert(v.type == JSON_OBJECT);
	assert(index < v.u.o.size);
	return v.u.o.m[index].k;
}

size_t lept_json::get_object_key_length(const lept_value& v, size_t index) {
	assert(v.type == JSON_OBJECT);
	assert(index < v.u.o.size);
	return v.u.o.m[index].klen;
}

lept_value& lept_json::get_object_value(const lept_value& v, size_t index) {
	assert(v.type == JSON_OBJECT);
	assert(index < v.u.o.size);
	return v.u.o.m[index].v;
}

void lept_json::lept_free(lept_value& v) {
	size_t i;
	switch (v.type) {
		case JSON_STRING:
			free(v.u.s.s);
			break;
		case JSON_ARRAY:
			for (i = 0; i < v.u.a.size; ++i)
				lept_free(v.u.a.e[i]);
			free(v.u.a.e);
			break;
		case JSON_OBJECT:
			for (i = 0; i < v.u.o.size; ++i) {
				free(v.u.o.m[i].k);
				lept_free(v.u.o.m[i].v);
			}
			free(v.u.o.m);
			break;
		default:
			break;
	}
	v.type = JSON_NULL;
}

void lept_json::stringify_string(lept_context& c, const char* s, size_t len) {
	size_t i, size;
	char* head, *p;
	assert(s != nullptr);
	p = head = (char*)lept_context::push(c, (size = len * 6 + 2));	// "\u00xx..."
	*p++ = '"';
	for (i = 0; i < len; ++i) {
		unsigned char ch = (unsigned char)s[i];
		switch (ch) {
			case '\"': *p++ = '\\'; *p++ = '\"'; break;
			case '\\': *p++ = '\\'; *p++ = '\\'; break;
			case '\b': *p++ = '\\'; *p++ = 'b';	 break;
			case '\f': *p++ = '\\'; *p++ = 'f';  break;
			case '\n': *p++ = '\\'; *p++ = 'n';  break;
			case '\r': *p++ = '\\'; *p++ = 'r';  break;
			case '\t': *p++ = '\\'; *p++ = 't';  break;
			default:
				if (ch < 0x20) {
					*p++ = '\\'; *p++ = 'u'; *p++ = '0'; *p++ = '0';
					*p++ = hex_digits[ch >> 4];
					*p++ = hex_digits[ch & 15];
				} else {
					*p++ = s[i];
				}
		}
	}
	*p++ = '"';
	c.top -= size - (p - head);
}

void lept_json::stringify_value(lept_context& c, const lept_value& v) {
	size_t i;
	switch (v.type) {
		case JSON_NULL: PUTS(c, "null", 4); break;
		case JSON_TRUE: PUTS(c, "true", 4); break;
		case JSON_FALSE: PUTS(c, "false", 5); break;
		case JSON_NUMBER: c.top -= 32 - sprintf((char*)lept_context::push(c, 32), "%.17g", v.u.n); break;
		case JSON_STRING: stringify_string(c, v.u.s.s, v.u.s.len); break;
		case JSON_ARRAY:
			PUTC(c, '[');
			for (i = 0; i < v.u.a.size; ++i) {
				if (i > 0) PUTC(c, ',');
				stringify_value(c, v.u.a.e[i]);
			}
			PUTC(c, ']');
			break;
		case JSON_OBJECT:
			PUTC(c, '{');
			for (i = 0; i < v.u.o.size; ++i) {
				if (i > 0) PUTC(c, ',');
				stringify_string(c, v.u.o.m[i].k, v.u.o.m[i].klen);
				PUTC(c, ':');
				stringify_value(c, v.u.o.m[i].v);
			}
			PUTC(c, '}');
			break;
		default: assert(0 && "invalid type");
	}
}

char* lept_json::stringify(const lept_value& v, size_t& length) {
	lept_context c;
	c.stack = (char*)malloc(c.size = LEPT_PARSE_STRINGIFY_INIT_SIZE);
	c.top = 0;
	stringify_value(c, v);
	length = c.top;
	PUTC(c, '\0');
	return c.stack;
}

void lept_json::copy(lept_value& dst, const lept_value& src) {
	assert(&dst != &src);
	switch (src.type) {
		case JSON_STRING:
			set_string(dst, src.u.s.s, src.u.s.len);
			break;
		case JSON_ARRAY:
			set_array(dst, src.u.a.size);
			for (size_t i = 0; i < src.u.a.size; ++i)
				copy(dst.u.a.e[i], src.u.a.e[i]);
			dst.u.a.size = src.u.a.size;
			break;
		case JSON_OBJECT:
			set_object(dst, src.u.o.size);
			for (size_t i = 0; i < src.u.o.size; ++i) {
				lept_value& val = set_object_value(dst, src.u.o.m[i].k, src.u.o.m[i].klen);
				copy(val, src.u.o.m[i].v);
			}
			dst.u.o.size = src.u.o.size;
			break;
		default:
			set_null(dst);
			memcpy(&dst, &src, sizeof(lept_value));
			break;
	}
}

void lept_json::move(lept_value& dst, lept_value& src) {
	assert(&dst != &src);
	set_null(dst);
	memcpy(&dst, &src, sizeof(lept_value));
	init(src);
}

void lept_json::swap(lept_value& lhs, lept_value& rhs) {
	if (&lhs != &rhs) {
		lept_value tmp;
		memcpy(&tmp, &lhs, sizeof(lept_value));
		memcpy(&lhs, &rhs, sizeof(lept_value));
		memcpy(&rhs, &tmp, sizeof(lept_value));
	}
}

bool lept_json::is_equal(const lept_value& lhs, const lept_value& rhs) {
	if (lhs.type != rhs.type) return false;
	switch (lhs.type) {
		case JSON_STRING:
			return lhs.u.s.len == rhs.u.s.len && memcmp(lhs.u.s.s, rhs.u.s.s, lhs.u.s.len) == 0;
		case JSON_NUMBER:
			return lhs.u.n == rhs.u.n;
		case JSON_ARRAY:
			if (lhs.u.a.size != rhs.u.a.size) return false;
			for (size_t i = 0; i < lhs.u.a.size; ++i) {
				if (!is_equal(lhs.u.a.e[i], rhs.u.a.e[i])) return false;
			}
			return true;
		case JSON_OBJECT:
			if (lhs.u.o.size != rhs.u.o.size) return false;
			for (size_t i = 0; i < lhs.u.o.size; ++i) {
				size_t index = find_object_index(rhs, lhs.u.o.m[i].k, lhs.u.o.m[i].klen);
				if (index == LEPT_KEY_NOT_EXIST) return false;
				if (!is_equal(lhs.u.o.m[i].v, rhs.u.o.m[index].v)) return false;
			}
			return true;
		default:
			return true;
	}
}

size_t lept_json::get_array_capacity(const lept_value& v) {
	assert(v.type == JSON_ARRAY);
	return v.u.a.capacity;
}

void lept_json::reserve_array(lept_value& v, size_t capacity) {
	assert(v.type == JSON_ARRAY);
	if (v.u.a.capacity < capacity) {
		v.u.a.e = (lept_value*)realloc(v.u.a.e, capacity * sizeof(lept_value));
		v.u.a.capacity = capacity;
	}
}

void lept_json::shrink_array(lept_value& v) {
	assert(v.type == JSON_ARRAY);
	if (v.u.a.capacity > v.u.a.size) {
		v.u.a.e = (lept_value*)realloc(v.u.a.e, v.u.a.size * sizeof(lept_value));
		v.u.a.capacity = v.u.a.size;
	}
}

void lept_json::clear_array(lept_value& v) {
	assert(v.type == JSON_ARRAY);
	erase_array_element(v, 0, v.u.a.size);
}

lept_value& lept_json::pushback_array_element(lept_value& v) {
	assert(v.type == JSON_ARRAY);
	if (v.u.a.size == v.u.a.capacity) {
		reserve_array(v, v.u.a.capacity == 0 ? 1 : v.u.a.capacity * 2);
	}
	init(v.u.a.e[v.u.a.size]);
	return v.u.a.e[v.u.a.size++];
}

void lept_json::popback_array_element(lept_value& v) {
	assert(v.type == JSON_ARRAY && v.u.a.size > 0);
	set_null(v.u.a.e[--v.u.a.size]);
}

lept_value& lept_json::insert_array_element(lept_value& v, size_t index) {
	assert(v.type == JSON_ARRAY && index <= v.u.a.size);
	if (v.u.a.size == v.u.a.capacity) reserve_array(v, v.u.a.capacity == 0 ? 1 : (v.u.a.size << 1));
	memcpy(&v.u.a.e[index + 1], &v.u.a.e[index], (v.u.a.size - index) * sizeof(lept_value));
	init(v.u.a.e[index]);
	++v.u.a.size;
	return v.u.a.e[index];
}

void lept_json::erase_array_element(lept_value& v, size_t index, size_t count) {
	assert(v.type == JSON_ARRAY && index + count <= v.u.a.size);
	for (size_t i = index; i < index + count; ++i) {
		set_null(v.u.a.e[i]);
	}
	memcpy(v.u.a.e + index, v.u.a.e + index + count, (v.u.a.size - index - count) * sizeof(lept_value));
	for (size_t i = v.u.a.size - count; i < v.u.a.size; ++i) {
		init(v.u.a.e[i]);
	}
	v.u.a.size -= count;
}

void lept_json::set_array(lept_value& v, size_t capacity) {
	set_null(v);
	v.type = JSON_ARRAY;
	v.u.a.size = 0;
	v.u.a.capacity = capacity > 0 ? capacity : 0;
	v.u.a.e = capacity > 0 ? (lept_value*)malloc(capacity * sizeof(lept_value)) : nullptr;
}

void lept_json::set_object(lept_value& v, size_t capacity) {
	lept_free(v);
	v.type = JSON_OBJECT;
	v.u.o.size = 0;
	v.u.o.capacity = capacity > 0 ? capacity : 0;
	v.u.o.m = capacity > 0 ? (lept_member*)malloc(capacity * sizeof(lept_member)) : nullptr;
}

size_t lept_json::get_object_capacity(const lept_value& v) {
	assert(v.type == JSON_OBJECT);
	return v.u.o.capacity;
}

void lept_json::reserve_object(lept_value& v, size_t capacity) {
	assert(v.type == JSON_OBJECT);
	if (v.u.o.capacity < capacity) {
		v.u.o.m = (lept_member*)realloc(v.u.o.m, capacity * sizeof(lept_member));
		v.u.o.capacity = capacity;
	}
}

void lept_json::shrink_object(lept_value& v) {
	assert(v.type == JSON_OBJECT);
	if (v.u.o.capacity > v.u.o.size) {
		v.u.o.m = (lept_member*)realloc(v.u.o.m, v.u.o.size * sizeof(lept_member));
		v.u.o.capacity = v.u.o.size;
	}
}

void lept_json::clear_object(lept_value& v) {
	assert(v.type == JSON_OBJECT);
	for (size_t i = 0; i < v.u.o.size; ++i) {
		free(v.u.o.m[i].k);
		v.u.o.m[i].k = nullptr;
		v.u.o.m[i].klen = 0;
		lept_free(v.u.o.m[i].v);
	}
	v.u.o.size = 0;
}

size_t lept_json::find_object_index(const lept_value& v, const char* key, size_t klen) {
	assert(v.type == JSON_OBJECT && key != nullptr);
	for (size_t i = 0; i < v.u.o.size; ++i) {
		if (v.u.o.m[i].klen == klen && memcmp(v.u.o.m[i].k, key, klen) == 0) return i;
	}
	return LEPT_KEY_NOT_EXIST;
}

lept_value& lept_json::find_object_value(lept_value& v, const char* key, size_t klen) {
	size_t index = find_object_index(v, key, klen);
	assert(index != LEPT_KEY_NOT_EXIST);
	return v.u.o.m[index].v;
}

lept_value& lept_json::set_object_value(lept_value& v, const char* key, size_t klen) {
	assert(v.type == JSON_OBJECT && key != nullptr);
	size_t index = find_object_index(v, key, klen);
	if (index != LEPT_KEY_NOT_EXIST) return v.u.o.m[index].v;
	if (v.u.o.size == v.u.o.capacity) reserve_object(v, v.u.o.capacity == 0 ? 1 : (v.u.o.capacity << 1));
	size_t flag = v.u.o.size;
	v.u.o.m[flag].k = (char*)malloc(klen + 1);
	memcpy(v.u.o.m[flag].k, key, klen);
	v.u.o.m[flag].k[klen] = '\0';
	v.u.o.m[flag].klen = klen;
	init(v.u.o.m[flag].v);
	++ v.u.o.size;
	return v.u.o.m[flag].v;
}

void lept_json::remove_object_value(lept_value& v, size_t index) {
	assert(v.type == JSON_OBJECT && index < v.u.o.size);
	free(v.u.o.m[index].k);
	lept_free(v.u.o.m[index].v);
	memcpy(v.u.o.m + index, v.u.o.m + index + 1, (v.u.o.size - index - 1) * sizeof(lept_member));
	v.u.o.m[--v.u.o.size].k = nullptr;
	v.u.o.m[v.u.o.size].klen = 0;
	init(v.u.o.m[v.u.o.size].v);
}

};
