// 熔断的客户端测试代码
#include "user.pb.h"
#include "MprpcApplication.h"
#include "MprpcChannel.h"
#include "MprpcController.h"
#include <iostream>

#include <chrono> // 是为了给线程设置时间，是c11里的
#include <thread> // 后台线程，刷新用，是c11里的

// 熔断刷新任务
void refreshCacheTask(MprpcChannel* rpcChannel) {
    while (true) {
        rpcChannel->refreshCache();
        std::this_thread::sleep_for(std::chrono::seconds(10)); // 10s刷新一次
    }
}

void startRefreshTask(MprpcChannel* rpcChannel) { 
    std::thread refreshThread(refreshCacheTask, rpcChannel); // 创建刷新线程
    refreshThread.detach(); // 设置为后台线程
}

// 同步RPC
// fixbug是proto里的
void loginService(fixbug::UserServiceRpc_Stub& stub, MprpcController& controller)
{
    fixbug::LoginRequest request;
    request.set_name("zhang san");
    request.set_pwd("123456");
    // rpc方法的响应
    fixbug::LoginResponse response;

    // 发起rpc方法的调用
    stub.Login(&controller, &request, &response, nullptr);

    // 如果rpc远程调用失败，打印错误信息
    if (controller.Failed())
    {
        std::cout << controller.ErrorText() << std::endl;
    }
    // 调用rpc成功
    else
    {
        // 业务成功响应码为0
        if (0 == response.result().errcode())
        {
            std::cout << "rpc Login response success:" << response.success() << std::endl;
        }
        // 业务失败打印错误信息
        else
        {
            std::cout << "rpc response error : " << response.result().errmsg() << std::endl;
        }
    }
}

void registerService(fixbug::UserServiceRpc_Stub& stub, MprpcController& controller)
{
    // 演示调用Register
    fixbug::RegisterRequest req;
    // TODO:整数形式是否可行
    req.set_id(100);
    req.set_name("mprpc");
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
            std::cout << "rpc Register response success:" << rsp.success() << std::endl;
        }
        else
        {
            std::cout << "rpc response error : " << rsp.result().errmsg() << std::endl;
        }
    }
}

int main(int argc, char **argv)
{
    MprpcApplication::Init(argc, argv);

    // 调用远程发布的rpc方法Login
    //+在这里可以加入对负载均衡策略的选择
    MprpcChannel* rpc = new MprpcChannel();
    fixbug::UserServiceRpc_Stub stub(rpc);
   
    MprpcController controller;

    for (int i = 0; i < 10; i++) {
        startRefreshTask(rpc); // 刷新
        loginService(stub, controller);
        registerService(stub, controller);
        sleep(5); // 等待5s
    }
    return 0;
}