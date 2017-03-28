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


//�а�ȫ����:ָ����ⲿ��,����ⲿû�м�ʱʹ�����ݵĻ�,���ݿ��ܱ��ڲ��Ķ�,����ⲿǿ���޸����ݵĻ�,���ݻ�������
//����,���ô˺�����,��û��ʹ����õ�������֮ǰ,������ֹ��������,
//���confirmLength<=0�����"��ȷ�ϵĳ��ȵ�����"
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
		m_buf.clear();//resizeʱ,���ܻ���ϵ�����ִ��copy����,clear��Ͳ���copy��,
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
		//(�ұߵ�)ʣ��ռ�>m_sortSizeʱ,��ֱ��ʹ��ʣ��ռ�
		if (m_posEnd + m_sortSize < m_buf.size())
		{
			dataHead = &m_buf[m_posEnd];
			dataSize = m_buf.size() - m_posEnd;
			retVal = 0;
			break;
		}
		//(�ұߵ�)ʣ��ռ�<=m_sortSizeʱ,�������ڴ�,Ȼ��ʹ��������ʣ��ռ�
		if (0 < m_posBeg)
		{
			memcpy(&m_buf[0], &m_buf[m_posBeg], m_posEnd - m_posBeg);
			m_posEnd = m_posEnd - m_posBeg;
			m_posBeg = 0;
			retVal = AvaliableSpace(dataHead, dataSize);
			break;
		}
		//�����ڴ�ʧ��,
		dataHead = &m_buf[m_posEnd];
		dataSize = m_buf.size() - m_posEnd;
		retVal = (dataSize > 0 ? 0 : -1);
	} while (false);
	return retVal;
}

//TakeData�����������޸��ڴ�(m_buf)(�����ڴ�)
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

//����ԭ��:������\r��\nʱ,���Ե�ǰ���\r��\n,��ǰ����Ҫ��END�ַ���,ENDǰ��ľ���json�ַ���,
//Ȼ��ƫ�Ƶ���һ��"�Ȳ���\rҲ����\n"�ĵط�,��Ϊ��һ��json�ַ��������,
StdStringPtr RecvBuffer_Old::ParseMessage(const char* buff, std::size_t& posBeg, uint32_t posEnd)
{
	StdStringPtr msg = nullptr;
	int32_t msgBeg = -1, msgEnd = -1;//<0,��ʾδ��ʼ��
	for (uint32_t i = posBeg; i < posEnd; ++i)
	{
		//���û���ҵ���Ϣͷ,Ҫ������Ϣͷ
		if (msgBeg < 0)
		{
			if (CrOrLf(buff[i]) == false)
				msgBeg = i;
			continue;
		}
		//forѭ��������(i<posEnd),���õ���i����posEnd������,
		//�Ѿ���msgBeg���λ����,���Բ��õ���(i-1)Խ�������,
		if (CrOrLf(buff[i]) == true && CrOrLf(buff[i - 1]) == false &&
			msgBeg <= int32_t(i - 3) && memcmp(buff + i - 3, "END", 3) == 0)
		{
			msgEnd = i - 3;//һ����Ϣ������END������
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
        m_buf.clear();//resizeʱ,���ܻ���ϵ�����ִ��copy����,clear��Ͳ���copy��,
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
    //����ʲô˼��,Ҫʹ��m_strand.wrap��װһ��handler��?
    m_socket->async_send(boost::asio::buffer(pData, dataLen), m_strand.wrap(/*��std::bind�����*/boost::bind(&TcpSocket::sendLoopHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
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
    {//(�ұߵ�)ʣ��ռ�>m_sortSizeʱ,��ֱ��ʹ��ʣ��ռ�
        dataHead = &m_bufRecv.m_buf[m_bufRecv.m_posEnd];
        dataSize = m_bufRecv.m_buf.size() - m_bufRecv.m_posEnd;
    }
    else if (0 < m_bufRecv.m_posBeg)
    {//(�ұߵ�)ʣ��ռ�<=m_sortSizeʱ,�������ڴ�,Ȼ��ʹ��������ʣ��ռ�
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