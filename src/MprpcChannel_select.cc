#include "MprpcChannel.h"
#include "MprpcController.h"
#include "rpc_header.pb.h"

#include <muduo/net/TcpClient.h>

#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <set>
#include <netinet/tcp.h>

#include <fcntl.h>
#include <sys/select.h>
// 读取配置文件rpcserver的信息
// std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
// uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
// rpc调用方想调用service_name的method_name服务，需要查询zk上该服务所在的host信息

bool MprpcChannel::GetServiceAddress(const std::string& service_name, // eg. UserServiceRpc
                    const std::string& method_name,  // Register
                    std::set<ServiceAddress>& service_address, // 得到127.0.0.1:2181
                    ::google::protobuf::RpcController* controller) 
                    {
    ZkClient zkCli;
    zkCli.Start();
    std::string method_path = "/" + service_name + "/" + method_name; // /UserService/Login
    std::vector<std::string> host_datas = zkCli.GetChildData(method_path.c_str());
    std::cout <<"GetChildData: ";

    for (int i = 0; i < host_datas.size(); i++) 
        std::cout << host_datas[i] << "\t";
    std::cout << std::endl;

    for (int i = 0; i < host_datas.size(); i++) {
        std::string host_data = zkCli.GetData(host_datas[i].c_str()); // /UserService/Login/Node455
        std::cout << "host_data = " << host_data << std::endl; // 127.0.0.1:3002
        if (host_data == "") {
            controller->SetFailed(host_data + "is not exist!");
            return false;
        }
        int idx = host_data.find(":");
        if (idx == -1) {
            controller->SetFailed(method_path + " address is invalid!");
            return false;
        }
        
        ServiceAddress temp;
        temp.ip = host_data.substr(0, idx); // 127.0.0.1
        temp.port = atoi(host_data.substr(idx+1, host_data.size()-idx).c_str()); // 2181
        service_address.insert(temp);
    }
    return true;
}


// 根据服务名字，查到zookeeper上的对应的ip地址，进行服务发现 
bool MprpcChannel::GetServiceAddress(const std::string& service_name, // eg. UserServiceRpc
                    const std::string& method_name,  // Register
                    ServiceAddress& service_address, // 得到127.0.0.1:4545
                    ::google::protobuf::RpcController* controller) 
                    {
    ZkClient zkCli;
    zkCli.Start();
    std::string method_path = "/" + service_name + "/" + method_name; // /UserService/Login
    std::string host_data = zkCli.GetData(method_path.c_str()); // 根据字符串值可以得到ip:port
    // std::vector<std::string> host_datas = zkCli.GetChildData(method_path.c_str());
    if (host_data == "")
    {
        controller->SetFailed(method_path + " is not exist!");
        return false;
    }
    int idx = host_data.find(":");
    if (idx == -1)
    {
        controller->SetFailed(method_path + " address is invalid!");
        return false;
    }

    service_address.ip = host_data.substr(0, idx); // 127.0.0.1
    service_address.port = atoi(host_data.substr(idx+1, host_data.size()-idx).c_str()); // 2181
    return true;
}

// 总体来说，这段代码的作用是将 RPC 请求的头部信息和参数序列化后，按照一定格式拼接成一个完整的
//  RPC 请求字符串，并存储到指定的字符串对象中。
RPC_CHANNEL_CODE MprpcChannel::PackageRequest(std::string* rpc_request_str,
                                              const google::protobuf::MethodDescriptor* method,
                                              google::protobuf::RpcController* controller,
                                              const google::protobuf::Message* request)
{
    std::string service_name = method->service()->name();
    std::string method_name = method->name();

    // request里面储存着请求参数
    // 我们需要将请求参数序列化
    uint32_t args_size = 0;
    std::string args_str;
    if (request->SerializeToString(&args_str)) {
        // uint32_t args_size = 0;
        args_size = args_str.size();
    } else {
        controller->SetFailed("serialize request error!");
        return CHANNEL_PACKAGE_ERR;
    }

    // 定义rpc的请求header
    mprpc::RpcHeader rpc_header;
    rpc_header.set_service_name(service_name);
    rpc_header.set_methon_name(method_name);
    rpc_header.set_args_size(args_size);

    // 获取请求header的字符串长度
    uint32_t header_size = 0;
    std::string rpc_header_str;
    if (rpc_header.SerializeToString(&rpc_header_str)) {
        header_size = rpc_header_str.size();
    } else {
        controller->SetFailed("serialize rpc header error!");
        return CHANNEL_PACKAGE_ERR;
    }

    // memset(rpc_request_str, '\0', sizeof(rpc_request_str));
    (*rpc_request_str).insert(0, std::string((char*)&header_size, 4));
    (*rpc_request_str) += rpc_header_str;
    (*rpc_request_str) += args_str;

    return CHANNEL_SUCCESS;
}

