// 本文件应以 UTF-8 无 BOM 格式编码.
#ifndef RAPID_JSON_CONVERT_H
#define RAPID_JSON_CONVERT_H
//typedef <元素类型关键字><数组类型名>[<常量表达式>];
//typedef char CHAR_4[4];
#include <string>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
//////////////////////////////////////////////////////////////////////////
//RapidJSON 文档
//http://rapidjson.org/zh-cn/
//////////////////////////////////////////////////////////////////////////
#include "rapidjson/document.h"
//////////////////////////////////////////////////////////////////////////
/*
注意，RapidJSON 并不自动转换各种 JSON 类型。例如，对一个 String 的 Value 调用 GetInt() 是非法的。在调试模式下，它会被断言失败。在发布模式下，其行为是未定义的。
*/
namespace RJC  // RapidJsonConvert
{
    void toData(rapidjson::Value& jValue, bool& data, int size)
    {
        if (jValue.IsBool())
            data = jValue.GetBool();
        else
            data = {};
    }
    void toData(rapidjson::Value& jValue, char& data, int size)
    {
        if (jValue.IsString())
            data = jValue.GetString()[0];
        else
            data = {};
    }
    void toData(rapidjson::Value& jValue, char* data, int size)
    {
        if (jValue.IsString())
            std::memcpy(data, jValue.GetString(), std::min((int)jValue.GetStringLength(), size - 1));//为了char buff[64];,为所有函数都添加了size字段.
        else
            memset(data, 0, size);
    }
    void toData(rapidjson::Value& jValue, std::string& data, int size)
    {
        if (jValue.IsString())
            data = jValue.GetString();
        else
            data = {};
    }
    void toData(rapidjson::Value& jValue, std::int16_t& data, int size)
    {
        if (jValue.IsInt())
            data = jValue.GetInt();
        else
            data = {};
    }
    void toData(rapidjson::Value& jValue, std::uint16_t& data, int size)
    {
        if (jValue.IsInt())
            data = jValue.GetUint();
        else
            data = {};
    }
    void toData(rapidjson::Value& jValue, std::int32_t& data, int size)
    {
        if (jValue.IsInt())
            data = jValue.GetInt();
        else
            data = {};
    }
    void toData(rapidjson::Value& jValue, std::uint32_t& data, int size)
    {
        if (jValue.IsUint())
            data = jValue.GetUint();
        else
            data = {};
    }
    void toData(rapidjson::Value& jValue, std::int64_t& data, int size)
    {
        if (jValue.IsInt64())
            data = jValue.GetInt64();
        else
            data = {};
    }
    void toData(rapidjson::Value& jValue, std::uint64_t& data, int size)
    {
        if (jValue.IsUint64())
            data = jValue.GetUint64();
        else
            data = {};
    }
    void toData(rapidjson::Value& jValue, float& data, int size)
    {
        if (jValue.IsFloat())
            data = jValue.GetFloat();
        else
            data = {};
    }
    void toData(rapidjson::Value& jValue, double& data, int size)
    {
        if (jValue.IsDouble())
            data = jValue.GetDouble();
        else
            data = {};
    }
    template<typename Type>
    void toData(rapidjson::Value& jValue, std::vector<Type>& data, int size)
    {
        data.clear();
        Type tempNode = {};
        if (jValue.IsArray())
        {
            for (rapidjson::Value& item : jValue.GetArray())
            {
                toData(item, tempNode, sizeof(tempNode));
                data.push_back(tempNode);
            }
        }
    }
    template<typename Type>
    void toData(rapidjson::Value& jValue, std::list<Type>& data, int size)
    {
        data.clear();
        Type tempNode = {};
        if (jValue.IsArray())
        {
            for (rapidjson::Value& item : jValue.GetArray())
            {
                toData(item, tempNode, sizeof(tempNode));
                data.push_back(tempNode);
            }
        }
    }
    template<typename Kty, typename Vty>
    void toData(rapidjson::Value& jValue, std::map<Kty, Vty>& data, int size)
    {
        data.clear();
        Kty tempK = {};
        Vty tempV = {};
        if (jValue.IsObject())
        {
            for (auto& pair : jValue.GetObject())
            {
                toData(pair.name, tempK, sizeof(tempK));
                toData(pair.value, tempV, sizeof(tempV));
                data.insert(std::make_pair(tempK, tempV));
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, bool data)
    {
        jvOut.SetBool(data);
    }
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, char data)
    {
        jvOut.SetString(&data, 1, allocator);
    }
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, std::int32_t data)
    {
        jvOut.SetInt(data);
    }
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, std::uint32_t data)
    {
        jvOut.SetUint(data);
    }
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, std::int64_t data)
    {
        jvOut.SetInt64(data);
    }
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, std::uint64_t data)
    {
        jvOut.SetUint64(data);
    }
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, float data)
    {
        jvOut.SetFloat(data);
    }
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, double data)
    {
        jvOut.SetDouble(data);
    }
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, const char* data)
    {
        jvOut.SetString(data, allocator);//jvOut.SetString(data, strlen(data));
    }
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, const std::string& data)
    {
        jvOut.SetString(data.c_str(), data.size(), allocator);//jvOut.SetString(data.c_str(), data.size());
    }
    template<typename Type>
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, const std::vector<Type>& data)
    {
        jvOut.SetArray();
        jvOut.Clear();
        rapidjson::Value jValue;
        for (auto& item : data)
        {
            fromData(jValue, allocator, item);
            jvOut.PushBack(jValue, allocator);
        }
    }
    template<typename Type>
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, const std::list<Type>& data)
    {
        jvOut.SetArray();
        jvOut.Clear();
        rapidjson::Value jValue;
        for (auto& item : data)
        {
            fromData(jValue, allocator, item);
            jvOut.PushBack(jValue, allocator);
        }
    }
    template<typename VType>
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, const std::map<std::string, VType>& data)
    {
        jvOut.SetObject();
        jvOut.RemoveAllMembers();
        rapidjson::Value kjV;
        rapidjson::Value vjV;
        for (auto& pair : data)
        {
            fromData(kjV, allocator, pair.first);
            fromData(vjV, allocator, pair.second);
            jvOut.AddMember(kjV, vjV, allocator);
        }
    }
    //////////////////////////////////////////////////////////////////////////
    //rapidjson::CrtAllocator m_allocator;
};
#endif//RAPID_JSON_CONVERT_H
