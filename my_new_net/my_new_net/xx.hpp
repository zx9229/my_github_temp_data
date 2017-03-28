#ifndef XX_HPP
#define XX_HPP
#include <iostream>
#include <cstdint>
#include <mutex>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/bind.hpp>
using StdStringPtr = std::shared_ptr<std::string>;
class SendBuffer_Old
{
public:
	SendBuffer_Old();
	~SendBuffer_Old();
public:
	void Reset();
	void FeedData(const char* data, uint32_t len);
	void TakeData(const char*&data, uint32_t& len, int confirmLength);
private:
	const std::size_t m_capacity;
	std::mutex m_mutex;
	std::string m_bufWait;
	std::string m_bufWork;
	std::size_t m_posWork;//��ǰ��λ��,����ǰ�����ݶ��Ǳ�ʹ�ù�����,���ǹ�ʱ��������
    std::size_t m_posNeedConfirm;//ȡ����������,��Ҫȷ��.
};
/************************************************************************/
/*                                                                      */
/************************************************************************/
class SendBufferEx
{
public:
    class AutoLock
    {
    public:
        AutoLock(std::mutex& mutex, std::string& bufWait, std::string bufWork, std::size_t& posWork) :
            m_mutex(mutex), m_lg(m_mutex), m_bufWait(bufWait), m_bufWork(bufWork), m_posWork(posWork){/*std::cout << "ctor" << std::endl;*/}
        AutoLock(const AutoLock& _o) :m_mutex(_o.m_mutex), m_lg(m_mutex),
            m_bufWait(_o.m_bufWait), m_bufWork(_o.m_bufWork), m_posWork(_o.m_posWork){/*std::cout << "ctor_2" << std::endl;*/}
        ~AutoLock() {/* std::cout << "dtor" << std::endl; */}
        operator bool()const { return true; }
    public:
        std::mutex& m_mutex;
        std::string& m_bufWait;
        std::string& m_bufWork;
        std::size_t& m_posWork;
    private:
        std::lock_guard<std::mutex> m_lg;
    };
public:
    SendBufferEx();
    ~SendBufferEx();
    void reset();
    AutoLock lock();
private:
    const std::size_t m_capacity;
    std::mutex m_mutex;
    std::string m_bufWait;
    std::string m_bufWork;
    std::size_t m_posWork;//��ǰ��λ��,����ǰ�����ݶ��Ǳ�ʹ�ù�����,���ǹ�ʱ��������
};
/************************************************************************/
/*                                                                      */
/************************************************************************/
class RecvBuffer_Old
{
public:
	RecvBuffer_Old();
	~RecvBuffer_Old();
public:
	void Reset();
	int AvaliableSpace(char*& dataHead, uint32_t&dataSize);
	void FeedSize(uint32_t len);
	StdStringPtr NextMessage();
private:
	static bool CrOrLf(char c);
	static StdStringPtr ParseMessage(const char* buff, std::size_t& posBeg, uint32_t posEnd);
private:
	const std::size_t m_capacity;
	const std::size_t m_sortSize;//(�ұߵ�)ʣ��ռ�<=m_sortSizeʱ,�����ڴ�����
	std::string m_buf;
	std::size_t m_posBeg;
	std::size_t m_posEnd;
};
/************************************************************************/
/*                                                                      */
/************************************************************************/
using BoSocketPtr = std::unique_ptr<boost::asio::ip::tcp::socket>;
class TcpSocket
{
private:
    class SendBuffer
    {
    public:
        SendBuffer() :m_capacity(1024 * 512) { reset(); }
    public:
        void reset();
    public:
        const std::size_t m_capacity;
        std::mutex m_mutex;
        std::string m_bufWait;
        std::string m_bufWork;
        std::size_t m_posWork;//��ǰ��λ��,����ǰ�����ݶ��Ǳ�ʹ�ù�����,���ǹ�ʱ��������
        bool m_isSending;
    };
    class RecvBuffer
    {
    public://û����.
        RecvBuffer() :m_capacity(1024 * 512), m_sortSize(1024 * 4), m_isRecving(false) { reset(); }
    public:
        void reset();
    public:
        const std::size_t m_capacity;
        const std::size_t m_sortSize;//(�ұߵ�)ʣ��ռ�<=m_sortSizeʱ,�����ڴ�����
        std::mutex m_mutex;
        std::string m_buf;
        std::size_t m_posBeg;
        std::size_t m_posEnd;
        bool m_isRecving;
    };
public:
    TcpSocket(boost::asio::io_service& io);
    TcpSocket(BoSocketPtr& sock);
public:
    bool isOpen()const;
    void close();
    void connect(const std::string& ip, std::uint16_t port);
	void localPoint() { m_socket->local_endpoint(); }
	void remotePoint() { m_socket->remote_endpoint(); }
    void send(const char* p, std::uint32_t len);
    void sendLoop(std::size_t lastSendLength);
    void sendLoopHandler(const boost::system::error_code& ec, std::size_t bytes_transferred);
    void startRecvAsync();
    void recvAsync();
    void recvHandler(const boost::system::error_code& error, std::size_t bytes_transferred);
	void onRtnStream(){}
	void onRtnMessage(){}
	void onError(){}
private:
	boost::asio::strand m_strand;
	BoSocketPtr m_socket;//�ഴ����ɺ�,ָ��϶���Ϊ��,socketһ���Ǵ��ڵ�.
	SendBuffer m_bufSend;
	RecvBuffer m_bufRecv;
	bool m_isRecving;
	std::atomic_bool m_isSending;
};
#include "xx_impl.hpp"
#endif//XX_HPP
