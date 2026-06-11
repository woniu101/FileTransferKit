
About QSsh(关于 QSsh)
==========

    QSsh provides SSH and SFTP support for Qt applications. The aim of this project 
    is to provide a easy way to use these protocols in any Qt application.
    This project is based on Qt Creator's libQtcSsh.so. All the credits to
    Qt Creator's team!

    QSsh为qt提供SSH和SFTP支持，使你能方便快捷的使用SSH和SFTP。
    这个项目是基于Qt Creator中的LibQtcSsh.so。十分感谢Qt Creator团队。
    基于GPL和LGPL授权。
==========

Prerequisites:(编译环境)
-------------
    Qt 5.x 

    On Windows: MinGW 4.7 or later, Visual Studio 2010 or later

    On Mac: XCode 2.5 or later, compiling on 10.4 requires setting the environment variable QTC_TIGER_COMPAT before running qmake

==========

Compiling QSsh(编译QSsh)
----------------------
    mkdir $BUILD_DIRECTORY

    cd $BUILD_DIRECTORY

    qmake $SOURCE_DIRECTORY/qssh.pro

    make (or mingw32-make or nmake depending on your platform)


###Or

    You can open "qssh.pro" used "Qt Creator".

    你也可以直接用Qt Creator打开qssh.pro。

Examples
===========
- Directory examples/ssh/  Qt-Creator下自带的例子。
 
> 注：生成的库在 lib文件夹下就可以直接用的。

SFTP 文件名编码
==============
本版本在 `QSsh::SftpChannel` 和 `examples/SecureUploader/SZRSFtpTools`
中增加了远端文件名编码配置，默认使用 UTF-8；连接中文 GBK 文件名服务器时，
可在上传/下载前切换为 GBK。

```cpp
SZRSFtpTools sftp;
sftp.setSftpInfo("192.168.18.26", "aaa", "123456");
sftp.setFileNameEncoding(SZRSFtpTools::Utf8FileNameEncoding);
sftp.uploadFile("/home/aaa", "D:/test/中文.txt");

SZRSFtpTools gbkSftp;
gbkSftp.setSftpInfo("192.168.18.166", "aaa", "123456");
gbkSftp.setFileNameEncoding(SZRSFtpTools::GbkFileNameEncoding);
gbkSftp.uploadFile("/home/aaa", "D:/test/中文.txt");
```

直接使用 QSsh 时也可以设置：

```cpp
channel->setFileNameEncoding(QSsh::SftpChannel::GbkFileNameEncoding);
```
