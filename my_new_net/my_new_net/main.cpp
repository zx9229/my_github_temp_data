#include "xx.hpp"
#include <iostream>

int main()
{
    SendBufferEx sbe;
    if (auto d = sbe.lock())
    {
        std::cout << d.m_posWork << std::endl;
    }
    if (auto d = sbe.lock())
    {
        std::cout << d.m_bufWait << std::endl;
    }
    return 0;
}