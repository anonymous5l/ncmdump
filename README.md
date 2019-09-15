Netease Cloud Music Copyright Protection File Dump
===========

## 重要通知!!!
尽量不要升级网易云，新版本貌似不会写入封面数据和音乐信息至ncm文件了。

## 简介
因为最近买了个WALKMAN 发现网易云音乐有的音乐下载的文件是ncm受保护的文件 没法放到里面听 所以这个工具诞生了 用于将ncm格式转回原有的格式. 自动设置封面以及标题作者专辑等信息. 请勿传播扩散送给有缘人. 

代码用C++重写了 兼容了更复杂奇怪的结构 去除了 `openssl` 依赖库 本人不会C++多多包涵

## 便捷式传送门

Windows GUI版本 [ncmdump-gui](https://github.com/anonymous5l/ncmdump-gui)

Windows GUI应用程序 [ncmdump-gui-release](https://github.com/anonymous5l/ncmdump-gui/releases/tag/fully) 运行库基于 `.NetFramework 4.6.1` 

Android版本 [DroidNCM](https://github.com/bunnyblueair/DroidNCM)

## 依赖库
	* taglib

## MSBuild（MSVC）编译
	参见[如何使用MSBuild编译](msbuild/如何使用MSBuild编译.md)
	已经添加相应的.gitignore

## 使用
	ncmdump [files]...

## 微信捐赠

![wechat-pay](https://raw.githubusercontent.com/anonymous5l/static-resoures/master/wechat-pay.png?raw=true)
