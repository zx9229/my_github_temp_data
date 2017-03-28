#include "xx.hpp"
#include <iostream>

int main()
{
    boost::asio::io_service io;
    TcpSocket sock(io);
    return 0;
}