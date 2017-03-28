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
}
#endif//TcpSocket::SendBuffer

#if 1 //TcpSocket::RecvBuffer
void TcpSocket::RecvBuffer::reset()
{
    std::lock_guard<std::mutex> lg(m_mutex);
    if (m_buf.size() < m_capacity)
    { //resize时,可能会对老的数据执行copy操作,clear后就不会copy了,
        m_buf.clear();
        m_buf.resize(m_capacity);
    }
    m_posBeg = 0;
    m_posEnd = 0;
}
#endif//TcpSocket::RecvBuffer

TcpSocket::TcpSocket(boost::asio::io_service& io) : m_timer(io), m_strand(io)
{
    m_socket = BoostSocketPtr(new boost::asio::ip::tcp::socket(io));
}

TcpSocket::TcpSocket(BoostSocketPtr& sock) : m_timer(sock->get_io_service()), m_strand(sock->get_io_service())
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
    //接收线程可能正在接收数据,或处理数据.
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
        onError(ec.value(), ec.message());
        return -1;
    }
    m_ep.address(addr);
    m_ep.port(port);
    m_timer.expires_from_now(std::chrono::milliseconds(0));
    m_timer.async_wait(boost::bind(&TcpSocket::doReconnect, this, boost::asio::placeholders::error));
    return 0;
}

void TcpSocket::doReconnect(const boost::system::error_code& ec)
{
    if (ec)
    {
        onError(ec.value(), ec.message());
        return;
    }
    if (m_ep.port() == 0)
    {
        std::string msg = "boost::asio::ip::tcp::endpoint abnormal, port==0.";
        onError(-1, msg);
        return;
    }
    boost::system::error_code err;
    m_socket->connect(m_ep, err);
    if (err)
    {
        onError(err.value(), err.message());
        m_timer.expires_from_now(std::chrono::milliseconds(5000));
        m_timer.async_wait(boost::bind(&TcpSocket::doReconnect, this, boost::asio::placeholders::error));
    }
    else
    {
        //do sth.
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
    sendLoop(0);
}

void TcpSocket::sendLoop(std::size_t lastSendLength)
{
    const char* pData;
    std::uint32_t dataLen;
    if (true)
    {
        std::lock_guard<std::mutex> lg(m_bufSend.m_mutex);
        m_bufSend.m_posWork += lastSendLength;
        assert(m_bufSend.m_posWork < m_bufSend.m_bufWork.size());

        if (m_bufSend.m_bufWork.size() == m_bufSend.m_posWork)
        {
            m_bufSend.m_bufWork.clear();
            m_bufSend.m_posWork = 0;
            std::swap(m_bufSend.m_bufWork, m_bufSend.m_bufWait);
        }
        pData = &m_bufSend.m_bufWork[m_bufSend.m_posWork];
        dataLen = m_bufSend.m_bufWork.size() - m_bufSend.m_posWork;
        if (dataLen)
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
    m_socket->async_send(boost::asio::buffer(pData, dataLen), m_strand.wrap(/*用std::bind会出错*/boost::bind(&TcpSocket::sendLoopHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
}

void TcpSocket::sendLoopHandler(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
    if (ec)
    {
        onError(ec.value(), ec.message());
        return;
    }
    sendLoop(bytes_transferred);
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
    char* dataHead = nullptr;
    std::uint32_t dataSize = 0;

    if (m_bufRecv.m_posEnd + m_bufRecv.m_sortSize < m_bufRecv.m_buf.size())
    {//(右边的)剩余空间>m_sortSize时,就直接使用剩余空间
        dataHead = &m_bufRecv.m_buf[m_bufRecv.m_posEnd];
        dataSize = m_bufRecv.m_buf.size() - m_bufRecv.m_posEnd;
    }
    else if (0 < m_bufRecv.m_posBeg)
    {//(右边的)剩余空间<=m_sortSize时,就整理内存,然后使用整理后的剩余空间
        memcpy(&m_bufRecv.m_buf[0], &m_bufRecv.m_buf[m_bufRecv.m_posBeg], m_bufRecv.m_posEnd - m_bufRecv.m_posBeg);
        m_bufRecv.m_posEnd = m_bufRecv.m_posEnd - m_bufRecv.m_posBeg;
        m_bufRecv.m_posBeg = 0;
        //
        dataHead = &m_bufRecv.m_buf[m_bufRecv.m_posEnd];
        dataSize = m_bufRecv.m_buf.size() - m_bufRecv.m_posEnd;
    }
    else
    {
        assert(false);
    }
    m_socket->async_receive(boost::asio::buffer(dataHead, dataSize), m_strand.wrap(boost::bind(&TcpSocket::recvHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
}

void TcpSocket::recvHandler(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
    if (true)
    {
        std::lock_guard<std::mutex> lg(m_bufRecv.m_mutex);
        if (ec)
        {
            m_bufRecv.m_isRecving = false;
            return;
        }
        m_bufRecv.m_posEnd += bytes_transferred;
    }
    recvAsync();
}


#endif//TcpSocket


#endif//xx_impl_H