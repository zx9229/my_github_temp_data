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
    { //resizeʱ,���ܻ���ϵ�����ִ��copy����,clear֮��Ͳ���copy��.
        m_buf.clear();
        m_buf.resize(m_capacity);
    }
    m_posBeg = 0;
    m_posEnd = 0;
    m_isRecving = false;
}
#endif//TcpSocket::RecvBuffer

TcpSocket::TcpSocket(boost::asio::io_service& io)
    : m_strand(io), m_timer(io), m_isWorking(false)
{
    m_socket = BoostSocketPtr(new boost::asio::ip::tcp::socket(io));
}

TcpSocket::TcpSocket(BoostSocketPtr& sock, bool isWorking)
    : m_strand(sock->get_io_service()), m_timer(sock->get_io_service()), m_isWorking(isWorking)
{
    m_socket = std::move(sock);
}

/************************************************************************/
/* ���TcpSocket�ǲ���һ����Ч��(���ڹ���״̬��)TcpSocket.                 */
/* ������TcpSocket��һ�����ӵ������˿ڵ�socket,��ô�����ܴ��ڵ�״̬��:      */
/* ���������С����������С���������+�����շ�������                          */
/* ������TcpSocket��һ��accept������socket,��ô�����ܴ��ڵ�״̬��:        */
/* ��������+�����շ�������,������ӶϿ�,ֱ��m_isWorking��FALSE,�޷���������  */
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
    return m_socket->is_open() ? m_socket->local_endpoint() : boost::asio::ip::tcp::endpoint();
    //return m_socket->local_endpoint();
}

boost::asio::ip::tcp::endpoint TcpSocket::remotePoint()
{
    return m_socket->is_open() ? m_socket->remote_endpoint() : boost::asio::ip::tcp::endpoint();
    //return m_socket->remote_endpoint();
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
    boost::asio::ip::tcp::endpoint peerPoint(addr, port);
    return doWorkStart(peerPoint);
}

void TcpSocket::close()
{
    boost::system::error_code ec(boost::asio::error::network_down);
    doWorkStop(ec);
}

#if 1 //callbacks.
void TcpSocket::onConnected()
{
    boost::asio::ip::tcp::endpoint lp = localPoint();
    boost::asio::ip::tcp::endpoint rp = remotePoint();
    char bufI2a[32];
    _itoa_s(lp.port(), bufI2a, 10);
    std::string lpStr = "local_Point:(" + lp.address().to_string() + "," + bufI2a + ")";
    _itoa_s(rp.port(), bufI2a, 10);
    std::string rpStr = "remotePoint:(" + rp.address().to_string() + "," + bufI2a + ")";
    std::cout << "onConnected," << lpStr << "," << rpStr << std::endl;
}
void TcpSocket::onDisconnected(const boost::system::error_code& ec)
{
    boost::asio::ip::tcp::endpoint lp = localPoint();
    boost::asio::ip::tcp::endpoint rp = remotePoint();
    std::cout << "onDisconnected," << ec.value() << "," << ec.message() << std::endl;
}
void TcpSocket::onError(int errId, std::string errMsg)
{
    std::cout << "onError," << errId << "," << errMsg << std::endl;
}
void TcpSocket::onRtnStream() {}
void TcpSocket::onRtnMessage() {}
#endif//callbacks.

