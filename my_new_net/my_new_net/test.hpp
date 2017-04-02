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
