# FileTransferKit

`FileTransferKit` 是一个基于 Qt 的 FTP/SFTP 统一文件传输封装。它把 FTP 和 SFTP 的上传、下载、删除操作封装成同一套同步接口，并支持远程文件名按 `UTF-8` 或 `GBK` 编码传输。

当前版本适合 Qt 5 项目使用，支持 qmake 和 CMake 构建。

## 功能特性

- 统一 FTP/SFTP 调用接口。
- 支持上传、下载、删除文件，以及远程目录列表、创建、删除、重命名和存在判断。
- 支持远程文件名编码：
  - `UTF-8`
  - `GBK`
  - 自定义 `QTextCodec` 名称。
- 同步 `bool` 返回，失败原因通过 `errorString()` 获取。
- 支持进度、连接成功、操作完成信号。
- 可编译为静态库，方便其他项目只包含头文件并链接库文件使用。
- 内置命令行测试程序 `filetransferclienttest`。

## 目录结构

```text
FileTransferKit
├── 3rdparty
│   ├── qftp              # Qt 旧版 QFtp 源码，用于 FTP
│   └── qsftp             # 已补充文件名编码支持的 QSftp，用于 SFTP
├── examples
│   └── filetransferclienttest
├── src
│   ├── FileTransferClient.h
│   └── FileTransferClient.cpp
├── CMakeLists.txt
├── FileTransferKit.pro
├── FileTransferKit.pri
└── FileTransferKitSources.pri
```

## 测试环境

当前验证过的服务端环境如下：

| 协议 | 示例地址 | 服务端软件 | 示例用户名 | 示例密码 | 文件名编码 |
| --- | --- | --- | --- | --- | --- |
| FTP | `ftp.example.com` | FileZilla Server | `your_user` | `your_password` | `GBK` |
| SFTP | `sftp-gbk.example.com` | freeFTPd | `your_user` | `your_password` | `GBK` |
| SFTP | `sftp-utf8.example.com` | freeSSHd | `your_user` | `your_password` | `UTF-8` |

说明：

- FTP 默认端口是 `21`。
- SFTP 默认端口是 `22`。
- 如果服务端使用了非默认端口，可以在测试程序参数里显式传入端口。

## API 用法

头文件：

```cpp
#include "FileTransferClient.h"
```

上传文件示例：

```cpp
FileTransferClient client;
client.setConnectionInfo(
    FileTransferClient::Sftp,
    "sftp-gbk.example.com",
    22,
    "your_user",
    "your_password"
);
client.setFileNameEncoding(FileTransferClient::GbkFileNameEncoding);

if (!client.uploadFile("D:/test/中文.txt", "中文.txt")) {
    qWarning() << client.errorString();
}
```

可用接口：

```cpp
bool uploadFile(const QString &localPath, const QString &remotePath);
bool downloadFile(const QString &remotePath, const QString &localPath);
bool removeFile(const QString &remotePath);
QList<FileTransferEntry> listDirectory(const QString &remotePath);
bool createDirectory(const QString &remotePath);
bool removeDirectory(const QString &remotePath);
bool rename(const QString &oldRemotePath, const QString &newRemotePath);
bool fileExists(const QString &remotePath);
void close();
QString errorString() const;
```

目录条目结构：

```cpp
struct FileTransferEntry {
    QString name;
    QString path;
    bool isDirectory;
    qint64 size; // 未提供大小时为 -1
};
```

目录管理示例：

```cpp
if (!client.createDirectory("upload")) {
    qWarning() << client.errorString();
}

const QList<FileTransferEntry> entries = client.listDirectory("upload");
if (!client.errorString().isEmpty()) {
    qWarning() << client.errorString();
}

if (client.fileExists("upload/report.txt")) {
    client.rename("upload/report.txt", "upload/report-old.txt");
}
```

连接配置：

```cpp
enum Protocol {
    Ftp,
    Sftp
};

enum FileNameEncoding {
    Utf8FileNameEncoding,
    GbkFileNameEncoding
};

void setConnectionInfo(
    Protocol protocol,
    const QString &host,
    quint16 port,
    const QString &userName,
    const QString &password
);

void setFileNameEncoding(FileNameEncoding encoding);
void setFileNameCodecName(const QByteArray &codecName);
void setTimeoutMs(int timeoutMs);
```

信号：

```cpp
void progress(qint64 done, qint64 total);
void connected();
void operationFinished(bool ok, const QString &error);
```

路径规则：

- `localPath` 是本地文件完整路径或当前进程工作目录下的相对路径。
- `remotePath` 是服务端上的完整远程路径或相对路径。
- 不自动拼接文件名，调用方传什么远程路径，就上传到什么远程路径。
- 上传默认覆盖已有文件。

