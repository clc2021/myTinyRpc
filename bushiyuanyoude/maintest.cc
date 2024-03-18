int main()
{
    EventLoop loop;
    //这个EventLoop就是main EventLoop，即负责循环事件监听处理新用户连接事件的事件循环器。
    //第一章概述篇的图2里面的EventLoop1就是我们的main EventLoop。
    
    InetAddress addr(4567);
    //InetAddress其实是对socket编程中的sockaddr_in进行封装，使其变为更友好简单的接口而已。
    
    EchoServer server(&loop, addr, "EchoServer-01");
    //EchoServer类，自己等一下往下翻一下。
    
    server.start(); 
    //启动TcpServer服务器
    
    loop.loop(); //执行EventLoop::loop()函数，这个函数在概述篇的EventLoop小节有提及，自己去看一下！！
    return 0;
}

class EchoServer
{
public:
    EchoServer(EventLoop *loop,
            const InetAddress &addr, 
            const std::string &name)
        : server_(loop, addr, name)
        , loop_(loop)
    {
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1)
        );
        // 将用户定义的连接事件处理函数注册进TcpServer中，TcpServer发生连接事件时会执行onConnection函数。
            
        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this,
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
        );
        //将用户定义的可读事件处理函数注册进TcpServer中，TcpServer发生可读事件时会执行onMessage函数。

        
        server_.setThreadNum(3);
        //设置sub reactor数量，你这里设置为3，就和概述篇图2中的EventLoop2 EventLoop3 
        //EventLoop4对应，有三个sub EventLoop。
    }
    void start(){
        server_.start();
    }
private:
    void onConnection(const TcpConnectionPtr &conn)
    {
        //用户定义的连接事件处理函数：当服务端接收到新连接建立请求，则打印Connection UP，
        //如果是关闭连接请求，则打印Connection Down
        if (conn->connected())
            LOG_INFO("Connection UP : %s", conn->peerAddress().toIpPort().c_str());
        else
            LOG_INFO("Connection DOWN : %s", conn->peerAddress().toIpPort().c_str());
    }

    void onMessage(const TcpConnectionPtr &conn,
                   Buffer *buf,
                   Timestamp time)
    {
        //用户定义的可读事件处理函数：当一个Tcp连接发生了可读事件就把它这个接收到
        //的消息原封不动的还回去
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);	
        conn->shutdown(); 
    }
    EventLoop *loop_;
    TcpServer server_;
};