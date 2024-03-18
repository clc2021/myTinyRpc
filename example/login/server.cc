#include "user.pb.h"
#include "MprpcApplication.h"
#include "MprpcProvider.h"
#include "MprpcController.h"

#include <iostream>
#include <string>

// 一个本地服务，现在要将它变成rpc服务
class UserService : public fixbug::UserServiceRpc  // 这里用fixbug，是因为在package fixbug
{   
public:
    bool Login(const std::string& name, const std::string& pwd)
    {
        std::cout << "doing local service: Login" << std::endl;
        std::cout << "name:" << name << " pwd:" << pwd << std::endl;
    }
    
    bool Register(uint32_t id, std::string name, std::string pwd)
    {
        std::cout << "doing local service: Register" << std::endl;
        std::cout << "id" << id << "name:" << name << " pwd:" << pwd << std::endl;
    }

    void Login(::google::protobuf::RpcController* controller,
                    const ::fixbug::LoginRequest* request,
                    ::fixbug::LoginResponse* response,
                    ::google::protobuf::Closure* done)
    {
        // 框架给业务上报了请求参数 LoginRequest，应用获取数据进行业务
        std::string name = request->name();
        std::string pwd = request->pwd();

        // 此处是本地业务
        // 调用了第一个Login()处理本地登录操作，结果存储在login_result变量里
        // 
        bool login_result = Login(name, pwd);

        // 把响应写入
        fixbug::ResultCode *code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("success");
        response->set_success(login_result);

        // 执行回调操作。都是由框架来
        // 完成响应对象数据的序列化和网络发送
        done->Run();
    }

    void Register(::google::protobuf::RpcController* controller,
                const ::fixbug::RegisterRequest* request,
                ::fixbug::RegisterResponse* response,
                ::google::protobuf::Closure* done)
    {
        uint32_t id = request->id();
        std::string name = request->name();
        std::string pwd = request->pwd();

        bool ret = Register(id, name, pwd);

        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        response->set_success(ret);

        done->Run();
    }

};

int main(int argc, char **argv)
{
    MprpcApplication::Init(argc, argv); // 第1步: 对 RPC 框架进行初始化操作，配置信息通过命令行传递。
    // provider是一个rpc网络服务对象，把userService对象发布到rpc节点上
    MprpcProvider provider; // 第2步: 生成一个 provider 对象，用于将业务发布到 RPC 节点上。
    provider.NotifyService(new UserService());

    // 启动一个rpc发布节点，进入阻塞状态等待远处rpc请求
    // 启动muduo的事件循环 
    provider.Run(); // 第3步: 启动RPC发布节点，阻塞等待远程RPC请求。 

    return 0;
}