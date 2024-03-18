/*
我们可以通过RpcController方法得到RPC调用是否发生错误，并重置RPC连接。


*/
#ifndef __MPRPC_CONTROLLER_H__
#define __MPRPC_CONTROLLER_H__

#include <google/protobuf/service.h>
#include <string>

class MprpcController : public google::protobuf::RpcController
{
public:
    MprpcController();
    void Reset(); // 将RpcController重设为初始状态，以便在调用中可以重新使用。
    bool Failed() const; // 调用失败返回true，只能在客户端调用且不能在调用结束前调用。
    std::string ErrorText() const; // 可读的错误描述信息
    void SetFailed(const std::string& reason);

    // TODO:目前未实现具体的功能
    // startCancel()告知 RPC 系统，调用者希望取消 RPC 调用。RPC 系统可以立即取消它，
    // 也可以等待一段时间后再取消它，或者甚至根本不取消调用。如果调用被取消，done回调
    // 仍将被调用，RpcController 将表明当时的调用失败。
    void StartCancel(); 
    // isCanceled() 如果为真，表示客户端取消了 RPC，所以服务器可能会放弃对它的回复。
    // 这个方法必须只在服务器端调用。服务器仍然应该调用最后的 "完成 "回调。
    bool IsCanceled() const;
    // NotifyOnCancel()要求在 RPC 被取消时调用给定的回调。传递给回调的参数将永远是空的。
    // 回调将总是被精确地调用一次。如果 RPC 完成而没有被取消，回调将在完成后被调用。
    // 如果当 NotifyOnCancel() 被调用时，RPC 已经被取消了，回调将被立即调用。
    // NotifyOnCancel() 必须在每个请求中被调用不超过一次。它必须只在服务器端调用。
    void NotifyOnCancel(google::protobuf::Closure* callback);
private:
    bool m_failed; // RPC方法执行过程中的状态
    std::string m_errText; // RPC方法执行过程中的错误信息
};

#endif // __MPRPC_CONTROLLER_H__