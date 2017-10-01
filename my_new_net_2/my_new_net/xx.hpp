#ifndef XX_HPP
#define XX_HPP

#include <cassert>
#include <string>
#include <set>
#include <memory>
#include <mutex>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/thread.hpp>


namespace MyNet
{
    //////////////////////////////////////////////////////////////////////////
    // IoWithMTh
    //////////////////////////////////////////////////////////////////////////

    class IoWithMTh
    {
    public:
        IoWithMTh() :m_wk(m_io) {}
        virtual ~IoWithMTh() { terminate(); }
        boost::asio::io_service& ioService() { return m_io; }
        void initialize(int threadNum);
        void terminate();
    private:
        boost::asio::io_service m_io;
        boost::asio::io_service::work m_wk;
        boost::thread_group m_thgp;
    };
    using IoWithMThPtr = std::shared_ptr<IoWithMTh>;
};


namespace MyNet
{
    //////////////////////////////////////////////////////////////////////////
    // TcpServer
    //////////////////////////////////////////////////////////////////////////

    class TcpSocket;
    using TcpSocketPtr = std::shared_ptr<TcpSocket>;
    using StdStringPtr = std::shared_ptr<std::string>;
    using BoostStrandPtr = std::shared_ptr<boost::asio::io_service::strand>;
    using BoostSocketPtr = std::unique_ptr<boost::asio::ip::tcp::socket>;

    class TcpServer
    {
    private:
        friend class TcpSocket;
    private:
        typedef std::unique_ptr<boost::asio::ip::tcp::acceptor>  BoostAcceptorPtr;

    public:
        TcpServer(boost::asio::io_service& netIo);
        TcpServer(boost::asio::io_service& netIo, boost::asio::io_service& appIo);
        TcpServer(int netThNum, int appThNum);

    public:
        int open(const std::string& ip, std::uint16_t port, std::string& eMessage);
        int open(const std::string& ip, std::uint16_t port);
        int close(std::string& eMessage);
        int close();
        //
        virtual void onConnected(TcpSocketPtr tcpSock)/* {}*/;
        virtual void onDisconnected(TcpSocketPtr tcpSock, const boost::system::error_code& ec)/* {}*/;
        virtual void onReceivedData(TcpSocketPtr tcpSock, StdStringPtr data)/* {}*/;
        virtual void onError(TcpSocketPtr tcpSock, const boost::system::error_code& ec)/* {}*/;
        virtual void onError(TcpSocketPtr tcpSock, int value, const std::string& message)/* {}*/;

    private:
        void doAcceptAsync();
        void doAcceptAsyncHandler(const boost::system::error_code& ec, boost::asio::ip::tcp::socket* rawSock);
        void addSocket(TcpSocketPtr sock);
        void delSocket(TcpSocketPtr sock);

    private:
        std::recursive_mutex m_mtx;
        bool                 m_isWorking;
        IoWithMThPtr   m_netMThIo;
        IoWithMThPtr   m_appMThIo;
        BoostStrandPtr m_netStrand;
        BoostStrandPtr m_appStrand;
        BoostAcceptorPtr m_acceptor;
        boost::asio::ip::tcp::endpoint m_localEndpoint;
        std::set<TcpSocketPtr> m_set;
    };
};


namespace MyNet
{
    //////////////////////////////////////////////////////////////////////////
    // 作为一个socket,应当是全双工的(发送的同时也在接收着数据).
    // 不应当半双工(一个大锁同时锁住了send和recv,send和recv只能同时只能工作一个).
    //////////////////////////////////////////////////////////////////////////

    class TcpSocket :public std::enable_shared_from_this<TcpSocket>
    {
    private:
        class SendBuffer
        {
        public:
            SendBuffer() :m_capacity(1024 * 512), m_isSending(false) { reset(); }
            void reset();
        public:
            std::size_t const m_capacity;
            std::string m_bufWait;
            std::string m_bufWork;
            std::size_t m_posWork;//当前的位置,再往前的数据都被发送过了,都是过时的数据了.
            bool        m_isSending;
        };

    public:
        TcpSocket(boost::asio::io_service& netIo);
        TcpSocket(boost::asio::io_service& netIo, boost::asio::io_service& appIo);
        TcpSocket(BoostSocketPtr& sock, TcpServer* server);//TODO:

    public:
        bool isAccepted() const { return (nullptr != m_server); }
        bool isWorking() const { return m_isWorking; }
        bool isConnected() const { return m_isConnected; }
        bool isOpen() const { return m_socket->is_open(); }
        const boost::asio::ip::tcp::endpoint& bindEndpoint() { return m_server ? m_server->m_localEndpoint : m_defaultEndpoint; }
        const boost::asio::ip::tcp::endpoint& connectEndpoint() { return m_peerEndpoint; }
        const boost::asio::ip::tcp::endpoint& localEndpoint() { return m_localEndpoint; }
        const boost::asio::ip::tcp::endpoint& remoteEndpoint() { return m_remoteEndpoint; }
        const boost::asio::ip::tcp::endpoint& localEndpointEx() { return m_server ? bindEndpoint() : localEndpoint(); }
        const boost::asio::ip::tcp::endpoint& remoteEndpointEx() { return m_server ? remoteEndpoint() : connectEndpoint(); }
        //
        int  connect(const std::string& ip, std::uint16_t port, bool asyncConnect, std::string& eMessage);
        int  connect(const std::string& ip, std::uint16_t port, bool asyncConnect = true);
        void close();
        int  sendAsync(const char* data, std::uint32_t size, std::string& eMeseage);
        int  sendAsync(const char* data, std::uint32_t size);
        int  sendSynch(const char* data, std::uint32_t size, int&bytesTransferred, std::string& eMeseage);
        //
        virtual void onConnected(TcpSocketPtr tcpSock)/* {}*/;
        virtual void onDisconnected(TcpSocketPtr tcpSock, const boost::system::error_code& ec)/* {}*/;
        virtual void onReceivedData(TcpSocketPtr tcpSock, StdStringPtr data)/* {}*/;
        virtual void onError(TcpSocketPtr tcpSock, int value, const std::string& message)/* {}*/;
        virtual void onError(TcpSocketPtr tcpSock, const boost::system::error_code& ec)/* {}*/;