## 测试程序

测试程序位置：

```text
bin/filetransferclienttest
bin/filetransferclienttest.exe
```

参数格式：

```text
filetransferclienttest <ftp|sftp> <host> <user> <password> <utf8|gbk> <local-file> <remote-file> [port] [--remove]
filetransferclienttest manage <ftp|sftp> <host> <user> <password> <utf8|gbk> <local-file> <remote-directory> [port]
filetransferclienttest cleanup <ftp|sftp> <host> <user> <password> <utf8|gbk> <remote-directory> [port]
```

参数说明：

| 参数 | 说明 |
| --- | --- |
| `<ftp|sftp>` | 使用 FTP 或 SFTP |
| `<host>` | 服务端 IP 或域名 |
| `<user>` | 用户名 |
| `<password>` | 密码 |
| `<utf8|gbk>` | 远程文件名编码 |
| `<local-file>` | 本地待上传文件 |
| `<remote-file>` | 上传后的远程文件路径或文件名 |
| `[port]` | 可选端口；FTP 默认 `21`，SFTP 默认 `22` |
| `[--remove]` | 可选；上传并下载校验成功后删除远程文件 |

测试程序执行流程：

1. 上传 `<local-file>` 到 `<remote-file>`。
2. 再把 `<remote-file>` 下载到本地临时目录。
3. 比较下载文件和本地源文件内容是否一致。
4. 默认保留远程文件，方便测试者到服务端确认。
5. 如果传入 `--remove`，校验成功后删除远程文件。

`manage` 模式用于回归测试 v0.2 的远程文件管理接口。它会在指定的空目录中依次执行：

1. 创建目录并验证空目录列表。
2. 上传一个中文文件名的测试文件。
3. 列出目录并检查文件存在。
4. 重命名文件，再次检查文件存在。
5. 删除文件和目录。

测试成功后会自动清理创建的远程目录和文件。`remote-directory` 必须是一个不存在的目录。

`cleanup` 模式用于删除指定测试目录下的一级文件及目录本身，不会递归删除子目录。

成功输出示例：

```text
Uploading...
Progress: 0/5780
Progress: 5780/5780
Downloading...
Progress: 0/5780
Progress: 5780/5780
Remote file kept: 测试2.txt
OK: 测试2.txt
```

## 测试命令示例

Windows 示例：

```bat
bin\filetransferclienttest.exe ftp ftp.example.com your_user your_password gbk README.md 统一封装_ftp_gbk.txt
bin\filetransferclienttest.exe sftp sftp-gbk.example.com your_user your_password gbk README.md 统一封装_sftp_gbk.txt
bin\filetransferclienttest.exe sftp sftp-utf8.example.com your_user your_password utf8 README.md 统一封装_sftp_utf8.txt
```

Linux 示例：

```sh
./bin/filetransferclienttest ftp ftp.example.com your_user your_password gbk README.md 统一封装_ftp_gbk.txt
./bin/filetransferclienttest sftp sftp-gbk.example.com your_user your_password gbk README.md 统一封装_sftp_gbk.txt
./bin/filetransferclienttest sftp sftp-utf8.example.com your_user your_password utf8 README.md 统一封装_sftp_utf8.txt
```

目录管理回归测试：

```sh
./bin/filetransferclienttest manage ftp ftp.example.com your_user your_password gbk README.md ftk_manage_ftp
./bin/filetransferclienttest manage sftp sftp-gbk.example.com your_user your_password gbk README.md ftk_manage_sftp_gbk
./bin/filetransferclienttest manage sftp sftp-utf8.example.com your_user your_password utf8 README.md 测试目录_sftp_utf8
```

指定端口示例：

```sh
./bin/filetransferclienttest sftp sftp-gbk.example.com your_user your_password gbk 测试.txt 测试2.txt 2222
```

测试完成后删除远程文件：

```sh
./bin/filetransferclienttest ftp ftp.example.com your_user your_password gbk 测试.txt 测试2.txt --remove
```

指定端口并删除远程文件：

```sh
./bin/filetransferclienttest sftp sftp-gbk.example.com your_user your_password gbk 测试.txt 测试2.txt 2222 --remove
```

## Windows qmake 构建

示例环境：

- Visual Studio 2019
- Qt 5.9 MSVC 64-bit
- Visual Studio 安装路径：`D:\Microsoft Visual Studio`

从仓库根目录执行：

```bat
cmd /c "call ""D:\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"" x64 && if not exist build-filetransferkit mkdir build-filetransferkit && cd build-filetransferkit && D:\Qt\Qt5.9.0\5.9\msvc2015_64\bin\qmake.exe ..\FileTransferKit.pro && nmake"
```

