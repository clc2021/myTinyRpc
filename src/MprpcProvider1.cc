#include "MprpcProvider.h"
#include "rpc_header.pb.h"
#include "google/protobuf/descriptor.h"

#include <functional>

// 框架，需要设计成抽象基类
// 注册服务，所以参数是服务类型的指针
// 执行结果：对于传入的service，得到service名-Service的Info信息{service对象，多个方法+方法名}
// 注意：这个注册结果是针对一个service，但是实际上的serviceMap应该非常庞大
// serviceMap = { 
// service1名 -> ServiceInfo结构体【service1对象, {rpc1名 -> 方法1描述*, rpc2名 -> 方法2描述*...}】, 
// service2名 -> ServiceInfo结构体【service2对象, {rpc1名 -> 方法1描述*, rpc2名 -> 方法2描述*...}】,  
// service3名 -> ServiceInfo结构体【service3对象, {rpc1名 -> 描述*, rpc2名 -> 描述*....}】
// ...}

void MprpcProvider::NotifyService(google::protobuf::Service *service) 
{
    // ServiceInfo：包括服务对象和方法名-方法描述，一个对象可能跟多个这样的方法名+描述在一起
    ServiceInfo service_info;

    // 获取服务对象的描述信息
    // 你看，protobuf::Service还有配套的protobuf::ServiceDescriptor
    const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();
    
    // 获取服务的名字
    // 需包含 google/protobuf/descriptor.h 
    // ServiceDescriptor 中包含了关于服务的各种元信息，而 Service 则是实际的服务对象，可以用于执行具体的 RPC 调用。
    std::string service_name = pserviceDesc->name();

    std::cout << "=========================================" << std::endl;
    std::cout << "service_name:" << service_name << std::endl; // UserServiceRpc

    for (int i = 0; i < pserviceDesc->method_count(); ++i)
    {
        // 获取了服务对象指定下标的服务方法的描述「方法名，方法」
        const google::protobuf::MethodDescriptor* pmethodDesc = pserviceDesc->method(i);
        std::string method_name = pmethodDesc->name();
        service_info.m_methodMap.insert({method_name, pmethodDesc});
        std::cout << "method_name:" << method_name << std::endl; // login; Register
    }
    std::cout << "=========================================" << std::endl;
    service_info.m_service = service;
    m_serviceMap.insert({service_name, service_info});
}

// 将服务注册到zookeeper上
void MprpcProvider::RegisterZookeeper(const muduo::net::InetAddress& address, ZkClient* zkCli)
{
    // 把当前rpc节点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务
    zkCli->Start();
    // service_name为永久性节点，method_name为临时性节点
    for (auto &sp : m_serviceMap) 
    {
        std::string service_path = "/" + sp.first; // sp.first就是服务名字
        // std::cout << "service_path/ = " << sp.first << std::endl;
        //              service_path/ = /UserServiceRpc
        zkCli->Create(service_path.c_str(), nullptr, 0);  // 创建了服务的永久性节点
        std::cout << "永久性服务节点 " << service_path.c_str() << std::endl;
        //               永久性服务节点 /UserServiceRpc
        for (auto &mp : sp.second.m_methodMap) // sp.second.methodMap就是多个方法名+方法描述
        {
            std::string method_path = service_path + "/" + mp.first; // /service1名/方法1名
            char method_path_data[128] = {0};
            //sprintf(method_path_data, "%s:%d", address.toIp().c_str(), 4545);
            sprintf(method_path_data, "%s:%d", address.toIp().c_str(), address.port());
            std::cout << "服务端的 " << address.toIpPort().c_str() << std::endl;
            // ZOO_EPHEMERAL表示znode是一个临时性节点
            char childPath[16] = {0};
            sprintf(childPath, "Node%d", address.port());
            zkCli->Create(method_path.c_str(), nullptr, 0); //也是永久的
            zkCli->CreateMultipleChildren(method_path.c_str(), childPath, method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }
}

// socket连接回调(新连接到来或者连接关闭事件)
void MprpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        // 和rpc client的连接断开了
        conn->shutdown();
    }
}


