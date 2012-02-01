
#ifndef JSON_H
#define JSON_H

#include <list>
#include <map>
#include <string>
#include <vector>

namespace ThreadServer {
namespace JSON {

enum Type {
    NullType = 0,
    StringType,
    IntType,
    DoubleType,
    BoolType,
    StructType,
    ArrayType
};

class Value_t {
protected:
    Value_t();

public:
    virtual Type getType() const = 0;

    virtual bool isNull() const;

    virtual operator std::string() const = 0;
};

class Null_t : public Value_t {
friend class Pool_t;
protected:
    Null_t();

public:
    virtual Type getType() const;

    virtual operator std::string() const;
};

class String_t : public Value_t {
friend class Pool_t;
protected:
    String_t(const std::string &data);

public:
    virtual Type getType() const;

    virtual operator std::string() const;

    static std::string escape(const std::string &s);

protected:
    const std::string data;
};

class Int_t : public Value_t {
friend class Pool_t;
protected:
    Int_t(const long long data);

public:
    virtual Type getType() const;

    virtual operator std::string() const;

protected:
    const long long data;
};

class Double_t : public Value_t {
friend class Pool_t;
protected:
    Double_t(const double data);

public:
    virtual Type getType() const;

    virtual operator std::string() const;

protected:
    const double data;
};

class Bool_t : public Value_t {
friend class Pool_t;
protected:
    Bool_t(const bool data);

public:
    virtual Type getType() const;

    virtual operator std::string() const;

protected:
    const bool data;
};

class Struct_t : public Value_t {
friend class Pool_t;
protected:
    Struct_t();

public:
    virtual Type getType() const;

    virtual operator std::string() const;

    Struct_t& append(const std::string &name, const Value_t &value);

protected:
    std::map<std::string, const Value_t*> data;
};

class Array_t : public Value_t {
friend class Pool_t;
protected:
    Array_t();

public:
    virtual Type getType() const;

    virtual operator std::string() const;

    Array_t& push_back(const Value_t &value);

protected:
    std::vector<const Value_t*> data;
};

class Pool_t {
public:
    Pool_t();

    ~Pool_t();

    Null_t& Null();

    String_t& String(const std::string &data);

    Int_t& Int(const long long data);

    Double_t& Double(const double data);

    Bool_t& Bool(const bool data);

    Struct_t& Struct();

    Array_t& Array();

protected:
    std::list<Value_t*> values;

    Null_t null;
};

} // namespace JSON
} // namespace ThreadServer

#endif // JSON_H

