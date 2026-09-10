#include "json.h"

#include <stdlib.h>
#include <string.h>

namespace {

struct P {
    const char *p, *e;
    std::string err;

    void ws() { while (p < e && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++; }
    bool fail(const char *m) { if (err.empty()) err = m; return false; }

    bool str(std::string *out) {
        if (p >= e || *p != '"') return fail("expected string");
        p++;
        out->clear();
        while (p < e && *p != '"') {
            if (*p != '\\') { out->push_back(*p++); continue; }
            if (++p >= e) return fail("truncated escape");
            char c = *p++;
            switch (c) {
                case '"':  out->push_back('"');  break;
                case '\\': out->push_back('\\'); break;
                case '/':  out->push_back('/');  break;
                case 'b':  out->push_back('\b'); break;
                case 'f':  out->push_back('\f'); break;
                case 'n':  out->push_back('\n'); break;
                case 'r':  out->push_back('\r'); break;
                case 't':  out->push_back('\t'); break;
                case 'u': {
                    if (e - p < 4) return fail("truncated \\u");
                    unsigned cp = (unsigned)strtoul(std::string(p, p + 4).c_str(), nullptr, 16);
                    p += 4;
                    if (cp >= 0xd800 && cp < 0xdc00 && e - p >= 6 && p[0] == '\\' && p[1] == 'u') {
                        unsigned lo = (unsigned)strtoul(std::string(p + 2, p + 6).c_str(), nullptr, 16);
                        if (lo >= 0xdc00 && lo < 0xe000) {
                            cp = 0x10000 + ((cp - 0xd800) << 10) + (lo - 0xdc00);
                            p += 6;
                        }
                    }
                    if (cp < 0x80) out->push_back((char)cp);
                    else if (cp < 0x800) {
                        out->push_back((char)(0xc0 | (cp >> 6)));
                        out->push_back((char)(0x80 | (cp & 0x3f)));
                    } else if (cp < 0x10000) {
                        out->push_back((char)(0xe0 | (cp >> 12)));
                        out->push_back((char)(0x80 | ((cp >> 6) & 0x3f)));
                        out->push_back((char)(0x80 | (cp & 0x3f)));
                    } else {
                        out->push_back((char)(0xf0 | (cp >> 18)));
                        out->push_back((char)(0x80 | ((cp >> 12) & 0x3f)));
                        out->push_back((char)(0x80 | ((cp >> 6) & 0x3f)));
                        out->push_back((char)(0x80 | (cp & 0x3f)));
                    }
                    break;
                }
                default: return fail("bad escape");
            }
        }
        if (p >= e) return fail("unterminated string");
        p++;
        return true;
    }

    bool val(jval *v) {
        ws();
        if (p >= e) return fail("unexpected end");
        char c = *p;
        if (c == '"') { v->kind = jval::STR; return str(&v->s); }
        if (c == '{') {
            p++; v->kind = jval::OBJ;
            ws();
            if (p < e && *p == '}') { p++; return true; }
            for (;;) {
                std::string k;
                ws();
                if (!str(&k)) return false;
                ws();
                if (p >= e || *p != ':') return fail("expected :");
                p++;
                v->obj.emplace_back(k, jval());
                if (!val(&v->obj.back().second)) return false;
                ws();
                if (p < e && *p == ',') { p++; continue; }
                if (p < e && *p == '}') { p++; return true; }
                return fail("expected , or }");
            }
        }
        if (c == '[') {
            p++; v->kind = jval::ARR;
            ws();
            if (p < e && *p == ']') { p++; return true; }
            for (;;) {
                v->arr.emplace_back();
                if (!val(&v->arr.back())) return false;
                ws();
                if (p < e && *p == ',') { p++; continue; }
                if (p < e && *p == ']') { p++; return true; }
                return fail("expected , or ]");
            }
        }
        if (e - p >= 4 && !strncmp(p, "true", 4))  { p += 4; v->kind = jval::BOOL; v->b = true;  return true; }
        if (e - p >= 5 && !strncmp(p, "false", 5)) { p += 5; v->kind = jval::BOOL; v->b = false; return true; }
        if (e - p >= 4 && !strncmp(p, "null", 4))  { p += 4; v->kind = jval::NUL; return true; }

        const char *s0 = p;
        if (p < e && (*p == '-' || *p == '+')) p++;
        bool isint = true;
        while (p < e && ((*p >= '0' && *p <= '9') || *p == '.' || *p == 'e' || *p == 'E' ||
                         *p == '+' || *p == '-')) {
            if (*p == '.' || *p == 'e' || *p == 'E') isint = false;
            p++;
        }
        if (p == s0) return fail("bad value");
        std::string t(s0, p);
        if (isint) { v->kind = jval::INT; v->i = strtoll(t.c_str(), nullptr, 10); v->d = (double)v->i; }
        else       { v->kind = jval::NUM; v->d = strtod(t.c_str(), nullptr); }
        return true;
    }
};

} /* namespace */

bool json_parse(const char *p, size_t n, jval *out, std::string *err) {
    P s{p, p + n, std::string()};
    if (!s.val(out)) { if (err) *err = s.err; return false; }
    s.ws();
    if (s.p != s.e) { if (err) *err = "trailing bytes after JSON value"; return false; }
    return true;
}
