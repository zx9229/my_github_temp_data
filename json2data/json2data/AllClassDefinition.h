// 本文件应以 UTF-8 无 BOM 格式编码.
#ifndef ALL_CLASS_DEFINITION_H
#define ALL_CLASS_DEFINITION_H
//////////////////////////////////////////////////////////////////////////
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <list>
//////////////////////////////////////////////////////////////////////////
#include "RJC.h"
//////////////////////////////////////////////////////////////////////////
class LoginData
{
public:
    std::string m_username;
    std::string m_password;
    std::string m_reservedData;//预留的数据(类似银行APP里面的预留信息)(响应数据).
    std::list<std::string> m_groups;//从属于哪些组(响应数据).
};

class PushMessage
{
public:
    std::string m_username;
    std::string m_message;
};

class TestCls
{
public:
    std::string m_str;
    std::vector<std::string> m_vstr;
    std::list<int> m_lint;
    std::map<std::string, long> m_msl;
    std::map<std::string, std::vector<int>> m_mv;
};

struct TestStruct
{
    char m_c;
    char m_buf[64];
    bool m_b;
    short m_s;
    unsigned short m_us;
    int m_i;
    unsigned int m_ui;
    long m_l;//在windows下,long和int都是std::int32_t,在Linux下long是std::int64_t,
    unsigned long m_ul;
    long long m_ll;
    unsigned long long m_ull;
    float m_f;
    double m_d;
};
//////////////////////////////////////////////////////////////////////////
struct one_t
{
    int id;
    bool operator<(const one_t& _r)const { return (this->id < _r.id); }
    bool operator==(const one_t& _r)const { return (this->id == _r.id); }
};
struct two
{
    std::string name;
    one_t one;
    int age;
};
struct composit_t
{
    int a;
    std::vector<std::string> b;
    int c;
    std::map<int, int> d;
    std::unordered_map<int, int> e;
    double f;
    std::list<one_t> g;
};
//////////////////////////////////////////////////////////////////////////
#if 1
namespace RJC
{
    void toData(rapidjson::Value& jValue, LoginData& dataOut, int size)
    {
        dataOut = {};
        rapidjson::Value::MemberIterator it;
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_username")))
            toData(it->value, dataOut.m_username, sizeof(dataOut.m_username));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_password")))
            toData(it->value, dataOut.m_password, sizeof(dataOut.m_password));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_reservedData")))
            toData(it->value, dataOut.m_reservedData, sizeof(dataOut.m_reservedData));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_groups")))
            toData(it->value, dataOut.m_groups, sizeof(dataOut.m_groups));
    };
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, const LoginData& data)
    {
        jvOut.SetObject();
        jvOut.RemoveAllMembers();
        rapidjson::Value jValue;
        fromData(jValue, allocator, data.m_username);
        jvOut.AddMember("m_username", jValue, allocator);
        fromData(jValue, allocator, data.m_password);
        jvOut.AddMember("m_password", jValue, allocator);
        fromData(jValue, allocator, data.m_reservedData);
        jvOut.AddMember("m_reservedData", jValue, allocator);
        fromData(jValue, allocator, data.m_groups);
        jvOut.AddMember("m_groups", jValue, allocator);
    };
    void toData(rapidjson::Value& jValue, PushMessage& dataOut, int size)
    {
        dataOut = {};
        rapidjson::Value::MemberIterator it;
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_username")))
            toData(it->value, dataOut.m_username, sizeof(dataOut.m_username));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_message")))
            toData(it->value, dataOut.m_message, sizeof(dataOut.m_message));
    };
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, const PushMessage& data)
    {
        jvOut.SetObject();
        jvOut.RemoveAllMembers();
        rapidjson::Value jValue;
        fromData(jValue, allocator, data.m_username);
        jvOut.AddMember("m_username", jValue, allocator);
        fromData(jValue, allocator, data.m_message);
        jvOut.AddMember("m_message", jValue, allocator);
    };
    void toData(rapidjson::Value& jValue, TestCls& dataOut, int size)
    {
        dataOut = {};
        rapidjson::Value::MemberIterator it;
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_str")))
            toData(it->value, dataOut.m_str, sizeof(dataOut.m_str));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_vstr")))
            toData(it->value, dataOut.m_vstr, sizeof(dataOut.m_vstr));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_lint")))
            toData(it->value, dataOut.m_lint, sizeof(dataOut.m_lint));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_msl")))
            toData(it->value, dataOut.m_msl, sizeof(dataOut.m_msl));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_mv")))
            toData(it->value, dataOut.m_mv, sizeof(dataOut.m_mv));
    };
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, const TestCls& data)
    {
        jvOut.SetObject();
        jvOut.RemoveAllMembers();
        rapidjson::Value jValue;
        fromData(jValue, allocator, data.m_str);
        jvOut.AddMember("m_str", jValue, allocator);
        fromData(jValue, allocator, data.m_vstr);
        jvOut.AddMember("m_vstr", jValue, allocator);
        fromData(jValue, allocator, data.m_lint);
        jvOut.AddMember("m_lint", jValue, allocator);
        fromData(jValue, allocator, data.m_msl);
        jvOut.AddMember("m_msl", jValue, allocator);
        fromData(jValue, allocator, data.m_mv);
        jvOut.AddMember("m_mv", jValue, allocator);
    };
    void toData(rapidjson::Value& jValue, TestStruct& dataOut, int size)
    {
        dataOut = {};
        rapidjson::Value::MemberIterator it;
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_c")))
            toData(it->value, dataOut.m_c, sizeof(dataOut.m_c));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_buf")))
            toData(it->value, dataOut.m_buf, sizeof(dataOut.m_buf));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_b")))
            toData(it->value, dataOut.m_b, sizeof(dataOut.m_b));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_s")))
            toData(it->value, dataOut.m_s, sizeof(dataOut.m_s));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_us")))
            toData(it->value, dataOut.m_us, sizeof(dataOut.m_us));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_i")))
            toData(it->value, dataOut.m_i, sizeof(dataOut.m_i));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_ui")))
            toData(it->value, dataOut.m_ui, sizeof(dataOut.m_ui));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_l")))
            toData(it->value, dataOut.m_l, sizeof(dataOut.m_l));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_ul")))
            toData(it->value, dataOut.m_ul, sizeof(dataOut.m_ul));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_ll")))
            toData(it->value, dataOut.m_ll, sizeof(dataOut.m_ll));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_ull")))
            toData(it->value, dataOut.m_ull, sizeof(dataOut.m_ull));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_f")))
            toData(it->value, dataOut.m_f, sizeof(dataOut.m_f));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_d")))
            toData(it->value, dataOut.m_d, sizeof(dataOut.m_d));
    };
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, const TestStruct& data)
    {
        jvOut.SetObject();
        jvOut.RemoveAllMembers();
        rapidjson::Value jValue;
        fromData(jValue, allocator, data.m_c);
        jvOut.AddMember("m_c", jValue, allocator);
        fromData(jValue, allocator, data.m_buf);
        jvOut.AddMember("m_buf", jValue, allocator);
        fromData(jValue, allocator, data.m_b);
        jvOut.AddMember("m_b", jValue, allocator);
        fromData(jValue, allocator, data.m_s);
        jvOut.AddMember("m_s", jValue, allocator);
        fromData(jValue, allocator, data.m_us);
        jvOut.AddMember("m_us", jValue, allocator);
        fromData(jValue, allocator, data.m_i);
        jvOut.AddMember("m_i", jValue, allocator);
        fromData(jValue, allocator, data.m_ui);
        jvOut.AddMember("m_ui", jValue, allocator);
        fromData(jValue, allocator, data.m_l);
        jvOut.AddMember("m_l", jValue, allocator);
        fromData(jValue, allocator, data.m_ul);
        jvOut.AddMember("m_ul", jValue, allocator);
        fromData(jValue, allocator, data.m_ll);
        jvOut.AddMember("m_ll", jValue, allocator);
        fromData(jValue, allocator, data.m_ull);
        jvOut.AddMember("m_ull", jValue, allocator);
        fromData(jValue, allocator, data.m_f);
        jvOut.AddMember("m_f", jValue, allocator);
        fromData(jValue, allocator, data.m_d);
        jvOut.AddMember("m_d", jValue, allocator);
    };
    void toData(rapidjson::Value& jValue, one_t& dataOut, int size)
    {
        dataOut = {};
        rapidjson::Value::MemberIterator it;
        if (jValue.MemberEnd() != (it = jValue.FindMember("id")))
            toData(it->value, dataOut.id, sizeof(dataOut.id));
    };
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, const one_t& data)
    {
        jvOut.SetObject();
        jvOut.RemoveAllMembers();
        rapidjson::Value jValue;
        fromData(jValue, allocator, data.id);
        jvOut.AddMember("id", jValue, allocator);
    };
    void toData(rapidjson::Value& jValue, two& dataOut, int size)
    {
        dataOut = {};
        rapidjson::Value::MemberIterator it;
        if (jValue.MemberEnd() != (it = jValue.FindMember("name")))
            toData(it->value, dataOut.name, sizeof(dataOut.name));
        if (jValue.MemberEnd() != (it = jValue.FindMember("one")))
            toData(it->value, dataOut.one, sizeof(dataOut.one));
        if (jValue.MemberEnd() != (it = jValue.FindMember("age")))
            toData(it->value, dataOut.age, sizeof(dataOut.age));
    };
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, const two& data)
    {
        jvOut.SetObject();
        jvOut.RemoveAllMembers();
        rapidjson::Value jValue;
        fromData(jValue, allocator, data.name);
        jvOut.AddMember("name", jValue, allocator);
        fromData(jValue, allocator, data.one);
        jvOut.AddMember("one", jValue, allocator);
        fromData(jValue, allocator, data.age);
        jvOut.AddMember("age", jValue, allocator);
    };
    void toData(rapidjson::Value& jValue, composit_t& dataOut, int size)
    {
        dataOut = {};
        rapidjson::Value::MemberIterator it;
        if (jValue.MemberEnd() != (it = jValue.FindMember("a")))
            toData(it->value, dataOut.a, sizeof(dataOut.a));
        if (jValue.MemberEnd() != (it = jValue.FindMember("b")))
            toData(it->value, dataOut.b, sizeof(dataOut.b));
        if (jValue.MemberEnd() != (it = jValue.FindMember("c")))
            toData(it->value, dataOut.c, sizeof(dataOut.c));
        if (jValue.MemberEnd() != (it = jValue.FindMember("d")))
            toData(it->value, dataOut.d, sizeof(dataOut.d));
        if (jValue.MemberEnd() != (it = jValue.FindMember("e")))
            toData(it->value, dataOut.e, sizeof(dataOut.e));
        if (jValue.MemberEnd() != (it = jValue.FindMember("f")))
            toData(it->value, dataOut.f, sizeof(dataOut.f));
        if (jValue.MemberEnd() != (it = jValue.FindMember("g")))
            toData(it->value, dataOut.g, sizeof(dataOut.g));
    };
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, const composit_t& data)
    {
        jvOut.SetObject();
        jvOut.RemoveAllMembers();
        rapidjson::Value jValue;
        fromData(jValue, allocator, data.a);
        jvOut.AddMember("a", jValue, allocator);
        fromData(jValue, allocator, data.b);
        jvOut.AddMember("b", jValue, allocator);
        fromData(jValue, allocator, data.c);
        jvOut.AddMember("c", jValue, allocator);
        fromData(jValue, allocator, data.d);
        jvOut.AddMember("d", jValue, allocator);
        fromData(jValue, allocator, data.e);
        jvOut.AddMember("e", jValue, allocator);
        fromData(jValue, allocator, data.f);
        jvOut.AddMember("f", jValue, allocator);
        fromData(jValue, allocator, data.g);
        jvOut.AddMember("g", jValue, allocator);
    };
};
#endif
//////////////////////////////////////////////////////////////////////////
#endif//ALL_CLASS_DEFINITION_H