RPC_CHANNEL_CODE MprpcChannel::SendRpcRquest(int* fd,
                               const ServiceAddress& service_address,
                               const std::string& send_rpc_str,
                               ::google::protobuf::RpcController* controller) 
{
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == client_fd) {
        char errtxt[512] = {0};
        sprintf(errtxt, "创建套接字失败, errno: %d", errno);
        controller->SetFailed(errtxt);
        return CHANNEL_SEND_ERR;
    }

    // 设置套接字为非阻塞模式
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(service_address.port);
    server_addr.sin_addr.s_addr = inet_addr(service_address.ip.c_str());
    LOG_INFO << "客户端连接connect: "<< service_address.ip << ":" << service_address.port;

    // 连接rpc服务节点
    int connect_res = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (connect_res < 0 && errno != EINPROGRESS) {
        close(client_fd);
        char errtxt[512] = {0};
        sprintf(errtxt, "连接RPC服务端错误! errno:%d", errno);
        controller->SetFailed(errtxt);
        return CHANNEL_SEND_ERR;
    }

    // 设置超时时间
    struct timeval timeout;
    timeout.tv_sec = 5; // 5s
    timeout.tv_usec = 0;
    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(client_fd, &write_fds);
    int select_res = select(client_fd + 1, nullptr, &write_fds, nullptr, &timeout);
    if (select_res <= 0) {
        close(client_fd);
        char errtxt[512] = {0};
        sprintf(errtxt, "连接超时! errno:%d", errno);
        controller->SetFailed(errtxt);
        return CHANNEL_SEND_ERR;
    }

    // 发送rpc请求
    std::cout << "客户端发送" << send_rpc_str << std::endl;
    int send_result = send(client_fd, send_rpc_str.c_str(), send_rpc_str.size(), 0);
    if (send_result < 0) {
        close(client_fd);
        char errtxt[512] = {0};
        sprintf(errtxt, "发送错误! errno:%d", errno);
        controller->SetFailed(errtxt);
        return CHANNEL_SEND_ERR;
    }
    (*fd) = client_fd;
    return CHANNEL_SUCCESS; // 我不明白，这里发送为什么引入select
}