/************************************************************************/
/* ֻ��doConnected��doDisconnected�����޸�m_isConnected,��������ֻ�����ȡ.*/
/************************************************************************/
void TcpSocket::doConnected()
{
    //��֤ onConnected �� onDisconnected �ɶԵı��ص�.
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
/* ֻ��doConnected��doDisconnected�����޸�m_isConnected,��������ֻ�����ȡ.*/
/************************************************************************/
void TcpSocket::doDisconnected(const boost::system::error_code& ec)
{
    //��֤ onConnected �� onDisconnected �ɶԵı��ص�.
    if (m_isConnected.exchange(false) == true)
    {
        m_strand.post(boost::bind(&TcpSocket::onDisconnected, this, ec));
    }
}
/************************************************************************/
/* ֻ��doWorkStart��doWorkStop�����޸�m_isWorking��ֵ                     */
/* ֻ��doWorkStart���Խ�m_isWorking�޸ĳ�true                            */
/************************************************************************/
int TcpSocket::doWorkStart(const boost::asio::ip::tcp::endpoint& peerPoint)
{
    if (m_isWorking.exchange(true) == true)
    {
        std::string msg = TcpSocket_is_already_working;
        m_strand.post(boost::bind(&TcpSocket::onError, this, -1, msg));
        return -1;
    }
    //m_peerPointû�м���,����port==0�ж��Ƿ���Ҫ����.
    m_peerPoint.address(peerPoint.address());
    m_peerPoint.port(peerPoint.port());
    doReconnectStart(boost::system::error_code());
    return 0;
}
/************************************************************************/
/* ֻ��doWorkStart��doWorkStop�����޸�m_isWorking��ֵ                     */
/* ֻ��doWorkStop���Խ�m_isWorking�޸ĳ�false                             */
/************************************************************************/
void TcpSocket::doWorkStop(const boost::system::error_code& ec)
{
    if (m_isWorking.exchange(false) == false)
    {
        return;
    }
    doReconnectStop();
    doStopBoostSocket(ec);
}

void TcpSocket::doStopBoostSocket(const boost::system::error_code& ec)
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
        //m_strand.post(boost::bind(&TcpSocket::onError, this, err.value(), err.message()));
    }
    doDisconnected(ec);
}

