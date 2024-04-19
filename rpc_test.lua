wrk.method = "POST"  -- 设置请求方法为 POST
wrk.body   = "dmprpc123456"  -- 设置请求体，模拟登录请求

-- 定义请求头
wrk.headers["Content-Type"] = "text/plain"

-- 发送 RPC 请求
function request()
    return wrk.format(nil, "/UserServiceRpc/Register")  -- 发送到登录接口
end

