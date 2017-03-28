#ifndef xx_impl_H
#define xx_impl_H
#include "xx.hpp"
#if 1 //SendBuffer
SendBuffer_Old::SendBuffer_Old() :m_capacity(1024 * 64)
{
	Reset();
}

SendBuffer_Old::~SendBuffer_Old() {}

void SendBuffer_Old::Reset()
{
	std::lock_guard<std::mutex> lg(m_mutex);
	m_bufWait.clear();
	m_bufWork.clear();
	m_posWork = 0;
    m_posNeedConfirm = 0;
	if (m_bufWait.capacity() < m_capacity)
		m_bufWait.reserve(m_capacity);
	if (m_bufWork.capacity() < m_capacity)
		m_bufWork.reserve(m_capacity);
}

void SendBuffer_Old::FeedData(const char* data, uint32_t len)
{
	std::lock_guard<std::mutex> lg(m_mutex);
	m_bufWait.append(data, len);
}


//有安全隐患:指针给外部了,如果外部没有及时使用数据的话,数据可能被内部改动,如果外部强制修改数据的话,数据会有问题
//所以,调用此函数后,在没有使用完得到的数据之前,这个类禁止其他操作,
//如果confirmLength<=0则忽略"待确认的长度的数据"
void SendBuffer_Old::TakeData(const char*&data, uint32_t& len, int confirmLength = -1)
{
    std::lock_guard<std::mutex> lg(m_mutex);
    if (confirmLength < 0)
    {
        m_posWork = m_posNeedConfirm;
    }
    else
    {
        m_posWork += confirmLength;
        assert(m_posWork <= m_posNeedConfirm);
        m_posNeedConfirm = m_posWork;
    }
    if (m_bufWork.size() == m_posWork)
    {
        std::swap(m_bufWork, m_bufWait);
        m_posNeedConfirm = m_posWork = 0;
        m_bufWait.clear();
        //
        data = m_bufWork.c_str();
        len = m_bufWork.size();
        m_posNeedConfirm += len;
    }
    else
    {
        data = &m_bufWork[m_posWork];
        len = m_bufWork.size() - m_posWork;
        m_posNeedConfirm += len;
    }
}
#endif//SendBuffer
/************************************************************************/
/*                                                                      */
/************************************************************************/
#if 1 //SendBufferEx
SendBufferEx::SendBufferEx():m_capacity(1024 * 64){}
SendBufferEx::~SendBufferEx(){}
void SendBufferEx::reset()
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
SendBufferEx::AutoLock SendBufferEx::lock()
{
    return SendBufferEx::AutoLock(m_mutex, m_bufWait, m_bufWork, m_posWork);
}
#endif//SendBufferEx
/************************************************************************/
/*                                                                      */
/************************************************************************/
#if 1 //RecvBuffer
RecvBuffer_Old::RecvBuffer_Old() :m_capacity(1024 * 512), m_sortSize(1024 * 4)
{
	Reset();
}

RecvBuffer_Old::~RecvBuffer_Old() {}

void RecvBuffer_Old::Reset()
{
	if (m_buf.size() < m_capacity)
	{
		m_buf.clear();//resize时,可能会对老的数据执行copy操作,clear后就不会copy了,
		m_buf.resize(m_capacity);
	}
	m_posBeg = 0;
	m_posEnd = 0;
}

int RecvBuffer_Old::AvaliableSpace(char*& dataHead, uint32_t& dataSize)
{
	int retVal = 0;
	do
	{
		//(右边的)剩余空间>m_sortSize时,就直接使用剩余空间
		if (m_posEnd + m_sortSize < m_buf.size())
		{
			dataHead = &m_buf[m_posEnd];
			dataSize = m_buf.size() - m_posEnd;
			retVal = 0;
			break;
		}
		//(右边的)剩余空间<=m_sortSize时,就整理内存,然后使用整理后的剩余空间
		if (0 < m_posBeg)
		{
			memcpy(&m_buf[0], &m_buf[m_posBeg], m_posEnd - m_posBeg);
			m_posEnd = m_posEnd - m_posBeg;
			m_posBeg = 0;
			retVal = AvaliableSpace(dataHead, dataSize);
			break;
		}
		//整理内存失败,
		dataHead = &m_buf[m_posEnd];
		dataSize = m_buf.size() - m_posEnd;
		retVal = (dataSize > 0 ? 0 : -1);
	} while (false);
	return retVal;
}

//TakeData函数不允许修改内存(m_buf)(整理内存)
void RecvBuffer_Old::FeedSize(uint32_t len)
{
	assert(m_posEnd + len <= m_buf.size());
	m_posEnd += len;
}

StdStringPtr RecvBuffer_Old::NextMessage()
{
	StdStringPtr msg = RecvBuffer_Old::ParseMessage(m_buf.c_str(), m_posBeg, m_posEnd);
	assert(m_posBeg <= m_posEnd);
	return msg;
}

bool RecvBuffer_Old::CrOrLf(char c)
{
	return ('\r' == c || '\n' == c) ? true : false;
}

//解析原则:当看到\r或\n时,忽略掉前面的\r和\n,最前面需要是END字符串,END前面的就是json字符串,
//然后偏移到第一个"既不是\r也不是\n"的地方,作为下一个json字符串的起点,
StdStringPtr RecvBuffer_Old::ParseMessage(const char* buff, std::size_t& posBeg, uint32_t posEnd)
{
	StdStringPtr msg = nullptr;
	int32_t msgBeg = -1, msgEnd = -1;//<0,表示未初始化
	for (uint32_t i = posBeg; i < posEnd; ++i)
	{
		//如果没有找到消息头,要先找消息头
		if (msgBeg < 0)
		{
			if (CrOrLf(buff[i]) == false)
				msgBeg = i;
			continue;
		}
		//for循环里面有(i<posEnd),不用担心i超过posEnd的问题,
		//已经有msgBeg这个位置了,所以不用担心(i-1)越界的问题,
		if (CrOrLf(buff[i]) == true && CrOrLf(buff[i - 1]) == false &&
			msgBeg <= int32_t(i - 3) && memcmp(buff + i - 3, "END", 3) == 0)
		{
			msgEnd = i - 3;//一条消息不包括END结束符
			msg = std::make_shared<std::string>(buff + msgBeg, msgEnd - msgBeg);
			posBeg = i;
			break;
		}
	}
	return msg;
}
#endif//RecvBuffer

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
    {
        m_buf.clear();//resize时,可能会对老的数据执行copy操作,clear后就不会copy了,
        m_buf.resize(m_capacity);
    }
    m_posBeg = 0;
    m_posEnd = 0;
}
#endif//TcpSocket::RecvBuffer

TcpSocket::TcpSocket(boost::asio::io_service& io) :m_strand(io)
{
    m_socket = BoSocketPtr(new boost::asio::ip::tcp::socket(io));
}

TcpSocket::TcpSocket(BoSocketPtr& sock) : m_strand(sock->get_io_service())
{
    m_socket = std::move(sock);
}

bool TcpSocket::isOpen()const
{
    return m_socket->is_open();
}

void TcpSocket::close()
{
    m_socket->close();
}

void TcpSocket::connect(const std::string& ip, std::uint16_t port)
{
    boost::system::error_code ec;
    boost::asio::ip::address addr = boost::asio::ip::address::from_string(ip, ec);
    boost::asio::ip::tcp::endpoint ep(addr, port);
    m_socket->connect(ep, ec);
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