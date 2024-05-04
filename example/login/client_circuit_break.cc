// 熔断的客户端测试代码
#include "user.pb.h"
#include "MprpcApplication.h"
#include "MprpcChannel.h"
#include "MprpcController.h"
#include <iostream>

#include <chrono> // 是为了给线程设置时间，是c11里的
#include <thread> // 后台线程，刷新用，是c11里的
#include <atomic>

std::atomic<bool> isExit(false); // 退出标志位


// 同步RPC
// fixbug是proto里的
void loginService(fixbug::UserServiceRpc_Stub& stub, MprpcController& controller)
{
    fixbug::LoginRequest request;
    request.set_name("zhang san");
    request.set_pwd("123456");
    fixbug::LoginResponse response;
    stub.Login(&controller, &request, &response, nullptr);
    if (controller.Failed()) {
        std::cout << controller.ErrorText() << std::endl;
    } else {
        if (0 == response.result().errcode()) {
            std::cout << "rpc Login response success:" << response.success() << std::endl;
        } else {
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
    if (controller.Failed()) {
        std::cout << controller.ErrorText() << std::endl;
    } else {
        if (0 == rsp.result().errcode()) {
            std::cout << "rpc Register response success:" << rsp.success() << std::endl;
        } else {
            std::cout << "rpc response error : " << rsp.result().errmsg() << std::endl;
        }
    }
}

int main(int argc, char **argv)
{
    MprpcApplication::Init(argc, argv);
    MprpcChannel* rpc = new MprpcChannel();
    fixbug::UserServiceRpc_Stub stub(rpc);
    MprpcController controller;

    // 开启后台线程进行刷新
    std::thread refreshThread(
        [&]() {
            while (!isExit.load()) {
                LOG_INFO << "               刷新熔断器...";
                std::this_thread::sleep_for(std::chrono::seconds(30)); // 30s刷新一次
                rpc->refreshCache();
                LOG_INFO << "               刷新熔断器完成...";
            }
            return ;
        }
    );

    for (int i = 0; i < 5; i++) {
        std::cout << "这是第" << i << "轮调用" << std::endl;
        loginService(stub, controller);
        sleep(3); // 等待3s
    }

    isExit.store(true);
    refreshThread.join();
    delete rpc;
    return 0;
}