void MprpcProvider::ParseRequest(muduo::net::Buffer* buffer, RequestInfo* req_info)
{
    // retrieveAllAsString() 从某个地方比如网络连接和文件检索数据, 并以字符串的形式。
    // 返回所有检索到的数据, 这个函数通常会在网络通信或文件读取等场景中使用。
    std::string recv_buf = buffer->retrieveAllAsString(); // buffer里的，将所有转换成字符串。
    std::cout << "服务端接收解析 = " << "///////////" << recv_buf << "///////////" << recv_buf.size() << std::endl;
    uint32_t header_size = 0; // 无符号整数类型，32bit占4字节
    char* address = (char*)&header_size; // 4个字节， (char *)(uint32_t的地址) 
    recv_buf.copy(address, 4, 0); // 从recv_buf复制4个字节到address里
    std::cout << "address = " << address << std::endl; 
    std::cout << "header_size = " << header_size << std::endl;
    std::string rpc_header_str = recv_buf.substr(4, header_size); // 从recv_buf里复制[4, 4+header_size]到rpc_header_str里

    // 数据反序列化
    mprpc::RpcHeader rpcHeader;
    if (!rpcHeader.ParseFromString(rpc_header_str))
    {
        LOG_WARN << "rpc_header_str:" << rpc_header_str << " parse error!";
        return;
    }
    
    req_info->service_name = rpcHeader.service_name();
    req_info->method_name = rpcHeader.methon_name();
    req_info->args_size = rpcHeader.args_size();
    req_info->args_str = recv_buf.substr(4 + header_size, req_info->args_size);
    std::cout << "req_info->service_name = " <<  req_info->service_name << std::endl; // req_info->service_name = UserServiceRpc
    std::cout << "req_info->method_name = " <<  req_info->method_name << std::endl; // req_info->method_name = Register
    std::cout << "req_info->args_size = " << req_info->args_size << std::endl; // req_info->args_size = 17
    std::cout << "req_info->args_str = " << req_info->args_str << std::endl; // req_info->args_str = dmprpc123456

    return;
}

// 解析HTTP请求
void MprpcProvider::ParseHttpRequest(muduo::net::Buffer* buffer, HttpRequest& http_request) {
    // 查找请求头和请求体的分隔符：
    std::string request = buffer->retrieveAllAsString(); // 转换成字符串类型
    std::cout << "ParseHttpRequest开头//////" << request << "////////" << std::endl; // 输出测试
    size_t pos = request.find("\r\n\r\n"); // 在name=zhang%20san&pwd=123456开头
    if (pos != std::string::npos) { // npos是无效位置
        // 解析请求行+头
        size_t header_end = pos;
        std::string request_line_header = request.substr(0, header_end); // 请求行+头
        size_t line_end = request_line_header.find("\r\n"); 
        if (line_end != std::string::npos) {  // 在第二行开头
            // 解析请求行 POST /login HTTP/1.1
            std::string request_line = request_line_header.substr(0, line_end);
            size_t method_end = request_line.find(' ');
            if (method_end != std::string::npos) {
                http_request.method = request_line.substr(0, method_end);
                // 解析请求路径
                size_t path_start = method_end + 1; // 跳过空格
                size_t path_end = request_line.find(' ', path_start); // .find(字符，从哪儿开始)
                if (path_end != std::string::npos) {
                    http_request.path = request_line.substr(path_start, path_end - path_start);
                    //std::cout << "解析请求: " << http_request.path << std::endl;
                    size_t ipos = http_request.path.find_last_of('/');
                    if (ipos != std::string::npos && ipos != 0 && ipos != http_request.path.length() - 1) {
                        http_request.service_name = http_request.path.substr(1, ipos - 1);  // 从第二个字符开始到最后一个'/'之间的部分
                        http_request.method_name = http_request.path.substr(ipos + 1);  // 从最后一个'/'之后的部分开始到结尾的部分
                    } 

                }
            } // 请求行解析完毕

            // 解析请求头   
            size_t pos = line_end; //第二行开头
            while ((pos = request_line_header.find("\r\n", pos + 2)) != std::string::npos) {
                // 获得键值对中的键
                size_t key_end = request_line_header.find(":", line_end + 2);
                if (key_end != std::string::npos) {
                    std::string key = request_line_header.substr(line_end + 2, key_end - line_end - 2); 
                    size_t value_start = key_end + 2; // 跳过:
                    if (value_start < pos) {
                        // 获取键值对中的值
                        std::string value = request_line_header.substr(value_start, pos - value_start);
                        http_request.headers[key] = value;
                    }
                }
            }
            line_end = pos;
        } 
       
       // 解析请求体
       http_request.body = request.substr(header_end + 4);
    }
    // 输出解析后的请求信息，供调试使用
    std::cout << "请求方法 " << http_request.method << std::endl;
    std::cout << "请求路径: " << http_request.path << std::endl;
    std::cout << "请求头:" << std::endl;
    for (const auto& pair : http_request.headers) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
    std::cout << "请求体: " << http_request.body << "\t" << http_request.body.size() << std::endl;
    std::cout << "服务名: " << http_request.service_name << std::endl;
    std::cout << "方法名: " << http_request.method_name << std::endl;
}

