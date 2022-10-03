#include "leptjson.h"

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>

namespace leptjson {

const lept_type lept_json::get_type(const lept_value& v) {
	return v.type;
}

int lept_json::parse(lept_value& v, const char* json) {
	lept_context c;
	int ret;
	c.json = json;
	v.type = JSON_NULL;
	parse_whitespace(c);
	if ((ret = parse_value(c, v)) == LEPT_PARSE_OK) {
		parse_whitespace(c);
		if (*c.json != '\0') {
			v.type = JSON_NULL;
			ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
		}
	}
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
		default: return parse_number(c, v);
	}
}

double lept_json::get_number(const lept_value& v) {
	assert(v.type == JSON_NUMBER);
	return v.n;
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
	v.n = strtod(c.json, nullptr);
	if (errno == ERANGE && (v.n == HUGE_VAL || v.n == -HUGE_VAL))
		return LEPT_PARSE_NUMBER_TOO_BIG;
	v.type = JSON_NUMBER;
	c.json = p;
	return LEPT_PARSE_OK;
}

};
