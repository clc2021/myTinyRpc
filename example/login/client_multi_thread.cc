#include "user.pb.h"
#include "MprpcApplication.h"
#include "MprpcChannel.h"
#include "MprpcController.h"

#include <iostream>
#include <thread>
#include <vector>
#include <string>

void loginService(fixbug::UserServiceRpc_Stub& stub, MprpcController& controller)
{
     fixbug::LoginRequest request;
    request.set_name("zhang san");
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
            std::cout << "rpc Login response success:" << response.success() << std::endl;
        }
        else
        {
            std::cout << "rpc response error : " << response.result().errmsg() << std::endl;
        }
    }
}

void registerService(fixbug::UserServiceRpc_Stub& stub, MprpcController& controller)
{
    fixbug::RegisterRequest req;
    req.set_id(100);
    req.set_name("mprpc");
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
            std::cout << "rpc Register response success:" << rsp.success() << std::endl;
        }
        else
        {
            std::cout << "rpc response error : " << rsp.result().errmsg() << std::endl;
        }
    }
}

void clientThread(const std::string& configFile, int count) {
    // 初始化客户端
    char* argv[] = {const_cast<char*>("./client_multi_thread"), const_cast<char*>("-i"), const_cast<char*>((configFile + std::to_string(count) + std::string(".conf")).c_str()), nullptr};
    std::cout << argv[2] << std::endl;
    MprpcApplication::Init(3, argv);
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    MprpcController controller;

    // 调用登录服务
    loginService(stub, controller);

    // 调用注册服务
    registerService(stub, controller);
}

int main(int argc, char **argv)
{
    // 启动的客户端数量
    const int numClients = 4;
    int count = 0;

    // 客户端线程容器
    std::vector<std::thread> clientThreads;

    // 指定配置文件路径
    std::string configFile = "test";

    // 创建并启动多个客户端线程
    for (int i = 0; i < numClients; ++i) {
        count++;
        clientThreads.emplace_back(clientThread, configFile, count);
        sleep(1);
    }

    // 等待所有客户端线程结束
    for (auto& thread : clientThreads) {
        thread.join();
    }

    return 0;
}
