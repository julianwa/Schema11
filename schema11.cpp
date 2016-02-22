/* Copyright (c) 2013 Dropbox, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "schema11.hpp"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <limits>
#include "third_party/json11/json11.hpp"

namespace schema11 {

static const int max_depth = 200;

using std::string;
using std::vector;
using std::map;
using std::make_shared;
using std::initializer_list;
using std::move;
using json11::Json;

// /* * * * * * * * * * * * * * * * * * * *
//  * Serialization
//  */
//
static void dump(std::nullptr_t, string &out) {
    out += "null";
}

static void dump(double value, string &out) {
    if (std::isfinite(value)) {
        char buf[32];
        snprintf(buf, sizeof buf, "%.17g", value);
        out += buf;
    } else {
        out += "null";
    }
}

static void dump(int value, string &out) {
    char buf[32];
    snprintf(buf, sizeof buf, "%d", value);
    out += buf;
}

static void dump(bool value, string &out) {
    out += value ? "true" : "false";
}

static void dump(const string &value, string &out) {
    out += '"';
    for (size_t i = 0; i < value.length(); i++) {
        const char ch = value[i];
        if (ch == '\\') {
            out += "\\\\";
        } else if (ch == '"') {
            out += "\\\"";
        } else if (ch == '\b') {
            out += "\\b";
        } else if (ch == '\f') {
            out += "\\f";
        } else if (ch == '\n') {
            out += "\\n";
        } else if (ch == '\r') {
            out += "\\r";
        } else if (ch == '\t') {
            out += "\\t";
        } else if (static_cast<uint8_t>(ch) <= 0x1f) {
            char buf[8];
            snprintf(buf, sizeof buf, "\\u%04x", ch);
            out += buf;
        } else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(value[i+1]) == 0x80
                   && static_cast<uint8_t>(value[i+2]) == 0xa8) {
            out += "\\u2028";
            i += 2;
        } else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(value[i+1]) == 0x80
                   && static_cast<uint8_t>(value[i+2]) == 0xa9) {
            out += "\\u2029";
            i += 2;
        } else {
            out += ch;
        }
    }
    out += '"';
}

static void dump(const Schema::array &values, string &out) {
    bool first = true;
    out += "[";
    for (const auto &value : values) {
        if (!first)
            out += ", ";
        value.dump(out);
        first = false;
    }
    out += "]";
}

static void dump(const Schema::object &values, string &out) {
    bool first = true;
    out += "{";
    for (const auto &kv : values) {
        if (!first)
            out += ", ";
        dump(kv.first, out);
        out += ": ";
        // TODO:
        // kv.second.dump(out);
        first = false;
    }
    out += "}";
}

void Schema::dump(string &out) const {
    m_ptr->dump(out);
}

/* * * * * * * * * * * * * * * * * * * *
 * Value wrappers
 */

template <Schema::Type tag>
class Value : public SchemaValue {
protected:

    // Constructors
    explicit Value() {}

    // Get type tag
    Schema::Type type() const override {
        return tag;
    }

    // Comparisons
    bool equals(const SchemaValue * other) const override {
        return type() == static_cast<const Value<tag> *>(other)->type();
        //return m_value == static_cast<const Value<tag, T> *>(other)->m_value;
    }
    bool less(const SchemaValue * other) const override {
        return type() < static_cast<const Value<tag> *>(other)->type();
        //return m_value < static_cast<const Value<tag, T> *>(other)->m_value;
    }

    // const T m_value;
    void dump(string &out) const override { /*schema11::dump(m_value, out); */ }
};

class SchemaNumber final : public Value<Schema::NUMBER> {
    // double number_value() const override { return m_value; }
    // int int_value() const override { return static_cast<int>(m_value); }
    // bool equals(const SchemaValue * other) const override { return m_value == other->number_value(); }
    // bool less(const SchemaValue * other)   const override { return m_value <  other->number_value(); }
// public:
    // explicit SchemaDouble(double value) : Value() {}
};

