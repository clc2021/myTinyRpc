set(SRC_LIST
    ./include/limit/FallBackMethod.cc
    ./limit/SampleEntity.cc
    ./limit/SampleWindow.cc
    ./limit/SlidingWindow.cc
    ./limit/LimitProcess.cc
    RoundRobinLoadBalancer.cc 
    ConsistentHashLoadBalancer.cc
    MprpcApplication.cc
    MprpcProvider.cc
    rpc_header.pb.cc
    MprpcChannel.cc
    MprpcController.cc
    ZookeeperUtil.cc
    FuseProtector.cc
    )
    
# aux_source_directory() 用于将指定目录下所有源文件都添加到一个变量中
aux_source_directory(mprpc SRC_LIST)
# 生成静态库: 生成可执行文件用 add_exe, 生成静态库用add_library()
add_library(mprpc ${SRC_LIST})  
# 目标需要链接的库
target_link_libraries(mprpc muduo_net muduo_base pthread zookeeper_mt uuid)