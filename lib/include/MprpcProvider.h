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
#include <sstream>
struct RequestInfo
{
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    std::string args_str;
};

// HTTP请求
struct HttpRequest {
    std::string method; // 请求方法 eg.POST
    std::string path; // 路径 eg. /login
    std::unordered_map<std::string, std::string> headers; // 请求头 
    std::string body; // 请求体s

    std::string service_name;
    std::string method_name;
};

// HTTP响应
struct HttpResponse {
    int statusCode;
    std::string statusMessage;
    std::string contentType;
    std::string body;

    // 构造函数
    HttpResponse(): statusCode(200), statusMessage("OK"), contentType("text/plain") {}
    HttpResponse(int code, std::string mess, std::string type, std::string b): 
    statusCode(code), statusMessage(mess), contentType(type), body(b) {}
    // 将这个响应转换成字符串
    std::string toString() const {
        std::ostringstream oss;
        oss << "HTTP/1.1" << " " << statusCode << " " << statusMessage << "\r\n";
        oss << "Content-Type: " << contentType << "\r\n";
        oss << "Content-Length: " << body.size() << "\r\n";
        oss << "\r\n";
        oss << body;
        return oss.str();
    }
};

class MprpcProvider
{
public:
    void NotifyService(google::protobuf::Service *service);

    void Run();

    void RunHttp();

private:
    void RegisterZookeeper(const muduo::net::InetAddress&, ZkClient*);

    void OnConnection(const muduo::net::TcpConnectionPtr&); /// 这是一个智能指针，代表了一个TCP连接

    void ParseRequest(muduo::net::Buffer* buffer, RequestInfo*);

    void ParseHttpRequest(muduo::net::Buffer* buffer, HttpRequest& http_request);

    void OnMessage(const muduo::net::TcpConnectionPtr&, muduo::net::Buffer*, muduo::Timestamp);

    void OnHttpMessage(const muduo::net::TcpConnectionPtr&, muduo::net::Buffer*, muduo::Timestamp);

    void SendRpcResponse(const muduo::net::TcpConnectionPtr&, google::protobuf::Message*);

    //void SendHttpResponse(const muduo::net::TcpConnectionPtr&, const HttpResponse& response);
    void SendHttpResponse(const muduo::net::TcpConnectionPtr&, google::protobuf::Message* response);

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