class SchemaBoolean final : public Value<Schema::BOOL> {
    // bool bool_value() const override { return m_value; }
// public:
    // explicit SchemaBoolean(bool value) : Value(value) {}
};

class SchemaString final : public Value<Schema::STRING> {
//     const string &string_value() const override { return m_value; }
// public:
//     explicit SchemaString(const string &value) : Value(value) {}
//     explicit SchemaString(string &&value)      : Value(move(value)) {}
};
//
// class SchemaArray final : public Value<Schema::ARRAY> {
//     const Schema::array &array_items() const override { return m_value; }
//     const Schema & operator[](size_t i) const override;
// public:
//     explicit SchemaArray(const Schema::array &value) : Value(value) {}
//     explicit SchemaArray(Schema::array &&value)      : Value(move(value)) {}
// };

class SchemaObject final : public Value<Schema::OBJECT> {
    const Schema::object &object_items() const override { return m_value; }
    const Schema & operator[](const string &key) const override;
public:
    explicit SchemaObject(const Schema::object &value) : Value(), m_value(value) {}
    explicit SchemaObject(Schema::object &&value) : Value(), m_value(move(value)) {}
    
    Schema::object m_value;
};

class SchemaNull final : public Value<Schema::NUL> {
public:
    SchemaNull() : Value() {}
};

/* * * * * * * * * * * * * * * * * * * *
 * Static globals - static-init-safe
 */
struct Statics {
    const std::shared_ptr<SchemaValue> null = make_shared<SchemaNull>();
    // const std::shared_ptr<SchemaValue> t = make_shared<SchemaBoolean>(true);
    // const std::shared_ptr<SchemaValue> f = make_shared<SchemaBoolean>(false);
    const string empty_string;
    const vector<Schema> empty_vector;
    const map<string, Schema> empty_map;
    Statics() {}
};

static const Statics & statics() {
    static const Statics s {};
    return s;
}

static const Schema & static_null() {
    // This has to be separate, not in Statics, because Schema() accesses statics().null.
    static const Schema json_null;
    return json_null;
}

/* * * * * * * * * * * * * * * * * * * *
 * Constructors
 */

Schema::Schema() noexcept                  : m_ptr(statics().null) {}
Schema::Schema(std::nullptr_t) noexcept    : m_ptr(statics().null) {}
Schema::Schema(Schema::Type type) {
    switch (type) {
        case Schema::Type::NUMBER: m_ptr = make_shared<SchemaNumber>(); break;
        case Schema::Type::BOOL: m_ptr = make_shared<SchemaBoolean>(); break;
        case Schema::Type::STRING: m_ptr = make_shared<SchemaString>(); break;
        default:
            assert(0);
    }
}
Schema::Schema(Schema::Type type, ValueConverter converter) : Schema(type) {
}
// Schema::Schema(const Schema::array &values)  : m_ptr(make_shared<SchemaArray>(values)) {}
// Schema::Schema(Schema::array &&values)       : m_ptr(make_shared<SchemaArray>(move(values))) {}
Schema::Schema(const Schema::object &values) : m_ptr(make_shared<SchemaObject>(values)) {}
Schema::Schema(Schema::object &&values)      : m_ptr(make_shared<SchemaObject>(move(values))) {}

/* * * * * * * * * * * * * * * * * * * *
 * Accessors
 */

Schema::Type Schema::type()                           const { return m_ptr->type();         }
// double Schema::number_value()                       const { return m_ptr->number_value(); }
// int Schema::int_value()                             const { return m_ptr->int_value();    }
// bool Schema::bool_value()                           const { return m_ptr->bool_value();   }
// const string & Schema::string_value()               const { return m_ptr->string_value(); }
// const vector<Schema> & Schema::array_items()          const { return m_ptr->array_items();  }
const map<string, Schema> & Schema::object_items()    const { return m_ptr->object_items(); }
const Schema & Schema::operator[] (size_t i)          const { return (*m_ptr)[i];           }
const Schema & Schema::operator[] (const string &key) const { return (*m_ptr)[key];         }

