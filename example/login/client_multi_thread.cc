#include "user.pb.h"
#include "MprpcApplication.h"
#include "MprpcChannel.h"
#include "MprpcController.h"

#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>

//互斥锁和条件变量
std::mutex mtx;
std::condition_variable cv;
bool ready = false;

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

void clientThread(const std::string& configFile) {
    // 初始化客户端
    char* argv[] = {const_cast<char*>("./client_multi_thread"), const_cast<char*>("-i"), const_cast<char*>(configFile.c_str()), nullptr};
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return ready; });
    }
    MprpcApplication::Init(3, argv);
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    MprpcController controller;

    loginService(stub, controller); // 调用登录服务
    registerService(stub, controller); // 调用注册服务
}

int main(int argc, char **argv)
{
    const int numClients = 4; // 启动的客户端数量
    std::vector<std::thread> clientThreads; // 客户端线程容器
    std::vector<std::string> configFiles = {"test1.conf", "test2.conf", "test3.conf", "test4.conf"}; // 指定配置文件路径
    
    {
        std::lock_guard<std::mutex> lock(mtx);
        ready = true;
        cv.notify_all();
    }
    
    for (int i = 0; i < numClients; ++i) { // 创建并启动多个客户端线程
        clientThreads.emplace_back(clientThread, configFiles[i]);
    }

    for (auto& thread : clientThreads) { // 等待所有客户端线程结束
        thread.join();
    }

    return 0;
}