// Closure的回调操作，用于序列化rpc的响应和网络发送
void MprpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn, 
                                    google::protobuf::Message* response)
{
    std::string response_str;
    if (response->SerializeToString(&response_str))
    {
        // 序列化成功后，通过网络将rpc方法执行结果发送给rpc的调用方
        std::cout << "在SendRpcResponse里, response_str = //////" << response_str << "///////" << std::endl;
        conn->send(response_str);
    }
    else
    {
        std::cout << "serialize response_str error!" << std::endl;
    }
    // 半关闭
    conn->shutdown();
}

void MprpcProvider::SendHttpResponse(const muduo::net::TcpConnectionPtr&conn, 
                                    google::protobuf::Message* response) {
    std::string response_str;
    // 将RPC调用结果序列化为字符串
    if (response->SerializeToString(&response_str)) {
        // 构建响应报文
        HttpResponse http_response; // 就用默认构造
        http_response.body = response_str;
        // 发送给客户端
        conn->send(http_response.toString());
    } else {
        LOG_WARN << "HTTP 响应 序列化失败";
    }
    conn->shutdown();
}

/*
如果远程有一个rpc服务的调用请求，那么OnMessage方法就会响应
这个发送的信息都是经过了协议商定的
service_name method_name args 定义proto的message类型，进行数据头的序列化和反序列化
既然是针对于数据头，那么我们需要获取指定的一些长度(需要考虑TCP黏包问题)

OnMessage(与客户端TCP连接的智能指针，缓冲区指针，时间点)
综上所述，这三个参数分别代表了与客户端的连接、接收到的消息缓冲区以及消息到达的时间戳。
*/
void MprpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn, 
                                muduo::net::Buffer* buffer,
                                muduo::Timestamp timestamp)
{
    // 解析自定义的请求信息
    RequestInfo request_info; //服务名-方法名-大小-参数
    ParseRequest(buffer, &request_info); // 缓冲区-请求信息

    // 在注册表上查找服务
    auto it = m_serviceMap.find(request_info.service_name);
    if (it == m_serviceMap.end())
    {
        LOG_WARN << request_info.service_name << " is not exist!";
        return;
    }

    auto mit = it->second.m_methodMap.find(request_info.method_name);
    if (mit == it->second.m_methodMap.end())
    {
        LOG_WARN << request_info.service_name << ":" << request_info.method_name << " is not exist!";
        return;
    }
    
    // 负责Response
    google::protobuf::Service *service = it->second.m_service;  // UserServiceRpc
    const google::protobuf::MethodDescriptor *method = mit->second;  // Login

    // 创建新的请求消息对象, 解析到request中
    // request将指向名为Login方法的请求消息的默认实例
    // 这个新创建的请求消息对象将用于填充实际的请求数据，然后通过 RPC 框架将其发送到远程服务端进行处理。
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    // 将 request_info.args_str 中的字符串解析为对应请求消息类型的对象，并存储在 request 指针指向的位置。
    if (!request->ParseFromString(request_info.args_str))
    {
        LOG_WARN << "request parse error, content:" << request_info.args_str;
        return;
    }
    // 创建新的响应消息对象, 解析到response中
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    // 给下面的method方法的调用，绑定一个Closure的回调函数
    //
    google::protobuf::Closure *done = google::protobuf::NewCallback<MprpcProvider, 
                                                                    const muduo::net::TcpConnectionPtr&, 
                                                                    google::protobuf::Message*>
                                                                    (this, 
                                                                    &MprpcProvider::SendRpcResponse, 
                                                                    conn, response);

    // 根据收到的远程RPC请求，调用当前RPC节点上发布的方法。
    // CallMethod(method, ctrl, request, response, done)
    // 这里为nullptr表示没有特定的控制器
    service->CallMethod(method, nullptr, request, response, done);
}

