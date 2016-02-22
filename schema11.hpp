#pragma once

#include <functional>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <initializer_list>

namespace json11 {
	class Json;
}

namespace schema11 {
    
class SchemaValue;

struct ValueConverter
{
	std::function<void(const json11::Json &)> from_json = [](const json11::Json &){};
	std::function<void(json11::Json &)> to_json = [](json11::Json &){};
};

class Schema final {
public:
    // Types
    enum Type {
        NUL, NUMBER, BOOL, STRING, ARRAY, OBJECT
    };

    // Array and object typedefs
    typedef std::vector<Schema> array; 
    typedef std::map<std::string, Schema> object;

    // Constructors for the various types of JSON value.
    Schema() noexcept;                // NUL
    Schema(std::nullptr_t) noexcept;  // NUL
    Schema(Type type, ValueConverter valueConverter=ValueConverter());
    // Schema(const array &values);      // ARRAY
    // Schema(array &&values);           // ARRAY
    Schema(const object &values);     // OBJECT
    Schema(object &&values);          // OBJECT

    // Implicit constructor: anything with a to_json() function.
    template <class T, class = decltype(&T::to_json)>
    Schema(const T & t) : Schema(t.to_json()) {}

    // Implicit constructor: map-like objects (std::map, std::unordered_map, etc)
    template <class M, typename std::enable_if<
        std::is_constructible<std::string, typename M::key_type>::value
        && std::is_constructible<Schema, typename M::mapped_type>::value,
            int>::type = 0>
    Schema(const M & m) : Schema(object(m.begin(), m.end())) {}

    // Implicit constructor: vector-like objects (std::list, std::vector, std::set, etc)
    template <class V, typename std::enable_if<
        std::is_constructible<Schema, typename V::value_type>::value,
            int>::type = 0>
    Schema(const V & v) : Schema(array(v.begin(), v.end())) {}

    // This prevents Schema(some_pointer) from accidentally producing a bool. Use
    // Schema(bool(some_pointer)) if that behavior is desired.
    Schema(void *) = delete;

    // Accessors
    Type type() const;

    bool is_null()   const { return type() == NUL; }
    bool is_number() const { return type() == NUMBER; }
    bool is_bool()   const { return type() == BOOL; }
    bool is_string() const { return type() == STRING; }
    bool is_array()  const { return type() == ARRAY; }
    bool is_object() const { return type() == OBJECT; }

    // // Return the enclosed value if this is a number, 0 otherwise. Note that json11 does not
    // // distinguish between integer and non-integer numbers - number_value() and int_value()
    // // can both be applied to a NUMBER-typed object.
    // double number_value() const;
    // int int_value() const;
    //
    // // Return the enclosed value if this is a boolean, false otherwise.
    // bool bool_value() const;
    // // Return the enclosed string if this is a string, "" otherwise.
    // const std::string &string_value() const;
    // // Return the enclosed std::vector if this is an array, or an empty vector otherwise.
    // const array &array_items() const;
    // Return the enclosed std::map if this is an object, or an empty map otherwise.
    const object &object_items() const;

    // Return a reference to arr[i] if this is an array, Schema() otherwise.
    const Schema & operator[](size_t i) const;
    // Return a reference to obj[key] if this is an object, Schema() otherwise.
    const Schema & operator[](const std::string &key) const;
	
	void from_json(const json11::Json &json) const;
	void to_json(json11::Json &json) const;

    // Serialize.
    void dump(std::string &out) const;
    std::string dump() const {
        std::string out;
        dump(out);
        return out;
    }
    //
    // // Parse. If parse fails, return Schema() and assign an error message to err.
    // static Schema parse(const std::string & in,
    //                   std::string & err,
    //                   SchemaParse strategy = SchemaParse::STANDARD);
    // static Schema parse(const char * in,
    //                   std::string & err,
    //                   SchemaParse strategy = SchemaParse::STANDARD) {
    //     if (in) {
    //         return parse(std::string(in), err, strategy);
    //     } else {
    //         err = "null input";
    //         return nullptr;
    //     }
    // }
    // // Parse multiple objects, concatenated or separated by whitespace
    // static std::vector<Schema> parse_multi(
    //     const std::string & in,
    //     std::string & err,
    //     SchemaParse strategy = SchemaParse::STANDARD);
    //
    bool operator== (const Schema &rhs) const;
    bool operator<  (const Schema &rhs) const;
    bool operator!= (const Schema &rhs) const { return !(*this == rhs); }
    bool operator<= (const Schema &rhs) const { return !(rhs < *this); }
    bool operator>  (const Schema &rhs) const { return  (rhs < *this); }
    bool operator>= (const Schema &rhs) const { return !(*this < rhs); }
    //
    // /* has_shape(types, err)
    //  *
    //  * Return true if this is a JSON object and, for each item in types, has a field of
    //  * the given type. If not, return false and set err to a descriptive message.
    //  */
    // typedef std::initializer_list<std::pair<std::string, Type>> shape;
    // bool has_shape(const shape & types, std::string & err) const;

private:
    std::shared_ptr<SchemaValue> m_ptr;
};

// Internal class hierarchy - SchemaValue objects are not exposed to users of this API.
class SchemaValue {
protected:
    friend class Schema;
    // friend class SchemaInt;
    // friend class SchemaDouble;
    virtual Schema::Type type() const = 0;
    virtual bool equals(const SchemaValue * other) const = 0;
    virtual bool less(const SchemaValue * other) const = 0;
	virtual void from_json(const json11::Json &json) const = 0;
	virtual void to_json(json11::Json &json) const = 0;
    virtual void dump(std::string &out) const = 0;
    // virtual double number_value() const;
    // virtual int int_value() const;
    // virtual bool bool_value() const;
    // virtual const std::string &string_value() const;
    // virtual const Schema::array &array_items() const;
    virtual const Schema &operator[](size_t i) const;
    virtual const Schema::object &object_items() const;
    virtual const Schema &operator[](const std::string &key) const;
    virtual ~SchemaValue() {}
};

ValueConverter PrimitiveConverter(int & value);
ValueConverter PrimitiveConverter(bool & value);
ValueConverter PrimitiveConverter(float & value);
ValueConverter PrimitiveConverter(double & value);
ValueConverter PrimitiveConverter(std::string & value);

template <typename T>
ValueConverter ObjectConverter(T & value, std::function<Schema(T &)> schema)
{
	return ValueConverter {
		.from_json = [&value, schema](const json11::Json & json) {
			
		},
		.to_json = [&value, schema](json11::Json & json) {

		}
	};
}

}