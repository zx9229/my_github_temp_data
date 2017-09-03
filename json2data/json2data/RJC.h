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
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
//////////////////////////////////////////////////////////////////////////
/*
注意，RapidJSON 并不自动转换各种 JSON 类型。例如，对一个 String 的 Value 调用 GetInt() 是非法的。在调试模式下，它会被断言失败。在发布模式下，其行为是未定义的。
*/
namespace RJC  // RapidJsonConvert
{
    template<typename DataType>
    void data2json(const DataType& data, std::string& jsonOut)
    {
        rapidjson::Document d;
        fromData(d, d.GetAllocator(), data);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        d.Accept(writer);

        jsonOut = buffer.GetString();
    }
    template<typename DataType>
    void json2data(const std::string& json, DataType& dataOut)
    {
        dataOut = {};
        rapidjson::Document d;
        d.Parse(json.c_str());
        if (!d.HasParseError())
        {
            toData(d, dataOut, sizeof(dataOut));
        }
    }
    //////////////////////////////////////////////////////////////////////////
    void toData(rapidjson::Value& jValue, char& dataOut, int size)
    {
        if (jValue.IsString())
            dataOut = jValue.GetString()[0];
        else
            dataOut = {};
    }
    void toData(rapidjson::Value& jValue, char* dataOut, int size)
    {
        if (jValue.IsString())
            std::memcpy(dataOut, jValue.GetString(), std::min((int)jValue.GetStringLength(), size - 1));//为了char buff[64];,为所有函数都添加了size字段.
        else
            memset(dataOut, 0, size);
    }
    void toData(rapidjson::Value& jValue, std::string& dataOut, int size)
    {
        if (jValue.IsString())
            dataOut = jValue.GetString();
        else
            dataOut = {};
    }
    void toData(rapidjson::Value& jValue, bool& dataOut, int size)
    {
        if (jValue.IsBool())
            dataOut = jValue.GetBool();
        else
            dataOut = {};
    }
    void toData(rapidjson::Value& jValue, std::int16_t& dataOut, int size)
    {
        if (jValue.IsInt())
            dataOut = jValue.GetInt();
        else
            dataOut = {};
    }
    void toData(rapidjson::Value& jValue, std::uint16_t& dataOut, int size)
    {
        if (jValue.IsInt())
            dataOut = jValue.GetUint();
        else
            dataOut = {};
    }
    void toData(rapidjson::Value& jValue, std::int32_t& dataOut, int size)
    {
        if (jValue.IsInt())
            dataOut = jValue.GetInt();
        else
            dataOut = {};
    }
    void toData(rapidjson::Value& jValue, std::uint32_t& dataOut, int size)
    {
        if (jValue.IsUint())
            dataOut = jValue.GetUint();
        else
            dataOut = {};
    }
    void toData(rapidjson::Value& jValue, std::int64_t& dataOut, int size)
    {
        if (jValue.IsInt64())
            dataOut = jValue.GetInt64();
        else
            dataOut = {};
    }
    void toData(rapidjson::Value& jValue, std::uint64_t& dataOut, int size)
    {
        if (jValue.IsUint64())
            dataOut = jValue.GetUint64();
        else
            dataOut = {};
    }
    void toData(rapidjson::Value& jValue, long& dataOut, int size)
    {
        if (sizeof(long) == sizeof(std::int32_t))
        {
            if (jValue.IsInt())
                dataOut = (long)jValue.GetInt();
            else
                dataOut = {};
        }
        else if (sizeof(long) == sizeof(std::int64_t))
        {
            if (jValue.IsInt64())
                dataOut = (long)jValue.GetInt64();
            else
                dataOut = {};
        }
        else
        {
            throw std::logic_error("value of sizeof(long) is abnormal.");
        }
    }
    void toData(rapidjson::Value& jValue, unsigned long& dataOut, int size)
    {
        if (sizeof(unsigned long) == sizeof(std::uint32_t))
        {
            if (jValue.IsUint())
                dataOut = (unsigned long)jValue.GetUint();
            else
                dataOut = {};
        }
        else if (sizeof(unsigned long) == sizeof(std::uint64_t))
        {
            if (jValue.IsUint64())
                dataOut = (unsigned long)jValue.GetUint64();
            else
                dataOut = {};
        }
        else
        {
            throw std::logic_error("value of sizeof(unsigned long) is abnormal.");
        }
    }
    void toData(rapidjson::Value& jValue, float& dataOut, int size)
    {
        if (jValue.IsFloat())
            dataOut = jValue.GetFloat();
        else
            dataOut = {};
    }
    void toData(rapidjson::Value& jValue, double& dataOut, int size)
    {
        if (jValue.IsDouble())
            dataOut = jValue.GetDouble();
        else
            dataOut = {};
    }
    template<typename Type>
    void toData(rapidjson::Value& jValue, std::vector<Type>& dataOut, int size)
    {
        dataOut.clear();
        Type tempNode = {};
        if (jValue.IsArray())
        {
            for (rapidjson::Value& item : jValue.GetArray())
            {
                toData(item, tempNode, sizeof(tempNode));
                dataOut.push_back(tempNode);
            }
        }
    }
    template<typename Type>
    void toData(rapidjson::Value& jValue, std::list<Type>& dataOut, int size)
    {
        dataOut.clear();
        Type tempNode = {};
        if (jValue.IsArray())
        {
            for (rapidjson::Value& item : jValue.GetArray())
            {
                toData(item, tempNode, sizeof(tempNode));
                dataOut.push_back(tempNode);
            }
        }
    }
    template<typename Kty, typename Vty>
    void toData(rapidjson::Value& jValue, std::map<Kty, Vty>& dataOut, int size)
    {
        dataOut.clear();
        Kty tempK = {};
        Vty tempV = {};
        if (jValue.IsObject())
        {
            for (auto& pair : jValue.GetObject())
            {
                toData(pair.name, tempK, sizeof(tempK));
                toData(pair.value, tempV, sizeof(tempV));
                dataOut.insert(std::make_pair(tempK, tempV));
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, char data)
    {
        jvOut.SetString(&data, 1, allocator);
    }
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, const char* data)
    {
        jvOut.SetString(data, allocator);//jvOut.SetString(data, strlen(data));
    }
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, const std::string& data)
    {
        jvOut.SetString(data.c_str(), data.size(), allocator);//jvOut.SetString(data.c_str(), data.size());
    }
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, bool data)
    {
        jvOut.SetBool(data);
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
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, long data)
    {
        if (sizeof(long) == sizeof(std::int32_t))
            jvOut.SetInt(data);
        else if (sizeof(long) == sizeof(std::int64_t))
            jvOut.SetInt64(data);
        else
            throw std::logic_error("value of sizeof(long) is abnormal.");
    }
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, unsigned long data)
    {
        if (sizeof(unsigned long) == sizeof(std::uint32_t))
            jvOut.SetUint(data);
        else if (sizeof(unsigned long) == sizeof(std::uint64_t))
            jvOut.SetUint64(data);
        else
            throw std::logic_error("value of sizeof(unsigned long) is abnormal.");
    }
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, float data)
    {
        jvOut.SetFloat(data);
    }
    void fromData(rapidjson::Value& jvOut, rapidjson::Document::AllocatorType& allocator, double data)
    {
        jvOut.SetDouble(data);
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