void MprpcProvider::OnHttpMessage(const muduo::net::TcpConnectionPtr& conn, 
                                muduo::net::Buffer* buffer,
                                muduo::Timestamp timestamp)
{
    HttpRequest request_info; //服务名-方法名-大小-参数
    ParseHttpRequest(buffer, request_info); // 缓冲区-请求信息

    auto it = m_serviceMap.find(request_info.service_name);
    if (it == m_serviceMap.end())
    {
        LOG_WARN << request_info.service_name << " is not exist!";
        return;
    }

    auto mit = it->second.m_methodMap.find(request_info.method_name);
    if (mit == it->second.m_methodMap.end())
    {
        LOG_WARN << request_info.service_name << ":" << request_info.method_name << " is not exist!";
        return;
    }
    
    google::protobuf::Service *service = it->second.m_service;  // UserServiceRpc
    const google::protobuf::MethodDescriptor *method = mit->second;  // Login
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();

    //+todo

    
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    google::protobuf::Closure *done = google::protobuf::NewCallback<MprpcProvider, 
                                                                    const muduo::net::TcpConnectionPtr&, 
                                                                    google::protobuf::Message*>
                                                                    (this, 
                                                                    &MprpcProvider::SendHttpResponse, 
                                                                    conn, response);

    service->CallMethod(method, nullptr, request, response, done);
}


// 启动rpc服务节点，开始提供rpc远程网络调用服务
void MprpcProvider::Run() 
{
    // TcpServer绑定此地址
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpc_server_ip"); //本地ip
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpc_server_port").c_str()); // 本地port
    muduo::net::InetAddress address(ip, port);

    muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider"); // 一个TCP连接的对象，server

    // 设置muduo库线程数量
    server.setThreadNum(4);

    // 绑定连接回调和消息读写回调方法  分离了网络代码和业务代码
    // this：当前Provider对象的实例
    server.setConnectionCallback(std::bind(&MprpcProvider::OnConnection, this, std::placeholders::_1));
    // 这里
    server.setMessageCallback(std::bind(&MprpcProvider::OnMessage, this, std::placeholders::_1, 
            std::placeholders::_2, std::placeholders::_3));
    // 将服务注册到zookeeper上
    ZkClient zkCli;
    RegisterZookeeper(address, &zkCli); // zkCli注册。本地ip到zkCli。注册在RPC服务IP：zookeeper的port
    
    LOG_INFO << "RpcProvider start service at ip:" << ip << " port:" << port; //3000

    // 启动网络服务
    server.start();
    m_eventLoop.loop();
}


// 使用HTTP服务，基于RPC架构
void MprpcProvider::RunHttp() 
{
    // TcpServer绑定此地址
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpc_server_ip"); //本地ip
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpc_server_port").c_str()); // 本地port
    muduo::net::InetAddress address(ip, port);

    muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider-HTTP"); // 一个TCP连接的对象，server

    // 设置muduo库线程数量
    server.setThreadNum(4);

    // 绑定连接回调和消息读写回调方法  分离了网络代码和业务代码
    // this：当前Provider对象的实例
    server.setConnectionCallback(std::bind(&MprpcProvider::OnConnection, this, std::placeholders::_1));
    // 这里
    server.setMessageCallback(std::bind(&MprpcProvider::OnHttpMessage, this, std::placeholders::_1, 
            std::placeholders::_2, std::placeholders::_3));
    // 将服务注册到zookeeper上
    ZkClient zkCli;
    RegisterZookeeper(address, &zkCli); // zkCli注册。本地ip到zkCli。注册在RPC服务IP：zookeeper的port
    
    LOG_INFO << "RpcProvider start service at ip:" << ip << " port:" << port; //3000

    // 启动网络服务
    server.start();
    m_eventLoop.loop();
}