void TcpSocket::doReconnectStart(const boost::system::error_code& ec)
{
    if (m_isConnecting.exchange(true) == true)
    {
        return;
    }

    //m_isConnected�ֶ���doStopBoostSocket���doDisconnected���汻�޸���.
    doStopBoostSocket(ec);

    m_timer.expires_from_now(std::chrono::milliseconds(0));
    m_timer.async_wait(boost::bind(&TcpSocket::doReconnectImpl, this, boost::asio::placeholders::error));
}
void TcpSocket::doReconnectStop()
{
    m_timer.cancel();
    m_isConnecting.exchange(false);
}
/************************************************************************/
/* ��(����)�����Ĺ�����,m_isWorking==true,m_isConnected==false,           */
/* ����������Ĺ�����,����peer_endpoint�ǿ�ֵ,����Ϊ���socket��һ���޷����� */
/* ��socket,��ʱ�ͻ���m_isWorking==false(��ʱm_isConnected�ض���false),   */
/* ͬʱ�˳��������߼�.                                                    */
/************************************************************************/
void TcpSocket::doReconnectImpl(const boost::system::error_code& ec)
{
    if (m_isWorking.load() == false)
    {
        doReconnectStop();
        doWorkStop(ec);
        return;
    }
    assert(m_isConnected.load() == false);
    assert(m_isConnecting.load() == true);
    if (ec)
    {
        m_strand.post(boost::bind(&TcpSocket::onError, this, ec.value(), ec.message()));
        doReconnectStop();
        doWorkStop(ec);
        return;
    }
    if (m_peerPoint.port() == 0)
    {
        std::string msg = peer_endpoint_is_unreachable;
        m_strand.post(boost::bind(&TcpSocket::onError, this, -1, msg));
        doReconnectStop();
        doWorkStop(boost::system::error_code(boost::asio::error::network_unreachable));
        return;
    }
    boost::system::error_code err;
    m_socket->connect(m_peerPoint, err);
    if (err)
    {
        m_strand.post(boost::bind(&TcpSocket::onError, this, err.value(), err.message()));
        m_timer.expires_from_now(std::chrono::milliseconds(5000));
        m_timer.async_wait(boost::bind(&TcpSocket::doReconnectImpl, this, boost::asio::placeholders::error));
    }
    else
    {
        doReconnectStop();
        doConnected();
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
    //��û��Ҫд�����ж�socket��״̬,��asio���Ҽ�鼴��.
    m_socket->async_send(boost::asio::buffer(bufSend, bufSize), m_strand.wrap(/*��std::bind�����*/boost::bind(&TcpSocket::doSendAsyncHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
}

void TcpSocket::doSendAsyncHandler(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
    if (ec)
    {
        m_sendBuf.reset();
        m_strand.post(boost::bind(&TcpSocket::onError, this, ec.value(), ec.message()));
        doReconnectStart(ec);
    }
    else
    {
        doSendAsync(bytes_transferred);
    }
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

    std::lock_guard<std::mutex> lg(m_recvBuf.m_mutex);
    if (m_recvBuf.m_isRecving)
    {
        std::string msg = TcpSocket_is_already_receiving;
        m_strand.post(boost::bind(&TcpSocket::onError, this, -1, msg));
        return -1;
    }
    else
    {
        m_recvBuf.m_isRecving = true;
        doRecvAsync(0);
        return 0;
    }
}
/************************************************************************/
/* �������߼�����ʱ,Ҫ��һʱ������Լ���buffer,��Ϊ�����߼��������,         */
/* ����Ļ�,��������Ҫ��ս���bufferʱ,���߼������м��ʳ����.               */
/************************************************************************/
void TcpSocket::doRecvAsync(std::size_t lastLengthReceived)
{
    char* bufRecv = nullptr;
    std::uint32_t bufSize = 0;

    if (true)
    {
        //����Ҫ����.����,��Ϊ�ǻ�����,�����Ļ�,ע�ⲻҪ����.
        //std::lock_guard<std::mutex> lg(m_recvBuf.m_mutex);
        m_recvBuf.m_posEnd += lastLengthReceived;
        if ((m_recvBuf.m_buf.size() - m_recvBuf.m_posEnd < m_recvBuf.m_sortSize) &&
            (0 < m_recvBuf.m_posBeg))
        { //(�ұߵ�)ʣ��ռ�<m_sortSize,���ҿ��������ڴ�,��ô�������ڴ�.
            memcpy(&m_recvBuf.m_buf[0], &m_recvBuf.m_buf[m_recvBuf.m_posBeg], m_recvBuf.m_posEnd - m_recvBuf.m_posBeg);
            m_recvBuf.m_posEnd = m_recvBuf.m_posEnd - m_recvBuf.m_posBeg;
            m_recvBuf.m_posBeg = 0;
        }
        bufRecv = &m_recvBuf.m_buf[m_recvBuf.m_posEnd];
        bufSize = m_recvBuf.m_buf.size() - m_recvBuf.m_posEnd;
    }

    if (bufSize)
    {
        //�����ʱsocket���ر���,��ô�ص������϶��������������Ϣ,�����ҿ�����asio���Ҽ��socket��״̬,��û��Ҫд�߼������.
        m_socket->async_receive(boost::asio::buffer(bufRecv, bufSize), m_strand.wrap(boost::bind(&TcpSocket::doRecvAsyncHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
    }
    else
    {
        boost::system::error_code ec(boost::asio::error::message_size);
        m_recvBuf.reset();
        m_strand.post(boost::bind(&TcpSocket::onError, this, ec.value(), ec.message()));
        doReconnectStart(ec);
    }
}
/************************************************************************/
/* �������߼�����ʱ,Ҫ��һʱ������Լ���buffer,��Ϊ�����߼��������,         */
/* ����Ļ�,��������Ҫ��ս���bufferʱ,���߼������м��ʳ����.               */
/************************************************************************/
void TcpSocket::doRecvAsyncHandler(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
    if (ec)
    {
        m_recvBuf.reset();
        m_strand.post(boost::bind(&TcpSocket::onError, this, ec.value(), ec.message()));
        doReconnectStart(ec);
    }
    else
    {
        doRecvAsync(bytes_transferred);
    }
}
#endif//TcpSocket

#endif//xx_impl_H