RPC_CHANNEL_CODE MprpcChannel::ReceiveRpcResponse(const int& client_fd,
                               google::protobuf::Message* response,
                               ::google::protobuf::RpcController* controller) 
{
    // 设置套接字为非阻塞
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

    // 接收缓冲区响应
    char recv_buf[1024] = {0};
    int recv_size = 0;
    int total_recv_size = 0;
    bool received_response = false;

    // 循环接收，直到接收完整的响应或超时
    while (!received_response) {
        recv_size = recv(client_fd, recv_buf + total_recv_size, sizeof(recv_buf) - total_recv_size, 0);
        if (recv_size < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) { // 非阻塞模式下, 暂时无数据可读
                // 可以选择继续等待或直接返回超时错误, 这里选后者
                close(client_fd);
                char errtxt[512] = {0};
                sprintf(errtxt, "接收超时! errno:%d", errno);
                controller->SetFailed(errtxt);
                return CHANNEL_RECEIVE_ERR;
            } else {
                // 其他错误
                close(client_fd);
                char errtxt[512] = {0};
                sprintf(errtxt, "接收错误! errno:%d", errno);
                controller->SetFailed(errtxt);
                return CHANNEL_RECEIVE_ERR;
            }
        } else if (recv_size == 0) {
            // 对端关闭连接
            close(client_fd);
            char errtxt[512] = {0};
            sprintf(errtxt, "连接被对端关闭");
            controller->SetFailed(errtxt);
            return CHANNEL_RECEIVE_ERR;
        } else {
            total_recv_size += recv_size;
            // 检查是否收到完整的响应
            // 这里可以根据具体的协议来判断，比如判断是否收到了响应的结束标志
            // 简单起见，这里假设收到的数据长度大于某个固定的值即认为收到了完整的响应
            if (total_recv_size >= 1024) {
                received_response = true;
            }
        }
    }

    std::string response_str(recv_buf, 0, recv_size);
    // 序列化响应信息
    if (!response->ParseFromArray(recv_buf, recv_size)) // TODO:使用Array而不是String
    {
        close(client_fd);
        char errtxt[512] = {0};
        sprintf(errtxt, "序列化错误! response_str:%s", recv_buf);
        controller->SetFailed(errtxt);
        return CHANNEL_RECEIVE_ERR;
    }

    close(client_fd);    
    return CHANNEL_SUCCESS;
}

/*
// 通过网络发送打包好的请求信息
RPC_CHANNEL_CODE MprpcChannel::SendRpcRquest(int* fd,
                               const ServiceAddress& service_address, 
                               const std::string& send_rpc_str,
                               ::google::protobuf::RpcController* controller)
{
    // 使用tcp编程，完成rpc方法的远程调用
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == client_fd)
    {
        char errtxt[512] = {0};
        sprintf(errtxt, "create socket error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return CHANNEL_SEND_ERR;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(service_address.port);
    server_addr.sin_addr.s_addr = inet_addr(service_address.ip.c_str());
    LOG_INFO << "客户端连接connect: "<< service_address.ip << ":" << service_address.port;

    struct timeval timeout;
    timeout.tv_sec = 5; // 5s
    timeout.tv_usec = 0;
    if (setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        close(client_fd);
        char errtxt[512] = {0};
        sprintf(errtxt, "超时! errno:%d", errno);
        controller->SetFailed(errtxt);
        return CHANNEL_SEND_ERR;
    }

    // 连接rpc服务节点：这里连接的是RPC服务器ip:zookeeper的port
    if (-1 == connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)))
    {
        close(client_fd);
        char errtxt[512] = {0};
        sprintf(errtxt, "connect error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return CHANNEL_SEND_ERR;
    }

    // 发送rpc请求
    std::cout << "客户端发送" << send_rpc_str << std::endl;
    if (-1 == send(client_fd, send_rpc_str.c_str(), send_rpc_str.size(), 0))
    {
        close(client_fd);
        char errtxt[512] = {0};
        sprintf(errtxt, "send error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return CHANNEL_SEND_ERR;
    }
    (*fd) = client_fd;
    return CHANNEL_SUCCESS;
}

RPC_CHANNEL_CODE MprpcChannel::ReceiveRpcResponse(const int& client_fd,
                        google::protobuf::Message* response,
                        ::google::protobuf::RpcController* controller)
{
    // 接收响应缓冲区
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if (-1 == (recv_size = recv(client_fd, recv_buf, 1024, 0)))
    {
        close(client_fd);
        char errtxt[512] = {0};
        sprintf(errtxt, "recv error! errno:%d", errno);
        controller->SetFailed(errtxt);
        return CHANNEL_RECEIVE_ERR;
    }

    std::string response_str(recv_buf, 0, recv_size);
    // 序列化响应信息
    if (!response->ParseFromArray(recv_buf, recv_size)) // TODO:使用Array而不是String
    {
        close(client_fd);
        char errtxt[512] = {0};
        sprintf(errtxt, "parse error! response_str:%s", recv_buf);
        controller->SetFailed(errtxt);
        return CHANNEL_RECEIVE_ERR;
    }

    close(client_fd);    
    return CHANNEL_SUCCESS;
}
*/




