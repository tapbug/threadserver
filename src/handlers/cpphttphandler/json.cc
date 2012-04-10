
#include <boost/lexical_cast.hpp>

#include <threadserver/handlers/cpphttphandler/json.h>

namespace ThreadServer {
namespace JSON {

Value_t::Value_t()
{
}

bool Value_t::isNull() const
{
    return (getType() == JSON::NullType);
}

Null_t::Null_t()
{
}

Type Null_t::getType() const
{
    return JSON::NullType;
}

Null_t::operator std::string() const
{
    return "null";
}

String_t::String_t(const std::string &data)
  : data(data)
{
}

Type String_t::getType() const
{
    return JSON::StringType;
}

String_t::operator std::string() const
{
    return escape(data);
}

std::string String_t::escape(const std::string &s)
{
    std::string result("\"");
    for (size_t i(0) ; i < s.size() ; ++i) {
        if (s[i] == '\n') {
            result.append("\\n");
        } else if (s[i] == '\r') {
            result.append("\\r");
        } else if (s[i] == '\b') {
            result.append("\\b");
        } else if (s[i] == '\f') {
            result.append("\\f");
        } else if (s[i] == '\t') {
            result.append("\\t");
        } else {
            if (s[i] == '"' || s[i] == '\\' || s[i] == '/') {
                result.push_back('\\');
            }
            result.push_back(s[i]);
        }
    }
    result.push_back('"');
    return result;
}

Int_t::Int_t(const long long data)
  : data(data)
{
}

Type Int_t::getType() const
{
    return JSON::IntType;
}

Int_t::operator std::string() const
{
    return boost::lexical_cast<std::string>(data);
}

Double_t::Double_t(const double data)
  : data(data)
{
}

Type Double_t::getType() const
{
    return JSON::DoubleType;
}

Double_t::operator std::string() const
{
    return boost::lexical_cast<std::string>(data);
}

Bool_t::Bool_t(const bool data)
  : data(data)
{
}

Type Bool_t::getType() const
{
    return JSON::BoolType;
}

Bool_t::operator std::string() const
{
    return data ? "true" : "false";
}

Struct_t::Struct_t()
  : data()
{
}

Type Struct_t::getType() const
{
    return JSON::StructType;
}

Struct_t::operator std::string() const
{
    std::stringstream output;
    output << "{";
    std::string comma;
    for (std::map<std::string, const Value_t*>::const_iterator idata(data.begin()) ;
         idata != data.end() ;
         ++idata) {

        output << comma << String_t::escape(idata->first) << ":" << std::string(*idata->second);
        comma = ",";
    }
    output << "}";
    return output.str();
}

Struct_t& Struct_t::append(const std::string &name, const Value_t &value)
{
    data[name] = &value;
    return *this;
}

Array_t::Array_t()
  : data()
{
}

Type Array_t::getType() const
{
    return JSON::ArrayType;
}

Array_t::operator std::string() const
{
    std::stringstream output;
    output << "[";
    std::string comma;
    for (std::vector<const Value_t*>::const_iterator idata(data.begin()) ;
         idata != data.end() ;
         ++idata) {

        output << comma << std::string(**idata);
        comma = ",";
    }
    output << "]";
    return output.str();
}

Array_t& Array_t::push_back(const Value_t &value)
{
    data.push_back(&value);
    return *this;
}

Pool_t::Pool_t()
{
}

Pool_t::~Pool_t()
{
    for (std::list<Value_t*>::iterator ivalues(values.begin()) ;
         ivalues != values.end() ;
         ++ivalues) {

        delete *ivalues;
    }
}

Null_t& Pool_t::Null()
{
    return null;
}

String_t& Pool_t::String(const std::string &data)
{
    String_t *s(new String_t(data));
    values.push_back(s);
    return *s;
}

Int_t& Pool_t::Int(const long long data)
{
    Int_t *i(new Int_t(data));
    values.push_back(i);
    return *i;
}

Double_t& Pool_t::Double(const double data)
{
    Double_t *d(new Double_t(data));
    values.push_back(d);
    return *d;
}

Bool_t& Pool_t::Bool(const bool data)
{
    Bool_t *b(new Bool_t(data));
    values.push_back(b);
    return *b;
}

Struct_t& Pool_t::Struct()
{
    Struct_t *s(new Struct_t());
    values.push_back(s);
    return *s;
}

Array_t& Pool_t::Array()
{
    Array_t *a(new Array_t());
    values.push_back(a);
    return *a;
}

} // namespace JSON
} // namespace ThreadServer

