#ifndef xx_impl_H
#define xx_impl_H
#include "xx.hpp"

#if 1 //TcpSocket

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
    m_isSending = false;
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
    m_isRecving = false;
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

/************************************************************************/
/* 这个TcpSocket是不是一个有效的(处于工作状态的)TcpSocket.                 */
/* 如果这个TcpSocket是一个连接到监听端口的socket,那么它可能处于的状态有:      */
/* 正在连接中、断线重连中、连接正常+正在收发数据中                          */
/* 如果这个TcpSocket是一个accept出来的socket,那么它可能处于的状态有:        */
/* 连接正常+正在收发数据中,如果连接断开,直接m_isWorking是FALSE,无法断线重连  */
/*                                                                      */
/************************************************************************/
bool TcpSocket::isWorking() const
{
    return m_isWorking.load();
}

bool TcpSocket::isConnected() const
{
    return m_isConnected.load();
}

bool TcpSocket::isOpen() const
{
    return m_socket->is_open();
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
	boost::system::error_code ec;
	boost::asio::ip::address addr = boost::asio::ip::address::from_string(ip, ec);
	if (ec)
	{
		m_strand.post(boost::bind(&TcpSocket::onError, this, ec.value(), ec.message()));
		return ec.value();
	}
	if (m_isConnected.load())
	{
		std::string msg = TcpSocket_has_been_connected;
		m_strand.post(boost::bind(&TcpSocket::onError, this, -1, msg));
		return -1;
	}
	if (m_isWorking.exchange(true) == true)
	{
		std::string msg = TcpSocket_is_already_working;
		m_strand.post(boost::bind(&TcpSocket::onError, this, -1, msg));
		return -1;
	}
	m_peerPoint.address(addr);
	m_peerPoint.port(port);
	m_timer.expires_from_now(std::chrono::milliseconds(0));
	m_timer.async_wait(boost::bind(&TcpSocket::doReconnect, this, boost::asio::placeholders::error));
	return 0;
}

void TcpSocket::close()
{
    boost::system::error_code ec(boost::asio::error::network_down);
    doStopWork(ec);
}

/************************************************************************/
/* 只有doConnected和doDisconnected允许修改m_isConnected,其它函数只允许读取.*/
/************************************************************************/
void TcpSocket::doConnected()
{
    //保证 onConnected 和 onDisconnected 成对的被回调.
	if (m_isConnected.exchange(true) == false)
	{
		m_strand.post(boost::bind(&TcpSocket::onConnected, this));
	}
	else
	{
		assert(false);
	}
}

/************************************************************************/
/* 只有doConnected和doDisconnected允许修改m_isConnected,其它函数只允许读取.*/
/************************************************************************/
void TcpSocket::doDisconnected(const boost::system::error_code& ec)
{
    //保证 onConnected 和 onDisconnected 成对的被回调.
    if (m_isConnected.exchange(false) == true)
    {
        m_strand.post(boost::bind(&TcpSocket::onDisconnected, this, ec));
    }
    //else
    //{
    //    assert(false);
    //}
}



void TcpSocket::doStopWork(const boost::system::error_code& ec)
{
    if (m_isWorking.exchange(false) == false)
    {
        return;
    }
    m_timer.cancel();
    m_peerPoint = boost::asio::ip::tcp::endpoint();
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
    //doClose的过程中,可能socket一直没有连接成功,但是在重连的过程中.
    doDisconnected(ec);
    //TODO:接收线程可能正在接收数据,或处理数据.
    //TODO:清理发送缓冲区和接收缓冲区.
    m_sendBuf.reset();
    m_recvBuf.reset();
}

void TcpSocket::doStopConnection(const boost::system::error_code& ec)
{
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
    doDisconnected(ec);
    //TODO:接收线程可能正在接收数据,或处理数据.
    //TODO:清理发送缓冲区和接收缓冲区.
    m_sendBuf.reset();
    m_recvBuf.reset();
}

void TcpSocket::doReconnectEntry(const boost::system::error_code& ec)
{
    if (m_isConnecting.exchange(true) == true)
    {
        return;
    }
    //m_isConnected字段在doStopConnection里面被修改了.
    doStopConnection(ec);

    m_timer.expires_from_now(std::chrono::milliseconds(0));
    m_timer.async_wait(boost::bind(&TcpSocket::doReconnect, this, boost::asio::placeholders::error));
}
/************************************************************************/
/* 在(断线)重连的过程中,m_isWorking==true,m_isConnected==false,           */
/* 如果在重连的过程中,发现peer_endpoint是空值,就认为这个socket是一个无法重连 */
/* 的socket,此时就会让m_isWorking==false(此时m_isConnected必定是false),   */
/* 同时退出重连的逻辑.                                                    */
/************************************************************************/
void TcpSocket::doReconnect(const boost::system::error_code& ec)
{
    if (m_isWorking.load() == false)
    {
        doStopWork(ec);
        return;
    }
    assert(m_isConnected.load() == false);
    assert(m_isConnecting.load() == true);
    if (ec)
    {
        m_strand.post(boost::bind(&TcpSocket::onError, this, ec.value(), ec.message()));
        doStopWork(ec);
        return;
    }
    if (boost::asio::ip::tcp::endpoint() == m_peerPoint)
    {
        std::string msg = Invalid_peer_endpoint;
        m_strand.post(boost::bind(&TcpSocket::onError, this, -1, msg));
        doStopWork(boost::system::error_code(boost::asio::error::network_unreachable));
        return;
    }
    boost::system::error_code err;
    m_socket->connect(m_peerPoint, err);
    if (err)
    {
        m_strand.post(boost::bind(&TcpSocket::onError, this, err.value(), err.message()));
        m_timer.expires_from_now(std::chrono::milliseconds(5000));
        m_timer.async_wait(boost::bind(&TcpSocket::doReconnect, this, boost::asio::placeholders::error));
    }
    else
    {
        doConnected();
        m_isConnecting.exchange(false);
        startRecvAsync();
    }
}

