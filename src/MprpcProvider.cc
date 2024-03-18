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
/*
InetAddress：IP地址和端口号
muduo::net::InetAddress address("127.0.0.1", 8080); // IPv4 地址示例
muduo::net::InetAddress address("::1", 8080);      // IPv6 地址示例

*/
// RegisterZookeeper(IP:port, zookeeper客户端) 服务端保留的是字符串服务，然后根据这个ip地址进行服务注册
// RegisterZookeeper会把字符串类型的服务和ip:port一块注册到zookeeper上
// 但是注意：这里字符串是路径，服务名变成路径，而ip:port是值，服务地址是值
//  ____________________
// |                    |
// |__服务名__|__地址___| 
// |  Login   |         |
// |_Register_|_________|        
// |__________|_________|

// 这个address是rpc服务器的地址：127.0.0.1:3000的那个
// 但是代码里注册是ip+2181
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
            // std::cout << "method_path = " << method_path <<std::endl;
            //               method_path = /UserServiceRpc/Register
            //               method_path = /UserServiceRpc/Login
            char method_path_data[128] = {0};
            // 这个为什么要和zk_port一样？？？？？？？？？？？？？？？？？？
            //+ 将12182改成4545
            //sprintf(method_path_data, "%s:%d", address.toIp().c_str(), 4545);
            sprintf(method_path_data, "%s:%d", address.toIp().c_str(), address.port());
            std::cout << "服务端的 " << address.toIpPort().c_str() << std::endl;
            // printf("%s:%d", address.toIp().c_str(), 2181); 
            // // 127.0.0.1:
            // ZOO_EPHEMERAL表示znode是一个临时性节点
            char childPath[16] = {0};
            sprintf(childPath, "Node%d", address.port());
            zkCli->Create(method_path.c_str(), nullptr, 0); //也是永久的
            zkCli->CreateMultipleChildren(method_path.c_str(), childPath, method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
            // std::cout<< "zkCli->CreateMultipleChildren(节点路径" << method_path.c_str() << "节点初始值, " << method_path_data << std::endl;
            //              zkCli->Create(节点路径/UserServiceRpc/Register节点初始值, 127.0.0.1:2181
            //              zkCli->Create(节点路径/UserServiceRpc/Login节点初始值, 127.0.0.1:2181
        }
    }
}


// 启动rpc服务节点，开始提供rpc远程网络调用服务
void MprpcProvider::Run() 
{
    // TcpServer绑定此地址
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpc_server_ip"); //本地ip
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpc_server_port").c_str()); // 本地port
    muduo::net::InetAddress address(ip, port);

    // 创建 TcpServer 对象  
    // &m_eventLoop：这是一个指向 EventLoop 对象的指针。EventLoop 是 Muduo 网络库中的事件循环，
    // 负责处理事件分派和调度。通过将 TcpServer 关联到特定的事件循环，可以确保网络事件的处理与程序
    // 的主事件循环同步。
    // address：这是一个 muduo::net::InetAddress 对象，表示服务器监听的地址和端口。在这个例子中，
    // 它指定了服务器要绑定的 IP 地址和端口号。
    // "RpcProvider"：这是服务器的名字，用于标识这个 TcpServer 实例。这个名字通常用于日志记录和诊
    // 断，以便在多个服务器实例运行时区分它们。
    // 这个对象的作用是告诉 TcpServer 类在哪个地址上监听传入的连接。当客户端想要连接到服务器时，
    // 它们会使用这个地址和端口来尝试建立连接。服务器会监听这个地址上的端口，并在有新连接到来时进行处理
    // 事件循环组件；TCP服务器绑定的地址，也就是服务器的IP地址和端口号。；字符串，用来表示TCP服务器名称
    // address: TcpServer在哪个地址上监听传入的连接。
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

// socket连接回调(新连接到来或者连接关闭事件)
void MprpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        // 和rpc client的连接断开了
        conn->shutdown();
    }
}

