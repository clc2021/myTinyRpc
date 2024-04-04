#include "user.pb.h"
#include "MprpcApplication.h"
#include "MprpcChannel.h"
#include "MprpcController.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <future> // 异步操作
#include <functional>
#include <memory>

class LoginClosure: public google::protobuf::Closure {
    public:
    fixbug::LoginResponse& response_;
    MprpcController& controller_;

    public:
    LoginClosure(fixbug::LoginResponse& response, MprpcController& controller): 
    response_(response), controller_(controller) {
        std::cout << "LoginClosure调用" << std::endl;
    }
    
    // 实现Run()
    void Run() override {
        std::cout << "LoginClosure的Run()调用..." << std::endl;
        if (controller_.Failed()) 
            std::cout << controller_.ErrorText() << std::endl;
        else {
            if (0 == response_.result().errcode())
                std::cout << "异步RPC Login()调用成功: " << response_.success() << std::endl;
            else 
                std::cout << "RPC响应错误: " << response_.result().errmsg()  << std::endl;
        }
        std::cout << "login的闭包" << std::endl;
    }

};

// 异步调用登录服务
void loginServiceAsync(fixbug::UserServiceRpc_Stub& stub, MprpcController& controller) {
    // 构建RPC请求
    fixbug::LoginRequest request;
    request.set_name("登录 张三");
    request.set_pwd("登录 123456");
    // 创建RPC响应
    fixbug::LoginResponse response;
    // 发起异步调用
    //LoginClosure* closure = new LoginClosure(response, controller);
    // stub.Login(&(closure->controller_), &request, &(closure->response_), closure);
    std::unique_ptr<LoginClosure> closure(new LoginClosure(response, controller));
    stub.Login(&controller, &request, &response, closure.release());
}

// 同理来构建Register
class RegisterClosure: public google::protobuf::Closure {
    public:
    fixbug::RegisterResponse& response_;
    MprpcController& controller_;

    public:
    RegisterClosure(fixbug::RegisterResponse& response, MprpcController& controller):
        response_(response), controller_(controller) {}

    void Run() override {
        if (controller_.Failed()) 
            std::cout << controller_.ErrorText() << std::endl;
        else {
            if (0 == response_.result().errcode()) 
                std::cout << "异步RPC Register() 调用成功: " << response_.success() << std::endl;
            else 
                std::cout << "RPC 响应错误: " << response_.result().errmsg() << std::endl;
        }

        std::cout << "register()的闭包" << std::endl;
    }
};

// 异步注册服务
void registerServiceAsync(fixbug::UserServiceRpc_Stub& stub, MprpcController& controller) {
    // 先构建请求
    fixbug::RegisterRequest request;
    request.set_id(100);
    request.set_name("注册 张三");
    request.set_pwd("注册 123456");
    // 构建响应
    fixbug::RegisterResponse response;
    std::unique_ptr<RegisterClosure> closure(new RegisterClosure(response, controller));
    // stub.Register(&(closure->controller_), &request, &(closure->response_), closure);
    stub.Register(&controller, &request, &response, closure.release());
}

int main(int argc, char** argv) {
    MprpcApplication :: Init(argc, argv);
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    MprpcController controller;

    for (int i = 0; i < 2; i++) {
        std::cout <<"主线程开始第" << i << "轮RPC调用...." << std::endl;
        std::cout << "客户端发送login" << std::endl;
        std::thread loginThr([&]() { loginServiceAsync(stub, controller); });
        loginThr.detach();
        std::cout << "客户端发送完login, 发送register" << std::endl;
        std::thread registerThr([&]() { registerServiceAsync(stub, controller); });
        registerThr.detach();
        std::cout << "客户端发送完register" << std::endl;
        std::cout << "客户端继续进行...." << std::endl;
    }

    // 主线程等待休眠一段时间: 
    std::this_thread::sleep_for(std::chrono::milliseconds(5000)); 
    return 0;
}