#include "user.pb.h"
#include "MprpcApplication.h"
#include "MprpcChannel.h"
#include "MprpcController.h"
#include <iostream>
#include <chrono>
#include <thread>

// void Wait() {
//     std::unique_lock<std::mutex> lock_(mutex_);
//     while (!ready_)
//         condition_.wait(lock_);
// }

// void Notify() {
//     std::lock_guard<std::mutex> lock_(mutex_);
//     ready_ = true;
//     condition_.notify_all();
// }

// 继承google::protobuf::Closure
// fixbug是user.proto里的
class LoginClosure: public google::protobuf::Closure {
    private:
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
        delete this; // 释放内存
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
    stub.Login(&controller, &request, &response, new LoginClosure(response, controller));
}

// 同理来构建Register
class RegisterClosure: public google::protobuf::Closure {
    private:
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
        delete this;
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
    stub.Register(&controller, &request, &response, new RegisterClosure(response, controller));
}

int main(int argc, char** argv) {
    MprpcApplication :: Init(argc, argv);
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    MprpcController controller;

    for (int i = 0; i < 5; i++) {
        std::cout <<"主线程开始第" << i << "轮RPC调用...." << std::endl;
        loginServiceAsync(stub, controller);
        registerServiceAsync(stub, controller);
        std::cout << "客户端休眠等待..." << std:: endl;
        std::this_thread::sleep_for(std::chrono::seconds(1)); //1s
        std::cout << "客户端继续进行...." << std::endl;
        
    }
    return 0;
}