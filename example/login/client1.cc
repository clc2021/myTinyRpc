#include "user.pb.h"
#include "MprpcApplication.h"
#include "MprpcChannel.h"
#include "MprpcController.h"

#include <iostream>

void loginService(fixbug::UserServiceRpc_Stub& stub, MprpcController& controller)
{
    fixbug::LoginRequest request;
    request.set_name("My Rpc"); 
    request.set_pwd("123456");
    // rpc方法的响应
    fixbug::LoginResponse response;

    // 发起rpc方法的调用。这里的参数通过request传递
    stub.Login(&controller, &request, &response, nullptr); // 一台主机上的进程A，通过参数传递的方式调用B上的函数

    // 如果rpc远程调用失败，打印错误信息
    if (controller.Failed()) // 调用Login(ctrl, req, res,)，先看ctrl行不行
    {
        std::cout << controller.ErrorText() << std::endl;
    }
    // ctrl可以，调用rpc成功
    else
    {
        // 业务成功响应码为0
        if (0 == response.result().errcode())
        {
            std::cout << "调用远程服务方法成功:" << response.success() << std::endl;
        }
        // 业务失败打印错误信息
        else
        {
            std::cout << "调用远程服务方法失败: " << response.result().errmsg() << std::endl;
        }
    }
}

void registerService(fixbug::UserServiceRpc_Stub& stub, MprpcController& controller)
{
    // 演示调用Register
    fixbug::RegisterRequest req;
    // TODO:整数形式是否可行
    req.set_id(100);
    req.set_name("My Rpc");
    req.set_pwd("123456");
    fixbug::RegisterResponse rsp;

    // 以同步的方式发起rpc调用请求，等待返回结果
    stub.Register(&controller, &req, &rsp, nullptr);
    if (controller.Failed())
    {
        std::cout << controller.ErrorText() << std::endl;
    }
    else
    {
        if (0 == rsp.result().errcode())
        {
            std::cout << "调用远程服务方法成功: " << rsp.success() << std::endl;
        }
        else
        {
            std::cout << "调用远程服务方法失败: " << rsp.result().errmsg() << std::endl;
        }
    }
}

int main(int argc, char **argv)
{
    MprpcApplication::Init(argc, argv);

    // 调用远程发布的rpc方法Login
    //+在这里可以加入对负载均衡策略的选择
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    MprpcController controller;

    for (int i = 0; i < 10; i++) {
        loginService(stub, controller);
        sleep(1);
        // registerService(stub, controller);
    }
    return 0;
}