qmake 构建输出：

```text
lib\FileTransferKit.lib
bin\filetransferclienttest.exe
3rdparty\qsftp\lib\QSsh.dll
3rdparty\qsftp\lib\QSsh.lib
```

运行测试程序前设置 `PATH`：

```bat
set PATH=%CD%\3rdparty\qsftp\lib;D:\Qt\Qt5.9.0\5.9\msvc2015_64\bin;%PATH%
```

## Ubuntu 18.04 CMake 构建

安装依赖：

```sh
sudo apt update
sudo apt install -y build-essential cmake qtbase5-dev
```

从仓库根目录执行：

```sh
cd /path/to/FileTransferKit
cmake -S . -B build-filetransferkit-cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build-filetransferkit-cmake -- -j$(nproc)
```

CMake 构建输出：

```text
lib/libFileTransferKit.a
lib/libQSsh.so
bin/filetransferclienttest
```

运行测试程序前设置动态库路径：

```sh
export LD_LIBRARY_PATH=$PWD/lib:$LD_LIBRARY_PATH
```

如果 Qt 安装在自定义路径，配置时传入 `CMAKE_PREFIX_PATH`：

```sh
cmake -S . -B build-filetransferkit-cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=/opt/Qt/5.9.0/5.9/gcc_64
```

## 在 qmake 项目中使用

假设你的项目和 `FileTransferKit` 目录并列：

```text
YourProject
FileTransferKit
```

在你的 `.pro` 文件中加入：

```qmake
FILETRANSFERKIT_ROOT = $$PWD/../FileTransferKit
include($$FILETRANSFERKIT_ROOT/FileTransferKit.pri)
```

业务代码中只需要包含：

```cpp
#include "FileTransferClient.h"
```

Windows 运行时需要确保 `QSsh.dll` 所在目录在 `PATH` 中：

```bat
3rdparty\qsftp\lib
```

## 在 CMake 项目中使用

先编译 `FileTransferKit`，然后在你的项目中链接生成的静态库和 QSsh 动态库：

```cmake
find_package(Qt5 REQUIRED COMPONENTS Core Network Gui Widgets)

add_executable(myapp main.cpp)

target_include_directories(myapp PRIVATE
    /path/to/FileTransferKit/src
    /path/to/FileTransferKit/3rdparty/qsftp/include
)

target_link_directories(myapp PRIVATE
    /path/to/FileTransferKit/lib
)

target_link_libraries(myapp PRIVATE
    FileTransferKit
    QSsh
    Qt5::Core
    Qt5::Network
    Qt5::Gui
    Qt5::Widgets
)
```

Linux 运行时需要确保 `libQSsh.so` 可被找到：

```sh
export LD_LIBRARY_PATH=/path/to/FileTransferKit/lib:$LD_LIBRARY_PATH
```

## 设计说明

- FTP 适配层直接使用 `QFtp`，不依赖历史封装工具类。
- SFTP 适配层直接使用 `QSsh::SshConnection` 和 `QSsh::SftpChannel`。
- FTP 文件名编码通过 `QTextCodec` 转换为目标编码字节，再以 `QString::fromLatin1()` 传给底层 `QFtp`。
- SFTP 文件名编码通过补充后的 `setFileNameEncoding()` / `setFileNameCodecName()` 传给底层 QSftp。
- 同一个 `FileTransferClient` 实例会复用 FTP 已登录连接，适合连续的上传、目录列表和重命名操作；调用 `close()` 或析构时断开。异常或超时时会强制中断并在下一次操作时重连。
- SFTP 每次操作会单独建立 SSH/SFTP 会话，操作完成后关闭连接。

## 注意事项

- 如果服务端文件名编码是 GBK，测试程序和业务代码都要选择 `gbk` / `GbkFileNameEncoding`。
- 如果服务端文件名编码是 UTF-8，选择 `utf8` / `Utf8FileNameEncoding`。
- 远程路径是否支持中文，最终取决于服务端实际使用的文件名编码和操作系统配置。
- `listDirectory()` 成功但目录为空时，返回空列表且 `errorString()` 为空；列表失败时也会返回空列表，但 `errorString()` 会包含错误原因。
- `fileExists()` 通过列出父目录后匹配文件名实现，以兼容部分不支持 SFTP `LSTAT` 的服务端。
- 某些 FTP 服务端会用 UTF-8 返回 `LIST` 结果，即使命令参数采用 GBK。本库会优先识别有效 UTF-8 列表响应，再回退到配置的文件名编码。
- Windows 命令行显示中文可能受代码页影响，但不代表传输失败，以测试程序最终的 `OK` 结果为准。
