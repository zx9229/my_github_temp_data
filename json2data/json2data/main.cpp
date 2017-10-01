// 本文件应以 UTF-8 无 BOM 格式编码.
#include <iostream>
#include "RJC.h"
#include "AllClassDefinition.h"

TestStruct getOneTestStruct();
bool isEqual(const composit_t& _l, const composit_t&_r);


int main()
{
    one_t one = { 2 };
    composit_t composit = { 1,{ "tom", "jack\r\nCRLF" }, 3,{ { 2,3 } },{ { 5,6 } }, 5.3,{ one } };
    std::string jsonStr;
    RJC::data2json(composit, jsonStr);

    std::cout << jsonStr << std::endl;

    composit_t comp;
    RJC::json2data(jsonStr, comp);

    if (isEqual(composit, comp))
        printf("");
    else
        printf("");

    return 0;
}


int test_1()
{
    TestStruct ts = getOneTestStruct();
    std::string jsonData;
    RJC::data2json(ts, jsonData);
    TestStruct t2 = {};
    RJC::json2data(jsonData, t2);

    if (std::memcmp(&ts, &t2, sizeof(ts)) == 0)
        printf("");
    else
        printf("");

    return 0;
}

TestStruct getOneTestStruct()
{
    TestStruct ts = {};
    ts.m_b = true;
    std::strncpy(ts.m_buf, "xxxxxxxxxxxxxxxxxxxx", sizeof(ts.m_buf) - 1);
    ts.m_c = 0x01;
    ts.m_d = 3.14;
    ts.m_f = (float)6.28;
    ts.m_i = 333;
    ts.m_l = 666;
    ts.m_ll = 999;
    ts.m_s = 66/*65535*/;
    ts.m_ui = ts.m_i * 2;
    ts.m_ul = ts.m_l * 2;
    ts.m_ull = ts.m_ull * 2;
    ts.m_us = 10086;

    return ts;
}

bool isEqual(const composit_t& _l, const composit_t&_r)
{
    bool bIsEqual = false;
    do
    {
        if (_l.a != _r.a)
            break;
        if (_l.b != _r.b)
            break;
        if (_l.c != _r.c)
            break;
        if (_l.d != _r.d)
            break;
        if (_l.e != _r.e)
            break;
        if (_l.f != _r.f)
            break;
        if (_l.g != _r.g)
            break;
        //
        bIsEqual = true;
        //
    } while (false);
    //
    return bIsEqual;
}
