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
        std::size_t m_posWork;//��ǰ��λ��,����ǰ�����ݶ��Ǳ�ʹ�ù�����,���ǹ�ʱ��������.
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
        const std::size_t m_sortSize;//(�ұߵ�)ʣ��ռ�<=m_sortSizeʱ,�����ڴ�����
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
    bool isOpen()const;
    void close();
    boost::asio::ip::tcp::endpoint localPoint();
    boost::asio::ip::tcp::endpoint remotePoint();
    int connect(const std::string& ip, std::uint16_t port);
    void send(const char* p, std::uint32_t len);
    void startRecvAsync();
    void onRtnStream() {}
    void onRtnMessage() {}
    void onError(int errId, const std::string& errMsg) {}
private:
    void doReconnect(const boost::system::error_code& ec);
    void sendLoop(std::size_t lastSendLength);
    void sendLoopHandler(const boost::system::error_code& ec, std::size_t bytes_transferred);
    void recvAsync();
    void recvHandler(const boost::system::error_code& error, std::size_t bytes_transferred);
private:
    boost::asio::strand m_strand;
    BoostSocketPtr m_socket;//Ҫ��:�ഴ����ɺ�,ָ��϶���Ϊ��,socketһ���Ǵ��ڵ�.
    boost::asio::steady_timer m_timer;
    boost::asio::ip::tcp::endpoint m_ep;
    SendBuffer m_bufSend;
    RecvBuffer m_bufRecv;
};
#include "xx_impl.hpp"
#endif//XX_HPP
