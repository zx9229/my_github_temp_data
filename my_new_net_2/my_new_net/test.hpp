#ifndef TEST_HPP
#define TEST_HPP

/*
TcpStocket,发送可以同步和异步,
同步发送时,如果发现还有异步的包没有发送,直接返回错误消息.
接收到多少就回调多少(不涉及到解析消息的问题).
函数,bool isTcpClient,表示是不是TCPClient
函数,xxx getTcpClient,得到TCPClient.
*/
#include <string>
#include <boost/asio.hpp>

class TcpSocket
{
public:
    ///@brief  同"boost::asio::ip::tcp::socket::is_open()".
    bool socketIsOpen()const;
    ///@brief  tcp连接是连通着的.
    bool isConnected()const;
    ///@brief  这个TcpSocket还在工作中.
    ///        客户端的socket:连接正常,连接中&重连中.
    ///        服务端的socket:连接正常.
    bool isWorking()const;
    ///@brief  是服务器端产生出来的TcpSocket.
    bool isAcceptedFromServer()const;
    int connect(const std::string& ip, std::uint16_t port);
    ///@brief  (同步发送)同步发送时,会检查"异步发送缓冲区",如果缓冲区内有数据,就返回失败,而不进行发送.
    int sendSync(const char* data, unsigned size);
    ///@brief  (异步发送)
    int sendAsync(const char* data, unsigned size);
    
private:
    std::string m_sendBuf;
    boost::asio::ip::tcp::socket m_sock;
};

class TcpClient
{//断线重连
public:
    void isOpen();
    void close();
    void connect();
    void localPoint();
    void peerPoint();
    void registerParser();//注册二进制流的解析器.
    void sendSync();
    void sendAsync();
    //callback
    void onRtnStream();//从socket出来的字节流,直接返回
    void onRtnMessage();//从socket出来的字节流,通过注册进来的解析函数解析成消息,返回每个消息.
	void onError();
};

class TcpServer
{
public:
    void listen();
    //callback
    void onConnected(connection);
    void OnDisconnected(connection);
};

class MsgServer:public TcpServer
{
public:

};

#endif//TEST_HPP
