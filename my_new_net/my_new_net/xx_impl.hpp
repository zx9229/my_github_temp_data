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


//�а�ȫ����:ָ����ⲿ��,����ⲿû�м�ʱʹ�����ݵĻ�,���ݿ��ܱ��ڲ��Ķ�,����ⲿǿ���޸����ݵĻ�,���ݻ�������
//����,���ô˺�����,��û��ʹ����õ�������֮ǰ,������ֹ��������,
//���confirmLength<=0�����"��ȷ�ϵĳ��ȵ�����"
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
		m_buf.clear();//resizeʱ,���ܻ���ϵ�����ִ��copy����,clear��Ͳ���copy��,
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

//����ԭ��:������\r��\nʱ,���Ե�ǰ���\r��\n,��ǰ����Ҫ��END�ַ���,ENDǰ��ľ���json�ַ���,
//Ȼ��ƫ�Ƶ���һ��"�Ȳ���\rҲ����\n"�ĵط�,��Ϊ��һ��json�ַ��������,
StdStringPtr RecvBuffer::ParseMessage(const char* buff, uint32_t& posBeg, uint32_t posEnd)
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

#endif//xx_impl_H