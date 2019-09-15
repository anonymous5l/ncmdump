## msbuild命令行编译

前置要求：  
自备vcpkg并进行全局集成  
以Powershell为例：  
`PS> [vcpkg路径]\vcpkg.exe integrate install`

1. 通过vcpkg安装taglib等依赖项  
还是以Powershell为例：  
`PS> [vcpkg路径]\vcpkg.exe install tagilb:x64-windows taglib:x86-windows`

2. 在Visual Studio命令提示中执行`msbuild "\msbuild\ncmdump.sln" /p:Configuration="Release"`  
项目文件默认配置是Visual Studio 2019，SDK 10.0.18362，win7机器编译可以试着重定向

3. 在Build文件夹下找生成输出，不能忽略附带的dll

