#include "user.pb.h"
#include "MprpcApplication.h"
#include "MprpcChannel.h"
#include "MprpcController.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <functional>

// 异步调用登录服务
static void LoginCallback(fixbug::LoginResponse* response, MprpcController* controller) {
    if (controller->Failed()) 
        std::cout << controller->ErrorText() << std::endl;
    else {
        if (0 == response->result().errcode())
            std::cout << "异步RPC Login()调用成功: " << response->success() << std::endl;
        else 
            std::cout << "RPC响应错误: " << response->result().errmsg() << std::endl;
    }
    std::cout << "login的闭包" << std::endl;
}

static void RegisterCallback(fixbug::RegisterResponse* response, MprpcController* controller) {
    if (controller->Failed()) 
        std::cout << controller->ErrorText() << std::endl;
    else {
        if (0 == response->result().errcode()) 
            std::cout << "异步RPC Register() 调用成功: " << response->success() << std::endl;
        else 
            std::cout << "RPC 响应错误: " << response->result().errmsg() << std::endl;
    }
    std::cout << "register()的闭包" << std::endl;
}

void loginServiceAsync(fixbug::UserServiceRpc_Stub& stub, MprpcController& controller) {
    // 构建RPC请求
    fixbug::LoginRequest;
    request.set_name("登录 张三");
    request.set_pwd("登录 123456");
    // 创建RPC响应
    fixbug::LoginResponse response;
    // 发起异步调用
    std::cout << "stub.Login()开始" << std::endl;
    stub.Login(&controller, &request, &response, google::protobuf::NewCallback(LoginCallback, &response, &controller));
    std::cout << "stub.Login()结束" << std::endl;
}

// 异步注册服务
void registerServiceAsync(fixbug::UserServiceRpc_Stub& stub, MprpcController& controller) {
    // 构建请求
    fixbug::RegisterRequest request;
    request.set_id(100);
    request.set_name("注册 张三");
    request.set_pwd("注册 123456");
    // 构建响应
    fixbug::RegisterResponse response;
    std::cout << "stub.Register()开始" << std::endl;
    stub.Register(&controller, &request, &response, google::protobuf::NewCallback(RegisterCallback, &response, &controller));
    std::cout << "stub.Register()结束" << std::endl;
}

// 模拟其他任务或逻辑
void otherTask() {
    std::cout << "执行其他任务或逻辑..." << std::endl;
    // 模拟耗时操作
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "其他任务或逻辑执行完成" << std::endl;
}

int main(int argc, char** argv) {
    MprpcApplication :: Init(argc, argv);
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    MprpcController controller;

    for (int i = 0; i < 2; i++) {
        std::cout <<"主线程开始第" << i << "轮RPC调用...." << std::endl;
        std::cout << "主线程login 开始" << std::endl;
        loginServiceAsync(stub, controller);
        std::cout << "主线程login结束, register开始" << std::endl;
        registerServiceAsync(stub, controller);
        std::cout << "主线程register结束" << std::endl;

        // 在异步RPC调用之后执行其他任务
        std::thread otherTaskThread(otherTask);
        otherTaskThread.detach();

        std::cout << "客户端休眠等待...";
        for (int j = 0; j < 5; j++) {
            std::cout << "过了" << j << "秒" << std::endl;
            sleep(1);
        }
        std::cout << "客户端继续进行...." ;
    }

    // 主线程等待休眠一段时间: 
    std::this_thread::sleep_for(std::chrono::milliseconds(5000)); 
    return 0;
}
