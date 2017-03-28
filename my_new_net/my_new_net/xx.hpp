#ifndef XX_HPP
#define XX_HPP
#include <cstdint>
#include <mutex>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/bind.hpp>
using StdStringPtr = std::shared_ptr<std::string>;
class SendBuffer
{
public:
	SendBuffer();
	~SendBuffer();
public:
	void Reset();
	void FeedData(const char* data, uint32_t len);
	void TakeData(const char*&data, uint32_t& len, int confirmLength);
private:
	const std::size_t m_capacity;
	std::mutex m_mutex;
	std::string m_bufWait;
	std::string m_bufWork;
	std::size_t m_posWork;//当前的位置,再往前的数据都是被使用过的了,都是过时的数据了
    std::size_t m_posNeedConfirm;//取出来的数据,需要确认.
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
            m_mutex(mutex), m_lg(m_mutex), m_bufWait(bufWait), m_bufWork(bufWait), m_posWork(posWork) {}
        AutoLock(const AutoLock& _o) :m_mutex(_o.m_mutex), m_lg(m_mutex),
            m_bufWait(_o.m_bufWait), m_bufWork(_o.m_bufWork), m_posWork(_o.m_posWork) {}
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
    std::size_t m_posWork;//当前的位置,再往前的数据都是被使用过的了,都是过时的数据了
};
/************************************************************************/
/*                                                                      */
/************************************************************************/
class RecvBuffer
{
public:
	RecvBuffer();
	~RecvBuffer();
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
	const std::size_t m_sortSize;//(右边的)剩余空间<=m_sortSize时,进行内存整理
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
public:
	TcpSocket(boost::asio::io_service& io) :m_strand(io) { m_socket = BoSocketPtr(new boost::asio::ip::tcp::socket(io)); }
	TcpSocket(BoSocketPtr& sock) :m_strand(sock->get_io_service()) { m_socket = std::move(sock); }
public:
	bool isOpen()const { return m_socket->is_open(); }
	void close() { m_socket->close(); }
	void connect(const std::string& ip, std::uint16_t port)
	{
		boost::system::error_code ec;
		boost::asio::ip::address addr = boost::asio::ip::address::from_string(ip, ec);
		boost::asio::ip::tcp::endpoint ep(addr, port);
		m_socket->connect(ep, ec);
	}
	void localPoint() { m_socket->local_endpoint(); }
	void remotePoint() { m_socket->remote_endpoint(); }
    void send(const char* p, std::uint32_t len)
    {
        if (p&&len)
            m_bufSend.FeedData(p, len);
        if (!m_isSending.load())
            sendLoop(0);
    }
	void sendLoop(std::uint32_t lastSendLength)
	{
        const char* pData;
        std::uint32_t dataLen;
        m_bufSend.TakeData(pData, dataLen, lastSendLength);
        if (!dataLen)
        {
            if (m_isSending.load())
            {
                m_isSending = false;
                send(nullptr, 0);
            }
            else
            {
                return;
            }
        }
		//m_socket->send(boost::asio::buffer(pData, dataLen));
		//基于什么思考,要使用m_strand.wrap包装一下handler呢?
		m_socket->async_send(boost::asio::buffer(pData, dataLen), m_strand.wrap(/*用std::bind会出错*/boost::bind(&TcpSocket::sendLoopHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
	}
    void sendLoopHandler(const boost::system::error_code& ec, std::size_t bytes_transferred)
    {
        sendLoop(bytes_transferred);
    }
	void recv()
	{
		char* dataHead;
		std::uint32_t dataSize;
		int rv = m_bufRecv.AvaliableSpace(dataHead, dataSize);
		std::size_t recvSize = m_socket->receive(boost::asio::buffer(dataHead,dataSize));
		m_bufRecv.FeedSize(recvSize);
	}
	void onRtnStream(){}
	void onRtnMessage(){}
	void onError(){}
private:
	boost::asio::strand m_strand;
	BoSocketPtr m_socket;//类创建完成后,指针肯定不为空,socket一定是存在的.
	SendBuffer m_bufSend;
	RecvBuffer m_bufRecv;
	bool m_isRecving;
	std::atomic_bool m_isSending;
};
#include "xx_impl.hpp"
#endif//XX_HPP