// double                    SchemaValue::number_value()              const { return 0; }
// int                       SchemaValue::int_value()                 const { return 0; }
// bool                      SchemaValue::bool_value()                const { return false; }
// const string &            SchemaValue::string_value()              const { return statics().empty_string; }
// const vector<Schema> &      SchemaValue::array_items()               const { return statics().empty_vector; }
const map<string, Schema> & SchemaValue::object_items()              const { return statics().empty_map; }
const Schema &              SchemaValue::operator[] (size_t)         const { return static_null(); }
const Schema &              SchemaValue::operator[] (const string &) const { return static_null(); }

const Schema & SchemaObject::operator[] (const string &key) const {
    auto iter = m_value.find(key);
    return (iter == m_value.end()) ? static_null() : iter->second;
}
// const Schema & SchemaArray::operator[] (size_t i) const {
//     if (i >= m_value.size()) return static_null();
//     else return m_value[i];
// }

/* * * * * * * * * * * * * * * * * * * *
 * Comparison
 */

bool Schema::operator== (const Schema &other) const {
    if (m_ptr->type() != other.m_ptr->type())
        return false;

    return m_ptr->equals(other.m_ptr.get());
}

bool Schema::operator< (const Schema &other) const {
    if (m_ptr->type() != other.m_ptr->type())
        return m_ptr->type() < other.m_ptr->type();

    return m_ptr->less(other.m_ptr.get());
}

/* * * * * * * * * * * * * * * * * * * *
 * Parsing
 */

/* esc(c)
 *
 * Format char c suitable for printing in an error message.
 */
