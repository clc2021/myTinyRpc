-- 导入 Protocol Buffers 库
local pb = require "pb"
local protoc = require "protoc"

-- 加载 Protocol Buffers 定义文件
protoc:loadfile("/home/ubuntu/projects/tinyrpcGai/src/rpc_header.proto")

-- 创建登录请求消息对象并填充数据
local rpcHeader = {service_name = "UserServiceRpc", method_name = "Login", args_size = 14}
local request_data = "My Rpc123456"  -- 请求体数据

-- 序列化登录请求消息对象
local request_buffer = pb.encode("mprpc.RpcHeader", rpcHeader)

-- 创建完整的 HTTP 请求
wrk.method = "POST"  -- 设置请求方法为 POST
wrk.body   = request_buffer .. request_data  -- 将序列化的请求消息对象和请求体数据合并作为请求体
wrk.headers["Content-Type"] = "application/octet-stream"  -- 设置 Content-Type 为 application/octet-stream

-- 发送 RPC 请求
function request()
    return wrk.format(nil, "/UserServiceRpc/Login")  -- 发送到登录接口
end
