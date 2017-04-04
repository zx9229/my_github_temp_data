#ifndef XX_HPP
#define XX_HPP
#include <cstdint>
#include <mutex>
#include <memory>
#include <atomic>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
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
#define peer_endpoint_is_unreachable   "peer_endpoint is unreachable."
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
        std::size_t m_posWork;//当前的位置,再往前的数据都被发送过了,都是过时的数据了.
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
        const std::size_t m_sortSize;//(右边的)剩余空间<=m_sortSize时,进行内存整理
        std::mutex  m_mutex;
        std::string m_buf;
        std::size_t m_posBeg;
        std::size_t m_posEnd;
        bool        m_isRecving;
    };
public:
    TcpSocket(boost::asio::io_service& io);
    TcpSocket(BoostSocketPtr& sock, bool isWorking);
    ~TcpSocket();
public:
    bool isWorking() const;
    bool isConnected() const;
    bool isOpen() const;
    boost::asio::ip::tcp::endpoint localPoint();
    boost::asio::ip::tcp::endpoint remotePoint();
    int connect(const std::string& ip, std::uint16_t port);
    void close();
    //callbacks.
    void onConnected();
    void onDisconnected(const boost::system::error_code& ec);
    void onError(int errId, std::string errMsg);
    void onRtnStream();
    void onRtnMessage();
private:
    void doConnected();
    void doDisconnected(const boost::system::error_code& ec);
    int  doWorkStart(const boost::asio::ip::tcp::endpoint& peerPoint);
    void doWorkStop(const boost::system::error_code& ec);
    void doStopBoostSocket(const boost::system::error_code& ec);
    void doReconnectStart(const boost::system::error_code& ec);
    void doReconnectStop();
    void doReconnectImpl(const boost::system::error_code& ec);
public:
    int send(const char* p, std::uint32_t len);
private:
    void doSendAsync(std::size_t lastLengthSent);
    void doSendAsyncHandler(const boost::system::error_code& ec, std::size_t bytes_transferred);
public:
    int startRecvAsync();
private:
    void doRecvAsync(std::size_t lastLengthReceived);//ok
    void doRecvAsyncHandler(const boost::system::error_code& error, std::size_t bytes_transferred);//ok
private:
    void cbOnConnected();
    void cbOnDisconnected(const boost::system::error_code& ec);
    void cbOnError(int errId, std::string errMsg);
public:
    void cbOnConnectedSetter(const std::function<void()>& func);
    void cbOnDisconnectedSetter(const std::function<void(const boost::system::error_code& ec)>& func);
    void cbOnErrorSetter(const std::function<void(int errId, std::string errMsg)>& func);
private:
    boost::asio::strand m_strand;
    BoostSocketPtr m_socket;//要求:类创建完成后,指针肯定不为空,socket一定是存在的.
    boost::asio::steady_timer m_timer;
    boost::asio::ip::tcp::endpoint m_peerPoint;//如果peerPoint为空,则不会断线重连.
    SendBuffer m_sendBuf;
    RecvBuffer m_recvBuf;
    std::atomic_bool m_isWorking;
    std::atomic_bool m_isConnected;
    std::atomic_bool m_isConnecting;
    std::function<void()>                                    cb_onConnected;
    std::function<void(const boost::system::error_code& ec)> cb_onDisconnected;
    std::function<void(int errId, std::string errMsg)>       cb_onError;
};

class TcpClient
{
public:
    TcpClient(int threadId);
    TcpClient(boost::asio::io_service& io);
    virtual ~TcpClient();
public:
    int connect(const std::string& ip, std::uint16_t port);
    int send(const char* p, std::uint32_t len);
    void close();
    //callbacks.
    void onConnected();
    void onDisconnected(const boost::system::error_code& ec);
    void onError(int errId, std::string errMsg);
private:
    void bindCallbacks(bool isBind);
private:
    boost::thread_group m_thgp;
    boost::asio::io_service m_io;
    boost::asio::io_service::work m_wk;
    boost::asio::io_service& m_ioRef;
    TcpSocket m_sock;
};

class TcpServer
{

public:
    int start(const std::string& addr, std::uint16_t port);
private:
    boost::asio::ip::tcp::acceptor m_acceptor;
};
#include "xx_impl.hpp"
#endif//XX_HPP
