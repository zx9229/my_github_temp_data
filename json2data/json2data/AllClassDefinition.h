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
    };
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, const TestCls& data)
    {
        jvOut.SetObject();
        jvOut.RemoveAllMembers();
        rapidjson::Value jValue;
        fromData(jValue, allocator, data.m_str);
        jvOut.AddMember("m_str", jValue, allocator);
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
};
#endif
//////////////////////////////////////////////////////////////////////////
#endif//ALL_CLASS_DEFINITION_H
