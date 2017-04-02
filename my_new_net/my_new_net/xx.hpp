#ifndef XX_HPP
#define XX_HPP
#include <cstdint>
#include <mutex>
#include <memory>
#include <atomic>
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
#define TcpSocket_is_already_working   "TcpSocket is already working."
#define TcpSocket_is_no_longer_working "TcpSocket is no longer working."
#define TcpSocket_has_been_connected   "TcpSocket has been connected."
#define TcpSocket_is_not_connected     "TcpSocket is not connected."
#define TcpSocket_is_already_receiving "TcpSocket is already receiving."
#define Invalid_peer_endpoint          "Invalid peer_endpoint."
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
        std::size_t m_posWork;//��ǰ��λ��,����ǰ�����ݶ������͹���,���ǹ�ʱ��������.
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
    bool isWorking() const;
    bool isConnected() const;
    bool isOpen() const;
    boost::asio::ip::tcp::endpoint localPoint();
    boost::asio::ip::tcp::endpoint remotePoint();
    int connect(const std::string& ip, std::uint16_t port);
    void close();
    int send(const char* p, std::uint32_t len);
    int startRecvAsync();//ok
    void onRtnStream() {}
    void onRtnMessage() {}
    void onError(int errId, std::string errMsg) {}
    void onConnected() {}
    void onDisconnected(const boost::system::error_code& ec) {}
private:
    void doConnected();
    void doDisconnected(const boost::system::error_code& ec);
    void doStopWork(const boost::system::error_code& ec);
    void doStopConnection(const boost::system::error_code& ec);
    void doReconnectEntry(const boost::system::error_code& ec);
    void doReconnect(const boost::system::error_code& ec);
    void doSendAsync(std::size_t lastLengthSent);
    void doSendAsyncHandler(const boost::system::error_code& ec, std::size_t bytes_transferred);
    void doRecvAsync(std::size_t lastLengthReceived);//ok
    void doRecvAsyncHandler(const boost::system::error_code& error, std::size_t bytes_transferred);//ok
private:
    boost::asio::strand m_strand;
    BoostSocketPtr m_socket;//Ҫ��:�ഴ����ɺ�,ָ��϶���Ϊ��,socketһ���Ǵ��ڵ�.
    boost::asio::steady_timer m_timer;
    boost::asio::ip::tcp::endpoint m_peerPoint;//���peerPointΪ��,�򲻻��������.
    SendBuffer m_sendBuf;
    RecvBuffer m_recvBuf;
    std::atomic_bool m_isWorking;
    std::atomic_bool m_isConnecting;
    std::atomic_bool m_isConnected;
};
#include "xx_impl.hpp"
#endif//XX_HPP
