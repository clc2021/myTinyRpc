/*
给客户端client.cc用的
*/
#ifndef __MPRPC_CHANNEL_H__
#define __MPRPC_CHANNEL_H__

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <muduo/base/Logging.h>
#include "MprpcApplication.h"
#include "ErrorCode.h"

#include <string.h>
#include "LoadBalancer.h"
#include <uuid/uuid.h>
#include <set>
#include "./fuse/FuseProtector.h"

// struct ServiceAddress
// {
//     std::string ip;
//     uint16_t port;
// };

class MprpcChannel : public google::protobuf::RpcChannel
{
private:
    std::string uuidString;
    int retryCount;
    FuseProtector* fuseProtector;
    
public:
    MprpcChannel() {
        uuid_t uuid;
        uuid_generate(uuid);
        char uuidStr[37];
        uuid_unparse(uuid, uuidStr);
        uuidString = std::string(uuidStr); // uuidString    
        fuseProtector = new FuseProtector(); // 熔断器检查机制
    }

    // 重写RpcChannel::CallMethod方法，统一做rpc方法的序列化和网络发送
    // 异步RPC
    void CallMethod(const google::protobuf::MethodDescriptor* method,
                          google::protobuf::RpcController* controller, 
                          const google::protobuf::Message* request,
                          google::protobuf::Message* response, 
                          google::protobuf::Closure* done);
    
    void refreshCache() {
        fuseProtector->refreshCache(); // 这个函数是对哈希表里的内容进行新的赋值
    }
    
private:
    // 按照自定义协议打包请求信息，成功返回序列化后的字符串
    RPC_CHANNEL_CODE PackageRequest(std::string* rpc_request_str,
                                    const google::protobuf::MethodDescriptor* method,
                                    google::protobuf::RpcController* controller,
                                    const google::protobuf::Message* request);
    // 通过网络发送打包好的请求信息
    RPC_CHANNEL_CODE SendRpcRquest(int* fd,
                                   const ServiceAddress& service_address, 
                                   const std::string& send_rpc_str,
                                   ::google::protobuf::RpcController* controller,
                                   const google::protobuf::MethodDescriptor* method);
    // 接收对端的响应
    RPC_CHANNEL_CODE ReceiveRpcResponse(const int& client_fd,
                                        google::protobuf::Message* response,
                                        ::google::protobuf::RpcController* controller,
                                        const google::protobuf::MethodDescriptor* method);

    bool GetServiceAddress(const std::string& service_name, // eg. UserServiceRpc
                    const std::string& method_name,  // Register
                    ServiceAddress& service_address, // 得到127.0.0.1:2181
                    ::google::protobuf::RpcController* controller);
    
    bool GetServiceAddress(const std::string& service_name, // eg. UserServiceRpc
                    const std::string& method_name,  // Register
                    std::set<ServiceAddress>& service_address, // 得到127.0.0.1:2181
                    ::google::protobuf::RpcController* controller);
};

#endif // __MPRPC_CHANNEL_H__