int TcpSocket::send(const char* p, std::uint32_t len)
{
    if (m_isWorking.load() == false)
    {
        std::string msg = TcpSocket_is_no_longer_working;
        m_strand.post(boost::bind(&TcpSocket::onError, this, -1, msg));
        return -1;
    }
    if (true)
    {
        std::lock_guard<std::mutex> lg(m_sendBuf.m_mutex);
        m_sendBuf.m_bufWait.append(p, len);
        if (m_sendBuf.m_isSending)
            return 0;
    }
    doSendAsync(0);
    return 0;
}

void TcpSocket::doSendAsync(std::size_t lastLengthSent)
{
    const char* bufSend = nullptr;
    std::uint32_t bufSize = 0;
    if (true)
    {
        std::lock_guard<std::mutex> lg(m_sendBuf.m_mutex);
        m_sendBuf.m_posWork += lastLengthSent;
        assert(m_sendBuf.m_posWork <= m_sendBuf.m_bufWork.size());

        if (m_sendBuf.m_bufWork.size() == m_sendBuf.m_posWork)
        {
            m_sendBuf.m_bufWork.clear();
            m_sendBuf.m_posWork = 0;
            std::swap(m_sendBuf.m_bufWork, m_sendBuf.m_bufWait);
        }
        bufSend = &m_sendBuf.m_bufWork[m_sendBuf.m_posWork];
        bufSize = m_sendBuf.m_bufWork.size() - m_sendBuf.m_posWork;
        if (bufSize)
        {
            m_sendBuf.m_isSending = true;
        }
        else
        {
            m_sendBuf.m_isSending = false;
            return;
        }
    }
    //我没必要写代码判断socket的状态,让asio代我检查即可.
    m_socket->async_send(boost::asio::buffer(bufSend, bufSize), m_strand.wrap(/*用std::bind会出错*/boost::bind(&TcpSocket::doSendAsyncHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
}

void TcpSocket::doSendAsyncHandler(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
    if (ec)
    {
        doStopWork(ec);
        m_strand.post(boost::bind(&TcpSocket::onError, this, ec.value(), ec.message()));
        return;
    }
    doSendAsync(bytes_transferred);
}

int TcpSocket::startRecvAsync()
{
    if (m_isWorking.load() == false)
    {
        std::string msg = TcpSocket_is_no_longer_working;
        m_strand.post(boost::bind(&TcpSocket::onError, this, -1, msg));
        return -1;
    }
    if (m_isConnected.load() == false)
    {
        std::string msg = TcpSocket_is_not_connected;
        m_strand.post(boost::bind(&TcpSocket::onError, this, -1, msg));
        return -1;
    }
    bool needRecv = true;
    if (true)
    {
        std::lock_guard<std::mutex> lg(m_recvBuf.m_mutex);
        needRecv = m_recvBuf.m_isRecving ? false : true;
        m_recvBuf.m_isRecving = true;
    }
    if (!needRecv)
    {
        std::string msg = TcpSocket_is_already_receiving;
        m_strand.post(boost::bind(&TcpSocket::onError, this, -1, msg));
        return -1;
    }
    doRecvAsync(0);
    return 0;
}

void TcpSocket::doRecvAsync(std::size_t lastLengthReceived)
{
    char* bufRecv = nullptr;
    std::uint32_t bufSize = 0;

    if (true)
    {
        std::lock_guard<std::mutex> lg(m_recvBuf.m_mutex);
        m_recvBuf.m_posEnd += lastLengthReceived;
        if ((m_recvBuf.m_buf.size() - m_recvBuf.m_posEnd < m_recvBuf.m_sortSize) &&
            (0 < m_recvBuf.m_posBeg))
        { //(右边的)剩余空间<m_sortSize,而且可以整理内存,那么就整理内存.
            memcpy(&m_recvBuf.m_buf[0], &m_recvBuf.m_buf[m_recvBuf.m_posBeg], m_recvBuf.m_posEnd - m_recvBuf.m_posBeg);
            m_recvBuf.m_posEnd = m_recvBuf.m_posEnd - m_recvBuf.m_posBeg;
            m_recvBuf.m_posBeg = 0;
        }
        bufRecv = &m_recvBuf.m_buf[m_recvBuf.m_posEnd];
        bufSize = m_recvBuf.m_buf.size() - m_recvBuf.m_posEnd;
    }
    
    if (bufSize)
    {
        //如果此时socket被关闭了,那么回调函数肯定会带出来错误消息,所以我可以让asio代我检查socket的状态,我没必要写逻辑检查它.
        m_socket->async_receive(boost::asio::buffer(bufRecv, bufSize), m_strand.wrap(boost::bind(&TcpSocket::doRecvAsyncHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
    }
    else
    {
        doStopWork(boost::system::error_code(boost::asio::error::message_size));
    }
}

void TcpSocket::doRecvAsyncHandler(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
    if (ec)
    {
        doStopWork(ec);
        m_strand.post(boost::bind(&TcpSocket::onError, this, ec.value(), ec.message()));
        return;
    }

    doRecvAsync(bytes_transferred);
}
#endif//TcpSocket

#endif//xx_impl_H
