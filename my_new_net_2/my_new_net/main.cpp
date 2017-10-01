#include "xx.hpp"
#include <iostream>
#include <boost/thread.hpp>
void test1();
int main()
{
    MyNet::TcpClient client(1, 1);
    test1();
    return 0;
}

void test1()
{
    MyNet::IoWithMThPtr mThIo = MyNet::IoWithMThPtr(new MyNet::IoWithMTh);
    mThIo->initialize(4);
    
    MyNet::TcpSocketPtr tcpSock = MyNet::TcpSocketPtr(new MyNet::TcpSocket(mThIo->ioService()));

    for (std::string cmd; (std::cout << "press:" << std::endl) && getline(std::cin, cmd); )
    {
        if (cmd == "connect")
        {
            std::cout << "please input remoteIp remotePort,(e.g. 127.0.0.1 22):" << std::endl;
            std::uint16_t remotePort;
            std::string remoteIp;
            std::cin >> remoteIp;
            std::cin >> remotePort;
            int rv = tcpSock->connect(remoteIp.c_str(), remotePort);
            std::cout << "connect: rv=" << rv << std::endl;
        }
        else if (cmd == "close")
        {
            tcpSock->close();
        }
        else
        {
            int rv = tcpSock->sendAsync(cmd.c_str(), cmd.size());
            std::cout << "sendAsync: rv=" << rv << std::endl;
        }
    }
    return;
}
