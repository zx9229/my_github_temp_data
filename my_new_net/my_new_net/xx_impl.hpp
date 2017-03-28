#ifndef xx_impl_H
#define xx_impl_H
#include "xx.hpp"

#if 1 //TcpSocket

#if 1 //TcpSocket::ErrorId
int TcpSocket::ErrorId::getNextId(int no)
{
	const int& area = 100000;
	std::lock_guard<std::mutex> lg(m_mutex);
	no = std::abs(no) % area;
	m_id = ++m_id % ((21 * 10000 * 10000) / area);
	return -1 * (m_id*area + no);
}
#endif//TcpSocket::ErrorId

#if 1 //TcpSocket::SendBuffer
void TcpSocket::SendBuffer::reset()
{
	std::lock_guard<std::mutex> lg(m_mutex);
	m_bufWait.clear();
	m_bufWork.clear();
	m_posWork = 0;
	if (m_bufWait.capacity() < m_capacity)
		m_bufWait.reserve(m_capacity);
	if (m_bufWork.capacity() < m_capacity)
		m_bufWork.reserve(m_capacity);
}
#endif//TcpSocket::SendBuffer

#if 1 //TcpSocket::RecvBuffer
void TcpSocket::RecvBuffer::reset()
{
	std::lock_guard<std::mutex> lg(m_mutex);
	if (m_buf.size() < m_capacity)
	{ //resize时,可能会对老的数据执行copy操作,clear之后就不会copy了.
		m_buf.clear();
		m_buf.resize(m_capacity);
	}
	m_posBeg = 0;
	m_posEnd = 0;
}
#endif//TcpSocket::RecvBuffer

TcpSocket::TcpSocket(boost::asio::io_service& io) : m_strand(io), m_timer(io)
{
	m_socket = BoostSocketPtr(new boost::asio::ip::tcp::socket(io));
}

TcpSocket::TcpSocket(BoostSocketPtr& sock) : m_strand(sock->get_io_service()), m_timer(sock->get_io_service())
{
	m_socket = std::move(sock);
}

bool TcpSocket::isOpen() const
{
    return m_socket->is_open();
}

void TcpSocket::close()
{
    m_ep = boost::asio::ip::tcp::endpoint();
    m_socket->close();
    //TODO:接收线程可能正在接收数据,或处理数据.
	//TODO:清理发送缓冲区和接收缓冲区.
}

boost::asio::ip::tcp::endpoint TcpSocket::localPoint()
{
    return m_socket->local_endpoint();
}

boost::asio::ip::tcp::endpoint TcpSocket::remotePoint()
{
    return m_socket->remote_endpoint();
}

int TcpSocket::connect(const std::string& ip, std::uint16_t port)
{
	int errId = 0;
	boost::system::error_code ec;
	boost::asio::ip::address addr = boost::asio::ip::address::from_string(ip, ec);
	if (ec)
	{
		errId = m_errId.getNextId(ec.value());
		m_strand.post(boost::bind(&TcpSocket::onError, this, errId, ec.message()));
		return errId;
	}
	m_ep.address(addr);
	m_ep.port(port);
	m_timer.expires_from_now(std::chrono::milliseconds(0));
	m_timer.async_wait(boost::bind(&TcpSocket::doReconnect, this, boost::asio::placeholders::error));
	return errId;
}

void TcpSocket::doReconnect(const boost::system::error_code& ec)
{
	if (ec)
	{
		m_strand.post(boost::bind(&TcpSocket::onError, this, m_errId.getNextId(ec.value()), ec.message()));
		return;
	}
	if (m_ep.port() == 0)
	{
		std::string msg = "boost::asio::ip::tcp::endpoint abnormal, port==0.";
		m_strand.post(boost::bind(&TcpSocket::onError, this, m_errId.getNextId(-1), msg));
		return;
	}
	boost::system::error_code err;
	m_socket->connect(m_ep, err);
	if (err)
	{
		m_strand.post(boost::bind(&TcpSocket::onError, this, m_errId.getNextId(err.value()), err.message()));
		m_timer.expires_from_now(std::chrono::milliseconds(5000));
		m_timer.async_wait(boost::bind(&TcpSocket::doReconnect, this, boost::asio::placeholders::error));
	}
	else
	{
		m_strand.post(boost::bind(&TcpSocket::onConnected, this));
	}
}

