#ifndef xx_impl_H
#define xx_impl_H
#include "xx.hpp"
#if 1 //SendBuffer
SendBuffer::SendBuffer() :m_capacity(1024 * 64)
{
	Reset();
}

SendBuffer::~SendBuffer() {}

void SendBuffer::Reset()
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

void SendBuffer::FeedData(const char* data, uint32_t len)
{
	std::lock_guard<std::mutex> lg(m_mutex);
	m_bufWait.append(data, len);
    return m_bufWait.size() + m_bufWork.size() - m_posWork;
}


//有安全隐患:指针给外部了,如果外部没有及时使用数据的话,数据可能被内部改动,如果外部强制修改数据的话,数据会有问题
//所以,调用此函数后,在没有使用完得到的数据之前,这个类禁止其他操作,
//如果confirmLength<=0则忽略"待确认的长度的数据"
void SendBuffer::TakeData(const char*&data, uint32_t& len, int confirmLength = -1)
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
#if 1 //RecvBuffer
RecvBuffer::RecvBuffer() :m_capacity(1024 * 512), m_sortSize(1024 * 4)
{
	Reset();
}

RecvBuffer::~RecvBuffer() {}

void RecvBuffer::Reset()
{
	if (m_buf.size() < m_capacity)
	{
		m_buf.clear();//resize时,可能会对老的数据执行copy操作,clear后就不会copy了,
		m_buf.resize(m_capacity);
	}
	m_posBeg = 0;
	m_posEnd = 0;
}

int RecvBuffer::AvaliableSpace(char*& dataHead, uint32_t& dataSize)
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
void RecvBuffer::FeedSize(uint32_t len)
{
	assert(m_posEnd + len <= m_buf.size());
	m_posEnd += len;
}

StdStringPtr RecvBuffer::NextMessage()
{
	StdStringPtr msg = RecvBuffer::ParseMessage(m_buf.c_str(), m_posBeg, m_posEnd);
	assert(m_posBeg <= m_posEnd);
	return msg;
}

bool RecvBuffer::CrOrLf(char c)
{
	return ('\r' == c || '\n' == c) ? true : false;
}

//解析原则:当看到\r或\n时,忽略掉前面的\r和\n,最前面需要是END字符串,END前面的就是json字符串,
//然后偏移到第一个"既不是\r也不是\n"的地方,作为下一个json字符串的起点,
StdStringPtr RecvBuffer::ParseMessage(const char* buff, uint32_t& posBeg, uint32_t posEnd)
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

#endif//xx_impl_H