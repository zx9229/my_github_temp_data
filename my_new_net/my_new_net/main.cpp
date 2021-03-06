#include "xx.hpp"
#include <iostream>
#include <boost/thread.hpp>
void test1();
void test2();
int main()
{
    test2();
    return 0;
}

void test1()
{
    boost::asio::io_service io;
    boost::asio::io_service::work wk(io);
    boost::thread_group thgp;
    for (int i = 0; i < 4; ++i)
    {
        thgp.create_thread(boost::bind(&boost::asio::io_service::run, boost::ref(io)));
    }
    TcpSocket tcpSock(io);
    for (std::string cmd; (std::cout << "press:" << std::endl) && getline(std::cin, cmd); )
    {
        if (cmd == "connect")
        {
            std::cout << "please input remoteIp remotePort,(e.g. 127.0.0.1 22):" << std::endl;
            std::uint16_t remotePort;
            std::string remoteIp;
            std::cin >> remoteIp;
            std::cin >> remotePort;
            tcpSock.connect(remoteIp.c_str(), remotePort);
        }
        else if (cmd == "close")
        {
            tcpSock.close();
        }
        else
        {
            tcpSock.send(cmd.c_str(), cmd.size());
        }
    }
    return;
}
void test2()
{
    TcpClient client(4);
    for (std::string cmd; (std::cout << "press:" << std::endl) && getline(std::cin, cmd); )
    {
        if (cmd == "connect")
        {
            std::cout << "please input remoteIp remotePort,(e.g. 127.0.0.1 22):" << std::endl;
            std::uint16_t remotePort;
            std::string remoteIp;
            std::cin >> remoteIp;
            std::cin >> remotePort;
            client.connect(remoteIp.c_str(), remotePort);
        }
        else if (cmd == "close")
        {
            client.close();
        }
        else
        {
            client.send(cmd.c_str(), cmd.size());
        }
    }
}