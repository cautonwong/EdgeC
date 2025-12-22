1. 这个工程化脚手架还缺点什么吧 是不是clang tidy clang format 测试覆盖率gcov? 还有什么缺失?
2. 有几个submodule中你增加了cmakelist 提交不到仓库中 造成github action执行失败  你要想办法解决
3. 创建samples目录  
3.1 将本项目依赖的项目比如cJSON等等都编写samples 给用户展示用法
3.2 给本项目的用法(比如core services等待)编写samples  给用户展示用法
4. error while loading shared libraries: libasan.so.6: cannot open shared object file: No such file or directory