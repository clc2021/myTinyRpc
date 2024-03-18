/*
MprpcProvider 注册服务

之前提到过，protobuf 只是提供了数据的序列化/反序列化和 RPC 接口，
但是并没有提供网络传输相关代码。而在这个项目中，我们发送和接收数据包的操作
由 muduo 库来完成。我们需要注册回调函数来处理不同的事件。
*/
#ifndef __MPRPC_PROVIDER_H__
#define __MPRPC_PROVIDER_H__

#include "google/protobuf/service.h"
#include "MprpcApplication.h"
#include <memory>
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>

struct RequestInfo
{
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    std::string args_str;
};

class MprpcProvider
{
public:
    // 框架，需要设计成抽象基类
    void NotifyService(google::protobuf::Service *service);

    // 启动rpc服务节点，开始提供rpc远程网络调用服务
    void Run();

private:
    // 新的socket连接回调
    void OnConnection(const muduo::net::TcpConnectionPtr&); /// 这是一个智能指针，代表了一个TCP连接

    // 已建立连接用户的读写事件回调
    // Buffer 类用于管理网络数据的缓冲区，它提供了一种高效的方式来处理接收到的数据或者准备发送的数据。
    void OnMessage(const muduo::net::TcpConnectionPtr&, muduo::net::Buffer*, muduo::Timestamp);

    // Closure的回调操作，用于序列化rpc的响应和网络发送
    // 是对 RPC 响应进行序列化并通过网络发送。
    void SendRpcResponse(const muduo::net::TcpConnectionPtr&, google::protobuf::Message*);

    // 将服务注册到zookeeper上
    void RegisterZookeeper(const muduo::net::InetAddress&, ZkClient*);

    // 解析数据包
    void ParseRequest(muduo::net::Buffer* buffer, RequestInfo*);

    // 服务类型信息。这是单个服务信息。
    struct ServiceInfo
    {
        // 保存服务对象
        google::protobuf::Service *m_service;
        // 保存服务方法
        // google::protobuf::MethodDescriptor用来描述rpc方法
        // 服务方法-方法描述这个描述是google::protobuf里的
        // 服务名-多个方法+描述 {方法名1-描述1*, 方法名2-描述2*, ......} 这就是m_methodMap
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor*> m_methodMap; 
    };

    std::unordered_map<std::string, ServiceInfo> m_serviceMap; // 存储注册成功的服务对象和服务方法的所有信息
    std::unique_ptr<muduo::net::TcpServer> m_tcpserverPtr; // 使用智能指针管理
    muduo::net::EventLoop m_eventLoop;
};

#endif // __MPRPC_PROVIDER_H__