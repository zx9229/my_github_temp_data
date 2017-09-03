// 本文件应以 UTF-8 无 BOM 格式编码.
#ifndef ALL_CLASS_DEFINITION_H
#define ALL_CLASS_DEFINITION_H
//////////////////////////////////////////////////////////////////////////
#include <string>
#include <map>
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
    short m_short;
    unsigned short m_ushort;
    int m_int;
    unsigned int m_uint;
    //long m_long;
    //unsigned long m_ulong;
    long long m_ll;
    unsigned long long m_ull;
    float m_f;
    double m_d;
};
//////////////////////////////////////////////////////////////////////////
#if 1
namespace RJC
{
    void toData(rapidjson::Value& jValue, LoginData& data, int size)
    {
        data = {};
        rapidjson::Value::MemberIterator it;
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_username")))
            toData(it->value, data.m_username, sizeof(data.m_username));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_password")))
            toData(it->value, data.m_password, sizeof(data.m_password));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_reservedData")))
            toData(it->value, data.m_reservedData, sizeof(data.m_reservedData));
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
    };
    void toData(rapidjson::Value& jValue, PushMessage& data, int size)
    {
        data = {};
        rapidjson::Value::MemberIterator it;
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_username")))
            toData(it->value, data.m_username, sizeof(data.m_username));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_message")))
            toData(it->value, data.m_message, sizeof(data.m_message));
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
    void toData(rapidjson::Value& jValue, TestCls& data, int size)
    {
        data = {};
        rapidjson::Value::MemberIterator it;
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_str")))
            toData(it->value, data.m_str, sizeof(data.m_str));
    };
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, const TestCls& data)
    {
        jvOut.SetObject();
        jvOut.RemoveAllMembers();
        rapidjson::Value jValue;
        fromData(jValue, allocator, data.m_str);
        jvOut.AddMember("m_str", jValue, allocator);
    };
    void toData(rapidjson::Value& jValue, TestStruct& data, int size)
    {
        data = {};
        rapidjson::Value::MemberIterator it;
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_c")))
            toData(it->value, data.m_c, sizeof(data.m_c));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_b")))
            toData(it->value, data.m_b, sizeof(data.m_b));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_short")))
            toData(it->value, data.m_short, sizeof(data.m_short));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_ushort")))
            toData(it->value, data.m_ushort, sizeof(data.m_ushort));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_int")))
            toData(it->value, data.m_int, sizeof(data.m_int));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_uint")))
            toData(it->value, data.m_uint, sizeof(data.m_uint));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_ll")))
            toData(it->value, data.m_ll, sizeof(data.m_ll));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_ull")))
            toData(it->value, data.m_ull, sizeof(data.m_ull));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_f")))
            toData(it->value, data.m_f, sizeof(data.m_f));
        if (jValue.MemberEnd() != (it = jValue.FindMember("m_d")))
            toData(it->value, data.m_d, sizeof(data.m_d));
    };
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, const TestStruct& data)
    {
        jvOut.SetObject();
        jvOut.RemoveAllMembers();
        rapidjson::Value jValue;
        fromData(jValue, allocator, data.m_c);
        jvOut.AddMember("m_c", jValue, allocator);
        fromData(jValue, allocator, data.m_b);
        jvOut.AddMember("m_b", jValue, allocator);
        fromData(jValue, allocator, data.m_short);
        jvOut.AddMember("m_short", jValue, allocator);
        fromData(jValue, allocator, data.m_ushort);
        jvOut.AddMember("m_ushort", jValue, allocator);
        fromData(jValue, allocator, data.m_int);
        jvOut.AddMember("m_int", jValue, allocator);
        fromData(jValue, allocator, data.m_uint);
        jvOut.AddMember("m_uint", jValue, allocator);
        fromData(jValue, allocator, data.m_ll);
        jvOut.AddMember("m_ll", jValue, allocator);
        fromData(jValue, allocator, data.m_ull);
        jvOut.AddMember("m_ull", jValue, allocator);
        fromData(jValue, allocator, data.m_f);
        jvOut.AddMember("m_f", jValue, allocator);
        fromData(jValue, allocator, data.m_d);
        jvOut.AddMember("m_d", jValue, allocator);
    };
};
#endif
//////////////////////////////////////////////////////////////////////////
#endif//ALL_CLASS_DEFINITION_H
