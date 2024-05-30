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
    fixbug::LoginResponse response;
    stub.Login(&controller, &request, &response, nullptr); 
    if (controller.Failed()) 
    {
        std::cout << controller.ErrorText() << std::endl;
    }
    else
    {
        if (0 == response.result().errcode())
        {
            std::cout << "调用远程服务方法成功:" << response.success() << std::endl;
        }
        else
        {
            std::cout << "调用远程服务方法失败: " << response.result().errmsg() << std::endl;
        }
    }
}

void registerService(fixbug::UserServiceRpc_Stub& stub, MprpcController& controller)
{
    fixbug::RegisterRequest req;
    req.set_id(100);
    req.set_name("My Rpc");
    req.set_pwd("123456");
    fixbug::RegisterResponse rsp;

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
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    MprpcController controller;

    for (int i = 0; i < 1; i++) {
        loginService(stub, controller);
        sleep(1);
        registerService(stub, controller);
    }
    return 0;
}