    private:
        void doJustCloseBoostSocket();
        void doConnectAsync(const boost::system::error_code& ec);
        void doConnectAsyncHandler(const boost::system::error_code& ec);
        void doRecvAsync();
        void doRecvAsyncHandler(const boost::system::error_code& ec, std::size_t bytes_transferred);
        void doSendAsync(std::size_t lastLengthSent);
        void doSendAsyncHandler(const boost::system::error_code& ec, std::size_t bytes_transferred);
        //
        void doOnConnected(TcpSocketPtr tcpSock);
        void doOnDisconnected(TcpSocketPtr tcpSock, const boost::system::error_code& ec);
        void doOnReceivedData(TcpSocketPtr tcpSock, StdStringPtr data);
        void doOnError(TcpSocketPtr tcpSock, const boost::system::error_code& ec);
        void doOnError(TcpSocketPtr tcpSock, int value, const std::string& message);
    private:
        std::recursive_mutex            m_mtx;
        boost::asio::ip::tcp::endpoint  m_peerEndpoint;
        TcpServer* const                m_server;
        bool                            m_isWorking;
        bool                            m_isConnected;
        boost::asio::steady_timer       m_timer;
        BoostStrandPtr                  m_netStrand;
        BoostStrandPtr                  m_appStrand;
        BoostSocketPtr                  m_socket;
        SendBuffer                      m_sendBuf;
        StdStringPtr                    m_recvBuf;

        boost::asio::ip::tcp::endpoint m_localEndpoint;
        boost::asio::ip::tcp::endpoint m_remoteEndpoint;
        boost::asio::ip::tcp::endpoint m_defaultEndpoint;
    };
};

namespace MyNet
{
    class TcpClient;

    class TcpSocketTemp :public TcpSocket
    {
    public:
        TcpSocketTemp(boost::asio::io_service& netIo) :TcpSocket(netIo) {}
        TcpSocketTemp(boost::asio::io_service& netIo, boost::asio::io_service& appIo) :TcpSocket(netIo, appIo) {}
    public:
        void setPointer(TcpClient* pointer) { m_client = pointer; }
    private:
        virtual void onConnected(TcpSocketPtr tcpSock) override;
        virtual void onDisconnected(TcpSocketPtr tcpSock, const boost::system::error_code& ec) override;
        virtual void onReceivedData(TcpSocketPtr tcpSock, StdStringPtr data) override;
        virtual void onError(TcpSocketPtr tcpSock, int value, const std::string& message) override;
        virtual void onError(TcpSocketPtr tcpSock, const boost::system::error_code& ec) override;
    private:
        TcpClient* m_client;
    };

    class TcpClient
    {
    public:
        TcpClient(boost::asio::io_service& netIo);
        TcpClient(boost::asio::io_service& netIo, boost::asio::io_service& appIo);
        TcpClient(int netThNum, int appThNum);
    public:
        bool isAccepted() const;
        bool isWorking() const;
        bool isConnected() const;
        bool isOpen() const;
        //
        const boost::asio::ip::tcp::endpoint& bindEndpoint();
        const boost::asio::ip::tcp::endpoint& connectEndpoint();
        const boost::asio::ip::tcp::endpoint& localEndpoint();
        const boost::asio::ip::tcp::endpoint& remoteEndpoint();
        const boost::asio::ip::tcp::endpoint& localEndpointEx();
        const boost::asio::ip::tcp::endpoint& remoteEndpointEx();
        //
        int  connect(const std::string& ip, std::uint16_t port, bool asyncConnect, std::string& eMessage);
        int  connect(const std::string& ip, std::uint16_t port, bool asyncConnect = true);
        void close();
        int  sendAsync(const char* data, std::uint32_t size, std::string& eMeseage);
        int  sendAsync(const char* data, std::uint32_t size);
        int  sendSynch(const char* data, std::uint32_t size, int&bytesTransferred, std::string& eMeseage);
    public:
        virtual void onConnected(TcpSocketPtr tcpSock) {};
        virtual void onDisconnected(TcpSocketPtr tcpSock, const boost::system::error_code& ec) {};
        virtual void onReceivedData(TcpSocketPtr tcpSock, StdStringPtr data) {};
        virtual void onError(TcpSocketPtr tcpSock, int value, const std::string& message) {};
        virtual void onError(TcpSocketPtr tcpSock, const boost::system::error_code& ec) {};
    private:
        IoWithMThPtr   m_netMThIo;
        IoWithMThPtr   m_appMThIo;
        std::shared_ptr<TcpSocketTemp> m_sock;
    };
};


#include "xx_impl.hpp"
#endif//XX_HPP