// 重写RpcChannel::CallMethod方法，统一做rpc方法的序列化和网络发送
// header_size service_name method_name args_size args
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                          google::protobuf::RpcController* controller, 
                          const google::protobuf::Message* request,
                          google::protobuf::Message* response, 
                          google::protobuf::Closure* done)
{
    // 按照自定义协议封装RPC请求
    std::string rpc_request_str;
    std::string* ptr = &rpc_request_str;
    RPC_CHANNEL_CODE res =  PackageRequest(ptr, method, controller, request);
    if (res) {
        LOG_ERROR << "PackageRequest failed()";
        done->Run();
        return;
    }

    // 获取微服务地址
    std::set<ServiceAddress> service_address_set; // 服务地址集合

    if (false == GetServiceAddress(method->service()->name(), method->name(), service_address_set, controller))
    {
        LOG_ERROR << "GetServiceAddress failed()";
        done->Run();
        return ;
    }
    for (auto it = service_address_set.begin(); it != service_address_set.end(); it++)
        std::cout << it->port << "\t";
    std::cout << std::endl;
    std::string loadbalancer = MprpcApplication::GetInstance().GetConfig().Load("loadbalancer"); 
    LoadBalancer* lB = nullptr;
    if (loadbalancer == "RoundRobinLoadBalancer") {
        std::cout << "负载均衡策略为RR" << std::endl; 
        lB = new RoundRobinLoadBalancer();
    } else {
        std::cout << "负载均衡策略为C" << std::endl; 
        lB = new ConsistentHashLoadBalancer(uuidString);
    }

    ServiceAddressRes serviceAddressRes = lB->select(service_address_set);
    ServiceAddress curAddress = serviceAddressRes.getCurServiceAddress();
    std::set<ServiceAddress> othersAddress = serviceAddressRes.getOtherServiceAddress();
    std::cout << "负载均衡策略选择: " << curAddress.port << std::endl;
    
    // 以下为重试机制+容错机制
    std::string retryCountStr = MprpcApplication::GetInstance().GetConfig().Load("retrycount");
    std::string faulTolerant = MprpcApplication::GetInstance().GetConfig().Load("faulttolerant"); 
    std::cout << "faulTolerant = " << faulTolerant << std::endl;
    retryCount = std::stoi(retryCountStr);
    int count = 1;
    while (count <= retryCount) {
        // 通过网络发送RPC请求，返回clientfd，我们将用此接收响应
        int client_fd;
        RPC_CHANNEL_CODE res1 = SendRpcRquest(&client_fd, curAddress, rpc_request_str, controller);
        RPC_CHANNEL_CODE res2 = ReceiveRpcResponse(client_fd, response, controller);
        if (res1 || res2) {
            std::cout << "接收响应错误: " << std::endl;
            std::cout << "现在选择相应的容错策略: " << std::endl;
            if (faulTolerant == "FailOver") {
                count++;
                std::cout << "当前cur" << curAddress.port << std::endl << "others:  ";
                for (auto it =  othersAddress.begin(); it != othersAddress.end(); it++)
                    std::cout << (*it).port << "\t";
                std::cout << "\n故障转移 这是第" << count << "次重试" << std::endl;
                if (!othersAddress.empty()) { // 这是一个set
                    auto next = othersAddress.begin();
                    curAddress = *next;
                    othersAddress.erase(next);
                }
            } else {
                LOG_ERROR << "ReceiveRpcResponse failed()";
                done->Run();
                return ;
            }
            // return;
        } else {
            std::cout << "接收响应成功" << std::endl;
            std::cout << "CallMethod()里, done调用开始..." << std::endl;
            done->Run();
            std::cout << "CallMethod()里, done调用结束..." << std::endl;
            return;
        }
    }
}