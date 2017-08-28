#ifndef TEST_HPP
#define TEST_HPP

/*
TcpStocket,���Ϳ���ͬ�����첽,
ͬ������ʱ,������ֻ����첽�İ�û�з���,ֱ�ӷ��ش�����Ϣ.
���յ����پͻص�����(���漰��������Ϣ������).
����,bool isTcpClient,��ʾ�ǲ���TCPClient
����,xxx getTcpClient,�õ�TCPClient.
*/
#include <string>
#include <boost/asio.hpp>

class TcpSocket
{
public:
    ///@brief  ͬ"boost::asio::ip::tcp::socket::is_open()".
    bool socketIsOpen()const;
    ///@brief  tcp��������ͨ�ŵ�.
    bool isConnected()const;
    ///@brief  ���TcpSocket���ڹ�����.
    ///        �ͻ��˵�socket:��������,������&������.
    ///        ����˵�socket:��������.
    bool isWorking()const;
    ///@brief  �Ƿ������˲���������TcpSocket.
    bool isAcceptedFromServer()const;
    int connect(const std::string& ip, std::uint16_t port);
    ///@brief  (ͬ������)ͬ������ʱ,����"�첽���ͻ�����",�����������������,�ͷ���ʧ��,�������з���.
    int sendSync(const char* data, unsigned size);
    ///@brief  (�첽����)
    int sendAsync(const char* data, unsigned size);
    
private:
    std::string m_sendBuf;
    boost::asio::ip::tcp::socket m_sock;
};

class TcpClient
{//��������
public:
    void isOpen();
    void close();
    void connect();
    void localPoint();
    void peerPoint();
    void registerParser();//ע����������Ľ�����.
    void sendSync();
    void sendAsync();
    //callback
    void onRtnStream();//��socket�������ֽ���,ֱ�ӷ���
    void onRtnMessage();//��socket�������ֽ���,ͨ��ע������Ľ���������������Ϣ,����ÿ����Ϣ.
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