// static inline string esc(char c) {
//     char buf[12];
//     if (static_cast<uint8_t>(c) >= 0x20 && static_cast<uint8_t>(c) <= 0x7f) {
//         snprintf(buf, sizeof buf, "'%c' (%d)", c, c);
//     } else {
//         snprintf(buf, sizeof buf, "(%d)", c);
//     }
//     return string(buf);
// }
//
// static inline bool in_range(long x, long lower, long upper) {
//     return (x >= lower && x <= upper);
// }
//
// /* SchemaParser
//  *
//  * Object that tracks all state of an in-progress parse.
//  */
// struct SchemaParser {
//
//     /* State
//      */
//     const string &str;
//     size_t i;
//     string &err;
//     bool failed;
//     const SchemaParse strategy;
//
//     /* fail(msg, err_ret = Schema())
//      *
//      * Mark this parse as failed.
//      */
//     Schema fail(string &&msg) {
//         return fail(move(msg), Schema());
//     }
//
//     template <typename T>
//     T fail(string &&msg, const T err_ret) {
//         if (!failed)
//             err = std::move(msg);
//         failed = true;
//         return err_ret;
//     }
//
//     /* consume_whitespace()
//      *
//      * Advance until the current character is non-whitespace.
//      */
//     void consume_whitespace() {
//         while (str[i] == ' ' || str[i] == '\r' || str[i] == '\n' || str[i] == '\t')
//             i++;
//     }
//
//     /* consume_comment()
//      *
//      * Advance comments (c-style inline and multiline).
//      */
//     bool consume_comment() {
//       bool comment_found = false;
//       if (str[i] == '/') {
//         i++;
//         if (i == str.size())
//           return fail("unexpected end of input inside comment", 0);
//         if (str[i] == '/') { // inline comment
//           i++;
//           if (i == str.size())
//             return fail("unexpected end of input inside inline comment", 0);
//           // advance until next line
//           while (str[i] != '\n') {
//             i++;
//             if (i == str.size())
//               return fail("unexpected end of input inside inline comment", 0);
//           }
//           comment_found = true;
//         }
//         else if (str[i] == '*') { // multiline comment
//           i++;
//           if (i > str.size()-2)
//             return fail("unexpected end of input inside multi-line comment", 0);
//           // advance until closing tokens
//           while (!(str[i] == '*' && str[i+1] == '/')) {
//             i++;
//             if (i > str.size()-2)
//               return fail(
//                 "unexpected end of input inside multi-line comment", 0);
//           }
//           i += 2;
//           if (i == str.size())
//             return fail(
//               "unexpected end of input inside multi-line comment", 0);
//           comment_found = true;
//         }
//         else
//           return fail("malformed comment", 0);
//       }
//       return comment_found;
//     }
//
//     /* consume_garbage()
//      *
//      * Advance until the current character is non-whitespace and non-comment.
//      */
//     void consume_garbage() {
//       consume_whitespace();
//       if(strategy == SchemaParse::COMMENTS) {
//         bool comment_found = false;
//         do {
//           comment_found = consume_comment();
//           consume_whitespace();
//         }
//         while(comment_found);
//       }
//     }
//
//     /* get_next_token()
//      *
//      * Return the next non-whitespace character. If the end of the input is reached,
//      * flag an error and return 0.
//      */
//     char get_next_token() {
//         consume_garbage();
//         if (i == str.size())
//             return fail("unexpected end of input", 0);
//
//         return str[i++];
//     }
//
//     /* encode_utf8(pt, out)
//      *
//      * Encode pt as UTF-8 and add it to out.
//      */
//     void encode_utf8(long pt, string & out) {
//         if (pt < 0)
//             return;
//
//         if (pt < 0x80) {
//             out += static_cast<char>(pt);
//         } else if (pt < 0x800) {
//             out += static_cast<char>((pt >> 6) | 0xC0);
//             out += static_cast<char>((pt & 0x3F) | 0x80);
//         } else if (pt < 0x10000) {
//             out += static_cast<char>((pt >> 12) | 0xE0);
//             out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
//             out += static_cast<char>((pt & 0x3F) | 0x80);
//         } else {
//             out += static_cast<char>((pt >> 18) | 0xF0);
//             out += static_cast<char>(((pt >> 12) & 0x3F) | 0x80);
//             out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
//             out += static_cast<char>((pt & 0x3F) | 0x80);
//         }
//     }
//
//     /* parse_string()
//      *
//      * Parse a string, starting at the current position.
//      */
//     string parse_string() {
//         string out;
//         long last_escaped_codepoint = -1;
//         while (true) {
//             if (i == str.size())
//                 return fail("unexpected end of input in string", "");
//
//             char ch = str[i++];
//
//             if (ch == '"') {
//                 encode_utf8(last_escaped_codepoint, out);
//                 return out;
//             }
//
//             if (in_range(ch, 0, 0x1f))
//                 return fail("unescaped " + esc(ch) + " in string", "");
//
//             // The usual case: non-escaped characters
//             if (ch != '\\') {
//                 encode_utf8(last_escaped_codepoint, out);
//                 last_escaped_codepoint = -1;
//                 out += ch;
//                 continue;
//             }
//
//             // Handle escapes
//             if (i == str.size())
//                 return fail("unexpected end of input in string", "");
//
//             ch = str[i++];
//
//             if (ch == 'u') {
//                 // Extract 4-byte escape sequence
//                 string esc = str.substr(i, 4);
//                 // Explicitly check length of the substring. The following loop
//                 // relies on std::string returning the terminating NUL when
//                 // accessing str[length]. Checking here reduces brittleness.
//                 if (esc.length() < 4) {
//                     return fail("bad \\u escape: " + esc, "");
//                 }
//                 for (int j = 0; j < 4; j++) {
//                     if (!in_range(esc[j], 'a', 'f') && !in_range(esc[j], 'A', 'F')
//                             && !in_range(esc[j], '0', '9'))
//                         return fail("bad \\u escape: " + esc, "");
//                 }
//
//                 long codepoint = strtol(esc.data(), nullptr, 16);
//
//                 // JSON specifies that characters outside the BMP shall be encoded as a pair
//                 // of 4-hex-digit \u escapes encoding their surrogate pair components. Check
//                 // whether we're in the middle of such a beast: the previous codepoint was an
//                 // escaped lead (high) surrogate, and this is a trail (low) surrogate.
//                 if (in_range(last_escaped_codepoint, 0xD800, 0xDBFF)
//                         && in_range(codepoint, 0xDC00, 0xDFFF)) {
//                     // Reassemble the two surrogate pairs into one astral-plane character, per
//                     // the UTF-16 algorithm.
//                     encode_utf8((((last_escaped_codepoint - 0xD800) << 10)
//                                  | (codepoint - 0xDC00)) + 0x10000, out);
//                     last_escaped_codepoint = -1;
//                 } else {
//                     encode_utf8(last_escaped_codepoint, out);
//                     last_escaped_codepoint = codepoint;
//                 }
//
//                 i += 4;
//                 continue;
//             }
//
//             encode_utf8(last_escaped_codepoint, out);
//             last_escaped_codepoint = -1;
//
//             if (ch == 'b') {
//                 out += '\b';
//             } else if (ch == 'f') {
//                 out += '\f';
//             } else if (ch == 'n') {
//                 out += '\n';
//             } else if (ch == 'r') {
//                 out += '\r';
//             } else if (ch == 't') {
//                 out += '\t';
//             } else if (ch == '"' || ch == '\\' || ch == '/') {
//                 out += ch;
//             } else {
//                 return fail("invalid escape character " + esc(ch), "");
//             }
//         }
//     }
//
//     /* parse_number()
//      *
//      * Parse a double.
//      */
//     Schema parse_number() {
//         size_t start_pos = i;
//
//         if (str[i] == '-')
//             i++;
//
//         // Integer part
//         if (str[i] == '0') {
//             i++;
//             if (in_range(str[i], '0', '9'))
//                 return fail("leading 0s not permitted in numbers");
//         } else if (in_range(str[i], '1', '9')) {
//             i++;
//             while (in_range(str[i], '0', '9'))
//                 i++;
//         } else {
//             return fail("invalid " + esc(str[i]) + " in number");
//         }
//
//         if (str[i] != '.' && str[i] != 'e' && str[i] != 'E'
//                 && (i - start_pos) <= static_cast<size_t>(std::numeric_limits<int>::digits10)) {
//             return std::atoi(str.c_str() + start_pos);
//         }
//
//         // Decimal part
//         if (str[i] == '.') {
//             i++;
//             if (!in_range(str[i], '0', '9'))
//                 return fail("at least one digit required in fractional part");
//
//             while (in_range(str[i], '0', '9'))
//                 i++;
//         }
//
//         // Exponent part
//         if (str[i] == 'e' || str[i] == 'E') {
//             i++;
//
//             if (str[i] == '+' || str[i] == '-')
//                 i++;
//
//             if (!in_range(str[i], '0', '9'))
//                 return fail("at least one digit required in exponent");
//
//             while (in_range(str[i], '0', '9'))
//                 i++;
//         }
//
//         return std::strtod(str.c_str() + start_pos, nullptr);
//     }
//
//     /* expect(str, res)
//      *
//      * Expect that 'str' starts at the character that was just read. If it does, advance
//      * the input and return res. If not, flag an error.
//      */
//     Schema expect(const string &expected, Schema res) {
//         assert(i != 0);
//         i--;
//         if (str.compare(i, expected.length(), expected) == 0) {
//             i += expected.length();
//             return res;
//         } else {
//             return fail("parse error: expected " + expected + ", got " + str.substr(i, expected.length()));
//         }
//     }
//
//     /* parse_json()
//      *
//      * Parse a JSON object.
//      */
//     Schema parse_json(int depth) {
//         if (depth > max_depth) {
//             return fail("exceeded maximum nesting depth");
//         }
//
//         char ch = get_next_token();
//         if (failed)
//             return Schema();
//
//         if (ch == '-' || (ch >= '0' && ch <= '9')) {
//             i--;
//             return parse_number();
//         }
//
//         if (ch == 't')
//             return expect("true", true);
//
//         if (ch == 'f')
//             return expect("false", false);
//
//         if (ch == 'n')
//             return expect("null", Schema());
//
//         if (ch == '"')
//             return parse_string();
//
//         if (ch == '{') {
//             map<string, Schema> data;
//             ch = get_next_token();
//             if (ch == '}')
//                 return data;
//
//             while (1) {
//                 if (ch != '"')
//                     return fail("expected '\"' in object, got " + esc(ch));
//
//                 string key = parse_string();
//                 if (failed)
//                     return Schema();
//
//                 ch = get_next_token();
//                 if (ch != ':')
//                     return fail("expected ':' in object, got " + esc(ch));
//
//                 data[std::move(key)] = parse_json(depth + 1);
//                 if (failed)
//                     return Schema();
//
//                 ch = get_next_token();
//                 if (ch == '}')
//                     break;
//                 if (ch != ',')
//                     return fail("expected ',' in object, got " + esc(ch));
//
//                 ch = get_next_token();
//             }
//             return data;
//         }
//
//         if (ch == '[') {
//             vector<Schema> data;
//             ch = get_next_token();
//             if (ch == ']')
//                 return data;
//
//             while (1) {
//                 i--;
//                 data.push_back(parse_json(depth + 1));
//                 if (failed)
//                     return Schema();
//
//                 ch = get_next_token();
//                 if (ch == ']')
//                     break;
//                 if (ch != ',')
//                     return fail("expected ',' in list, got " + esc(ch));
//
//                 ch = get_next_token();
//                 (void)ch;
//             }
//             return data;
//         }
//
//         return fail("expected value, got " + esc(ch));
//     }
// };
//
// Schema Schema::parse(const string &in, string &err, SchemaParse strategy) {
//     SchemaParser parser { in, 0, err, false, strategy };
//     Schema result = parser.parse_json(0);
//
//     // Check for any trailing garbage
//     parser.consume_garbage();
//     if (parser.i != in.size())
//         return parser.fail("unexpected trailing " + esc(in[parser.i]));
//
//     return result;
// }
//
// // Documented in schema11.hpp
// vector<Schema> Schema::parse_multi(const string &in,
//                                string &err,
//                                SchemaParse strategy) {
//     SchemaParser parser { in, 0, err, false, strategy };
//
//     vector<Schema> json_vec;
//     while (parser.i != in.size() && !parser.failed) {
//         json_vec.push_back(parser.parse_json(0));
//         // Check for another object
//         parser.consume_garbage();
//     }
//     return json_vec;
// }
//
// /* * * * * * * * * * * * * * * * * * * *
//  * Shape-checking
//  */
//
// bool Schema::has_shape(const shape & types, string & err) const {
//     if (!is_object()) {
//         err = "expected JSON object, got " + dump();
//         return false;
//     }
//
//     for (auto & item : types) {
//         if ((*this)[item.first].type() != item.second) {
//             err = "bad type for " + item.first + " in " + dump();
//             return false;
//         }
//     }
//
//     return true;
// }

template <typename T>
ValueConverter PrimitiveConverter(T & value, std::function<T(const json11::Json &)> fromJson)
{
	return ValueConverter {
		.FromJson = [&value, fromJson](const Json & json) {
			value = fromJson(json);
		},
		.ToJson = [&value](Json &json) {
			json = Json(value);
		}
	};
}
ValueConverter PrimitiveConverter(int & value)
{
	return PrimitiveConverter<int>(value, &json11::Json::int_value);
}
ValueConverter PrimitiveConverter(bool & value)
{
	return PrimitiveConverter<bool>(value, &json11::Json::bool_value);
}
ValueConverter PrimitiveConverter(float & value)
{
	return PrimitiveConverter<float>(value, &json11::Json::number_value);
}
ValueConverter PrimitiveConverter(double & value)
{
	return PrimitiveConverter<double>(value, &json11::Json::number_value);
}
ValueConverter PrimitiveConverter(string & value)
{
	return PrimitiveConverter<std::string>(value, &json11::Json::string_value);
}

} // namespace schema11
