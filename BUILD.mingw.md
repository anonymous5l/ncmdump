# Windows 平台编译指南

1. 先安装 Ming-w64 + Mingw-GCC 工具链，并把 gcc.exe 所在的路径加到 PATH 变量里
2. 安装 cmake
3. 到 https://taglib.org/ 下载 taglib 的源码包，或者直接下载最新的 
   https://taglib.org/releases/taglib-1.11.1.tar.gz
4. 解压到一个目录里，例如 C:/Temp/taglib-1.11.1，然后在同级建立一个目录 C:/Temp/build
5. 打开命令行，cd 到 C:/Temp/build 里，然后执行以下命令编译 taglib 库

```
cmake -G "MinGW Makefiles" ..\taglib-1.11.1 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=C:/Temp/taglib
make -j4
make install
```

6. 下载 ncmdump，比如到 C:/Temp/ncmdump，修改Makefile里的TAGLIB_PATH，然后 `make -f Makefile.mingw` 就 OK了。

