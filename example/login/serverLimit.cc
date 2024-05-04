#include "user.pb.h"
#include "MprpcApplication.h"
#include "MprpcProvider.h"
#include "MprpcController.h"

#include <iostream>
#include <string>

// 是限流+降级的RPC服务端
#include "LimitProcess.h"

class UserService : public fixbug::UserServiceRpc  
{
private:
    LimitProcess limitProcess; // 添加限流处理对象
    FallBackClass fallBackObj; // 添加降级类对象

public:
    bool Login(const std::string& name, const std::string& pwd)
    {
        std::cout << "在服务端: Login" << std::endl;
        std::cout << "name:" << name << " pwd:" << pwd << std::endl;
    }
    
    bool Register(uint32_t id, std::string name, std::string pwd)
    {
        std::cout << "在服务端: Register" << std::endl;
        std::cout << "id" << id << "name:" << name << " pwd:" << pwd << std::endl;
        this->name = name;
    }

    void Login(::google::protobuf::RpcController* controller,
                    const ::fixbug::LoginRequest* request,
                    ::fixbug::LoginResponse* response,
                    ::google::protobuf::Closure* done)
    {

        std::string name = request->name();
        std::string pwd = request->pwd();

        // 进行降级处理
        std::function<void()> fallBackMethod = fallBackObj.handleFallback1; // 设置降级方法为 handleFallback1
        LimitingRule loginFallBackRule("Login", fallBackMethod, IMMEDIATE_REFUSE, "", nullptr, 100);  // 设置最大QPS为100
        bool fallBackTriggered = limitProcess.limitHandle(loginFallBackRule);
        if (fallBackTriggered) {
            std::cout << "Login请求被降级" << std::endl;
            fixbug::ResultCode* code = response->mutable_result();
            code->set_errcode(2);
            code->set_errmsg("请求被降级");
            response->set_success(false);
            if (done)
                done->Run();
            return ;
        }

        // 进行限流处理
        LimitingRule loginLimitRule("Login", nullptr, IMMEDIATE_REFUSE, "", nullptr, 100); // 设置最大QPS为100
        bool loginAllowed = limitProcess.limitHandle(loginLimitRule);
        if (!loginAllowed) {
            std::cout << "Login请求被限流" << std::endl;
            fixbug::ResultCode *code = response->mutable_result();
            code->set_errcode(1);
            code->set_errmsg("请求被限流");
            response->set_success(false);
            if (done)
                done->Run();
            return ;
        }

        // 继续处理登录任务
        bool login_result = Login(name, pwd);

        fixbug::ResultCode *code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("在服务端: 成功");
        response->set_success(login_result);

        if (done)
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

        // 进行降级处理
        std::function<void()> fallBackMethod = fallBackObj.handleFallback2; 
        LimitingRule registerFallBackRule("Register", fallBackMethod, IMMEDIATE_REFUSE, "", nullptr, 100); // 最大请求是100
        bool fallBackTriggered = limitProcess.limitHandle(registerFallBackRule);
        if (fallBackTriggered) {
            std::cout << "Register请求被降级" << std::endl;
            response->mutable_result()->set_errcode(2);
            response->mutable_result()->set_errmsg("请求被降级");
            response->set_success(false);
            if (done)
                done->Run();
            return ;
        }

        // 进行限流处理
        LimitingRule registerLimitRule("Register", nullptr, IMMEDIATE_REFUSE, "", nullptr, 100); // 设置最大是100
        bool registerAllowed = limitProcess.limitHandle(registerLimitRule);
        if (!registerAllowed) {
            std::cout << "Register请求被限流" << std::endl;
            response->mutable_result()->set_errcode(1);
            response->mutable_result()->set_errmsg("请求被限流");
            response->set_success(false);
            if (done)
                done->Run();
            return ;
        }

        // 继续处理注册业务
        bool ret = Register(id, name, pwd);

        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        response->set_success(ret);

        if (done)
            done->Run();
    }
};

int main(int argc, char **argv)
{
    MprpcApplication::Init(argc, argv); 
    MprpcProvider provider; 
    UserService* service = new UserService();
    provider.NotifyService(service);
    provider.Run();
    return 0;
}