#ifndef xx_impl_H
#define xx_impl_H
#include "xx.hpp"

#if 1 //TcpSocket

#if 0 //TcpSocket::ErrorId
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

TcpSocket::TcpSocket(boost::asio::io_service& io) : m_strand(io), m_timer(io), m_isWorking(false)
{
	m_socket = BoostSocketPtr(new boost::asio::ip::tcp::socket(io));
}

TcpSocket::TcpSocket(BoostSocketPtr& sock) : m_strand(sock->get_io_service()), m_timer(sock->get_io_service()), m_isWorking(false)
{
	m_socket = std::move(sock);
}

bool TcpSocket::isWorking() const
{
    return m_isWorking;
}

bool TcpSocket::isOpen() const
{
    return m_socket->is_open();
}

void TcpSocket::close()
{
	boost::system::error_code ec(boost::asio::error::network_down);
	doClose(ec);
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
    if (m_isWorking.exchange(true) == true)
    {
		errId = -1;
        std::string msg = "The socket is already working.";
        m_strand.post(boost::bind(&TcpSocket::onError, this, errId, msg));
        return errId;
    }
    boost::system::error_code ec;
    boost::asio::ip::address addr = boost::asio::ip::address::from_string(ip, ec);
	if (ec)
	{
		m_isWorking.exchange(false);
		errId = ec.value();
		m_strand.post(boost::bind(&TcpSocket::onError, this, errId, ec.message()));
		return errId;
	}
    m_ep.address(addr);
    m_ep.port(port);
    m_timer.expires_from_now(std::chrono::milliseconds(0));
    m_timer.async_wait(boost::bind(&TcpSocket::doReconnect, this, boost::asio::placeholders::error));
    return errId;
}

void TcpSocket::doClose(const boost::system::error_code& ec)
{
	if (m_isWorking.exchange(false) == false)
	{
		return;
	}
	m_ep = boost::asio::ip::tcp::endpoint();
	boost::system::error_code err;
	m_socket->close(err);
	if (err)
	{
		m_strand.post(boost::bind(&TcpSocket::onError, this, err.value(), err.message()));
	}
	m_socket->shutdown(boost::asio::socket_base::shutdown_both, err);
	if (err)
	{
		m_strand.post(boost::bind(&TcpSocket::onError, this, err.value(), err.message()));
	}
	//保证 onConnected 和 onDisconnected 成对的被回调.
	if (m_isConnected.exchange(false) == true)
	{
		m_strand.post(boost::bind(&TcpSocket::onDisconnected, this, ec));
	}
	//TODO:接收线程可能正在接收数据,或处理数据.
	//TODO:清理发送缓冲区和接收缓冲区.
	m_bufSend.reset();
	m_bufRecv.reset();
}

void TcpSocket::doReconnect(const boost::system::error_code& ec)
{
    if (m_isWorking.load() == false)
    {
        return;
    }
	if (ec)
	{
		m_isWorking.exchange(false);
		m_strand.post(boost::bind(&TcpSocket::onError, this, ec.value(), ec.message()));
		return;
	}
	boost::system::error_code err;
	m_socket->connect(m_ep, err);
	if (err)
	{
		m_strand.post(boost::bind(&TcpSocket::onError, this, err.value(), err.message()));
		m_timer.expires_from_now(std::chrono::milliseconds(5000));
		m_timer.async_wait(boost::bind(&TcpSocket::doReconnect, this, boost::asio::placeholders::error));
	}
	else
	{
		m_strand.post(boost::bind(&TcpSocket::onConnected, this));
        startRecvAsync();
	}
}

int TcpSocket::send(const char* p, std::uint32_t len)
{
	int errId = 0;
	if (m_isWorking.load() == false)
	{
		errId = -1;
		std::string msg = "The socket is no longer working.";
		m_strand.post(boost::bind(&TcpSocket::onError, this, errId, msg));
		return errId;
	}
	if (true)
	{
		std::lock_guard<std::mutex> lg(m_bufSend.m_mutex);
		m_bufSend.m_bufWait.append(p, len);
		if (m_bufSend.m_isSending)
			return errId;
	}
	doSendAsync(0);
	return errId;
}

void TcpSocket::doSendAsync(std::size_t lastLengthSent)
{
	if (m_isWorking.load() == false)
		return;

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
    m_socket->async_send(boost::asio::buffer(p, len), m_strand.wrap(/*用std::bind会出错*/boost::bind(&TcpSocket::doSendAsyncHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
}

void TcpSocket::doSendAsyncHandler(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
    if (ec)
    {
		m_strand.post(boost::bind(&TcpSocket::onError, this, ec.value(), ec.message()));
        return;
    }
    doSendAsync(bytes_transferred);
}

int TcpSocket::startRecvAsync()
{
	int errId = 0;
	if (m_isWorking.load() == false)
	{
		errId = -1;
		std::string msg = "The socket is no longer working.";
		m_strand.post(boost::bind(&TcpSocket::onError, this, errId, msg));
		return errId;
	}
	bool needRecv = true;
	if (true)
	{
		std::lock_guard<std::mutex> lg(m_bufRecv.m_mutex);
		if (m_bufRecv.m_isRecving)
			needRecv = false;
		else
			m_bufRecv.m_isRecving = true;
	}
	if (!needRecv)
	{
		errId = -1;
		std::string msg = "The socket is already receiving.";
		m_strand.post(boost::bind(&TcpSocket::onError, this, errId, msg));
		return errId;
	}
	doRecvAsync();
	return errId;
}

void TcpSocket::doRecvAsync()
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
	m_socket->async_receive(boost::asio::buffer(p, len), m_strand.wrap(boost::bind(&TcpSocket::doRecvAsyncHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
}

void TcpSocket::doRecvAsyncHandler(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
	if (true)
	{
		std::lock_guard<std::mutex> lg(m_bufRecv.m_mutex);
		if (ec)
		{
			boost::system::error_code err(boost::asio::error::network_down);
            m_strand.post(boost::bind(&TcpSocket::onDisconnected, this, ec));
			m_bufRecv.m_isRecving = false;
			return;
		}
		m_bufRecv.m_posEnd += bytes_transferred;
	}
	if (m_isWorking.load() == false)
	{
		std::string msg = "The socket is no longer working.";
		m_strand.post(boost::bind(&TcpSocket::onError, this, -1, msg));
		return;
	}
	doRecvAsync();
}
#endif//TcpSocket

#endif//xx_impl_H
