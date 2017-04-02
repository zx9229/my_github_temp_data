#ifndef TEST_HPP
#define TEST_HPP

class TcpSocket
{
public:
private:
    std::string m_sendBuf;
    std::string m_recvBuf;
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
