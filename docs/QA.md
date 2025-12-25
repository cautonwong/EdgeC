1. 这个工程化脚手架还缺点什么吧 是不是clang tidy clang format 测试覆盖率gcov? 还有什么缺失? 都补上  
2. .local下的预编译依赖是否都健全 应用能否FindPackage到
3. 创建samples目录  
3.1 将本项目依赖的项目比如cJSON等等都编写samples 给用户展示用法
3.2 给本项目的用法(比如core services等待)编写samples  给用户展示用法

cd build && cmake .. && make -j$(nproc) && ctest --output-on-failure [current working directory /workspaces/edgeos/EdgeC]  

记住 要跨平台哟  .local下有linux-x86_64和linux-arm64预编译依赖 .local是要上传到仓库里的 删除多余的.local/linux-aarch64 因此 所有的代码都要跨平台哟 CI也要跨平台 别的代码使用FindPackage从.local中找依赖