void TcpSocket::send(const char* p, std::uint32_t len)
{
    if (true)
    {
        std::lock_guard<std::mutex> lg(m_bufSend.m_mutex);
        m_bufSend.m_bufWait.append(p, len);
        if (m_bufSend.m_isSending)
            return;
    }
    sendAsync(0);
}

void TcpSocket::sendAsync(std::size_t lastLengthSent)
{
	const char* p = nullptr;
	std::uint32_t len = 0;
    if (true)
    {
        std::lock_guard<std::mutex> lg(m_bufSend.m_mutex);
        m_bufSend.m_posWork += lastLengthSent;
        assert(m_bufSend.m_posWork <= m_bufSend.m_bufWork.size());

        if (m_bufSend.m_bufWork.size() == m_bufSend.m_posWork)
        {
            m_bufSend.m_bufWork.clear();
            m_bufSend.m_posWork = 0;
            std::swap(m_bufSend.m_bufWork, m_bufSend.m_bufWait);
        }
        p = &m_bufSend.m_bufWork[m_bufSend.m_posWork];
        len = m_bufSend.m_bufWork.size() - m_bufSend.m_posWork;
        if (len)
        {
            m_bufSend.m_isSending = true;
        }
        else
        {
            m_bufSend.m_isSending = false;
            return;
        }
    }
    //基于什么思考,要使用m_strand.wrap包装一下handler呢?
    m_socket->async_send(boost::asio::buffer(p, len), m_strand.wrap(/*用std::bind会出错*/boost::bind(&TcpSocket::sendAsyncHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
}

void TcpSocket::sendAsyncHandler(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
    if (ec)
    {
		m_strand.post(boost::bind(&TcpSocket::onError, this, m_errId.getNextId(ec.value()), ec.message()));
        return;
    }
    sendAsync(bytes_transferred);
}

void TcpSocket::startRecvAsync()
{
    if (true)
    {
        std::lock_guard<std::mutex> lg(m_bufRecv.m_mutex);
        if (m_bufRecv.m_isRecving)
            return;
        else
            m_bufRecv.m_isRecving = true;
    }
    recvAsync();
}

void TcpSocket::recvAsync()
{
	char* p = nullptr;
	std::uint32_t len = 0;

	if ((m_bufRecv.m_buf.size() - m_bufRecv.m_posEnd < m_bufRecv.m_sortSize) &&
		(0 < m_bufRecv.m_posBeg))
	{ //(右边的)剩余空间<m_sortSize,而且可以整理内存,那么就整理内存.
		memcpy(&m_bufRecv.m_buf[0], &m_bufRecv.m_buf[m_bufRecv.m_posBeg], m_bufRecv.m_posEnd - m_bufRecv.m_posBeg);
		m_bufRecv.m_posEnd = m_bufRecv.m_posEnd - m_bufRecv.m_posBeg;
		m_bufRecv.m_posBeg = 0;
	}
	p = &m_bufRecv.m_buf[m_bufRecv.m_posEnd];
	len = m_bufRecv.m_buf.size() - m_bufRecv.m_posEnd;
	if (!len)
	{
		assert(false);
	}
	m_socket->async_receive(boost::asio::buffer(p, len), m_strand.wrap(boost::bind(&TcpSocket::recvAsyncHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
}

void TcpSocket::recvAsyncHandler(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
	if (true)
	{
		std::lock_guard<std::mutex> lg(m_bufRecv.m_mutex);
		if (ec)
		{
			//TODO:断线.
			m_bufRecv.m_isRecving = false;
			return;
		}
		m_bufRecv.m_posEnd += bytes_transferred;
	}
	recvAsync();
}
#endif//TcpSocket

#endif//xx_impl_H