// 解析数据包
// 将buffer前四个字节读出来，方便知道接下来要读取多少数据
// muduo::net::Buffer类型 通常用来管理接收和发送数据的缓冲区
void MprpcProvider::ParseRequest(muduo::net::Buffer* buffer, RequestInfo* req_info)
{
    // retrieveAllAsString() 从某个地方比如网络连接和文件检索数据, 并以字符串的形式
    // 返回所有检索到的数据, 这个函数通常会在网络通信或文件读取等场景中使用。
    std::string recv_buf = buffer->retrieveAllAsString(); // 网络上接受的远程rpc调用请求的字符流
    
    uint32_t header_size = 0; // 无符号整数类型，32bit占4字节
    char* address = (char*)&header_size; // 4个字节， (char *)(uint32_t的地址)
    recv_buf.copy(address, 4, 0); // 从recv_buf复制4个字节到address里
    LOG_INFO << "服务端ParseRequest: address = " << address;

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
    
    return;
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
    std::cout << "OnMessage调用ParrseRequest" << std::endl;
    ParseRequest(buffer, &request_info); // 缓冲区-请求信息
    std::cout << "OnMessage调用ParrseRequest完成" << std::endl;

#if 0
    // 打印调试信息
    LOG_INFO << "============================================";
    LOG_INFO << "header_size: " << header_size <<; 
    LOG_INFO << "rpc_header_str: " << rpc_header_str <<;
    LOG_INFO << "service_name: " << service_name <<;
    LOG_INFO << "method_name: " << method_name <<;
    LOG_INFO << "args_str: " << args_str <<;
    LOG_INFO << "============================================";
#endif

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
    google::protobuf::Service *service = it->second.m_service; 
    const google::protobuf::MethodDescriptor *method = mit->second; 

    // 生成rpc方法调用的请求request和响应response参数
    // request参数需要单独解析，之前只是知道了request的序列化字符串长度
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if (!request->ParseFromString(request_info.args_str))
    {
        LOG_WARN << "request parse error, content:" << request_info.args_str;
        return;
    }
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    // 给下面的method方法的调用，绑定一个Closure的回调函数
    google::protobuf::Closure *done = google::protobuf::NewCallback<MprpcProvider, 
                                                                    const muduo::net::TcpConnectionPtr&, 
                                                                    google::protobuf::Message*>
                                                                    (this, 
                                                                    &MprpcProvider::SendRpcResponse, 
                                                                    conn, response);

    // 在框架上根据远端rpc请求，调用当前rpc节点上发布的方法
    // new UserService().Login(controller, request, response, done)
    service->CallMethod(method, nullptr, request, response, done);
}

// Closure的回调操作，用于序列化rpc的响应和网络发送
/*
SerializeToString() 是 Google Protocol Buffers（protobuf）库中的一个函数，
用于将消息对象序列化为字符串。Protocol Buffers 是一种轻量级的数据交换格式，
它能够有效地将结构化的数据序列化为紧凑的二进制格式，并且可以反序列化为原始的
结构化数据。
在使用 Protocol Buffers 时，我们首先定义一个消息类型（Message），然后使用
编译器生成的代码（例如 protoc）生成对应的数据结构和序列化/反序列化方法。
SerializeToString() 就是其中之一，它用于将消息对象序列化为字符串。
具体来说，SerializeToString() 接受一个消息对象作为参数，将该对象的数据按照
定义的消息格式序列化为一个字符串。这个字符串可以保存到文件中、通过网络发送、
存储到数据库等等。反之，我们也可以使用反序列化函数（如 ParseFromString()）
将这个字符串解析成对应的消息对象。
*/
void MprpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn, 
                                    google::protobuf::Message* response)
{
    std::string response_str;
    if (response->SerializeToString(&response_str))
    {
        // 序列化成功后，通过网络将rpc方法执行结果发送给rpc的调用方
        conn->send(response_str);
    }
    else
    {
        std::cout << "serialize response_str error!" << std::endl;
    }
    // 半关闭
    conn->shutdown();
}