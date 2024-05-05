#include "user.pb.h"
#include "MprpcApplication.h"
#include "MprpcProvider.h"
#include "MprpcController.h"

#include <iostream>
#include <string>
#include <functional>
// 是限流+降级的RPC服务端
#include "./limit/LimitProcess.h"
#include <thread>

// 记住这个扩展了protobuf的类叫做UserService, 
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
    }

    void Login(::google::protobuf::RpcController* controller,
                    const ::fixbug::LoginRequest* request,
                    ::fixbug::LoginResponse* response,
                    ::google::protobuf::Closure* done)
    {

        std::string name = request->name();
        std::string pwd = request->pwd();

        // 进行降级处理
        std::function<void()> fallBackMethod = std::bind(&FallBackClass::handleFallback1, &fallBackObj); // 设置降级方法为 handleFallback1
        // LimitingRule(id, fallBackMethod, blockStrategy, limitKey, limitValue, maxQPS) 
        // 限流的id是Login,方法是降级1，拒绝，limitKey和limitValue都是空, 100
        LimitingRule loginFallBackRule("Login", fallBackMethod, IMMEDIATE_REFUSE, "", nullptr, 100);  // 设置最大QPS为100
        std::cout << "fallBackTriggered = 开头: " << std::this_thread::get_id() << std::endl;
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
        // 限流的id是Login, 方法是空，拒绝，limitKey和limitValue都是空, 100
        LimitingRule loginLimitRule("Login", nullptr, IMMEDIATE_REFUSE, "", nullptr, 100); // 设置最大QPS为100
        std::cout << "loginAllowed = 开头: " << std::this_thread::get_id() << std::endl;
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
        std::function<void()> fallBackMethod = std::bind(&FallBackClass::handleFallback2, &fallBackObj); 
        LimitingRule registerFallBackRule("Register", fallBackMethod, IMMEDIATE_REFUSE, "", nullptr, 100); // 最大请求是100
        std::cout << "R fallBackTriggered = 开头: " << std::this_thread::get_id() << std::endl;
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
        std::cout << "registerAllowed = 开头: " << std::this_thread::get_id() << std::endl;
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
    std::cout << "main函数开头: " << std::this_thread::get_id() << std::endl;
    MprpcProvider provider; 
    UserService* service = new UserService(); // 就是造了一个UserService的类对象
    provider.NotifyService(service); // 
    provider.Run(); // 等待执行
    return 0;
}