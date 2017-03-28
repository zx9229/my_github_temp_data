#ifndef XX_HPP
#define XX_HPP
#include <cstdint>
#include <mutex>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/bind.hpp>
/************************************************************************/
/*                                                                      */
/************************************************************************/
using BoostSocketPtr = std::unique_ptr<boost::asio::ip::tcp::socket>;
class TcpSocket
{
private:
	class ErrorId
	{
	public:
		ErrorId() :m_id(0) {}
	public:
		int getNextId(int no);
	private:
		std::mutex m_mutex;
		int m_id;
	};
	class SendBuffer
	{
	public:
		SendBuffer() :m_capacity(1024 * 512), m_isSending(false) { reset(); }
	public:
		void reset();
	public:
		const std::size_t m_capacity;
		std::mutex  m_mutex;
		std::string m_bufWait;
		std::string m_bufWork;
		std::size_t m_posWork;//当前的位置,再往前的数据都被发送过了,都是过时的数据了.
		bool        m_isSending;
	};
	class RecvBuffer
	{
	public:
		RecvBuffer() :m_capacity(1024 * 512), m_sortSize(1024 * 4), m_isRecving(false) { reset(); }
	public:
		void reset();
	public:
		const std::size_t m_capacity;
		const std::size_t m_sortSize;//(右边的)剩余空间<=m_sortSize时,进行内存整理
		std::mutex  m_mutex;
		std::string m_buf;
		std::size_t m_posBeg;
		std::size_t m_posEnd;
		bool        m_isRecving;
	};
public:
	TcpSocket(boost::asio::io_service& io);
	TcpSocket(BoostSocketPtr& sock);
public:
	bool isOpen() const;
	void close();
	boost::asio::ip::tcp::endpoint localPoint();
	boost::asio::ip::tcp::endpoint remotePoint();
	int connect(const std::string& ip, std::uint16_t port);
	void send(const char* p, std::uint32_t len);
	void startRecvAsync();
	void onRtnStream() {}
	void onRtnMessage() {}
	void onError(int errId, std::string errMsg) {}
	void onConnected(){}
private:
	void doReconnect(const boost::system::error_code& ec);
	void sendAsync(std::size_t lastLengthSent);
	void sendAsyncHandler(const boost::system::error_code& ec, std::size_t bytes_transferred);
	void recvAsync();
	void recvAsyncHandler(const boost::system::error_code& error, std::size_t bytes_transferred);
private:
	boost::asio::strand m_strand;
	BoostSocketPtr m_socket;//要求:类创建完成后,指针肯定不为空,socket一定是存在的.
	boost::asio::steady_timer m_timer;
	boost::asio::ip::tcp::endpoint m_ep;
	SendBuffer m_bufSend;
	RecvBuffer m_bufRecv;
	ErrorId m_errId;
};
#include "xx_impl.hpp"
#endif//XX_HPP
