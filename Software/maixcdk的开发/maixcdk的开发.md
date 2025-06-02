# maixcdk的开发

## 一、准备环境

```
sudo apt update
sudo apt install git cmake build-essential python3 python3-pip autoconf automake libtool
cmake --version # cmake 版本应该 >= 3.13
```

* 拉源码

* 安装依赖

```
cd MaixCDK
pip install -U pip                     # 更新 pip 到最新版本
pip install -U -r requirements.txt     # 安装依赖

```

这里注意添加到环境变量

* 编译：

```
cd examples/hello_world
maixcdk menuconfig
maixcdk build
```

可能出现编译脚本试图调用python,但是系统中未正确配置python（系统只是安装了python3,但是编译脚本直接调用python）

```
# 检查 Python 3 是否安装
python3 --version  # 应返回版本号（如 Python 3.10.12）

# 如果未安装，执行：
sudo apt update
sudo apt install python3
```

创建python到python3的软链接

```
# 检查是否已有 python 命令
which python || echo "python not found"

# 创建软链接（需管理员权限）
sudo ln -s /usr/bin/python3 /usr/bin/python

# 验证
python --version  # 应显示 Python 3.x
```

然后再重新编译

```
maixcdk build
```

* 上传程序到设备

```
scp -r dist/ root@10.127.117.1:/root/
scp -r build/dl_lib/ root@10.127.117.1:/root/
```

* 运行

MaixVision 连接设备

ssh连接设备

```
ssh root@192.168.0.123
```

```
cd /root/dist/camera_display_release
./camera_display
```

## 二、修改开发的必要固件

在构建的时候会报Downloading ax620e_msp_arm64_glibc_v3.0.0_20241120230136.tar.xz这个下载失败的问题，然后，你去官网上也找不到这个的文件，解决方法就是，修改MaixCDK/tools/cmake/file_downloader.py 文件，跳过了 ax620e_msp 的下载（虽然听起来很唐，但是没办法，还没找到一个好的顶替方案）

上传的代码里面有修改好的

## 三、修改了maix的底层pwm

为了批量处理pwm的速度，直接修改了components下面的peripheral下面的maix_pwm.cpp与maix_pwm.hpp的代码

maix_pwm.cpp（port下面的maixcam下面的maix_pwm.cpp）

maix_pwm.hpp（include下面）

## 四、maixcdk的自启动

在项目下构建好app.yaml，比如这样

```
id: dart                         # 唯一 ID，使用小写字母并用下划线分隔单词
name: dart
name[zh]: 飞镖                  # 中文名称
version: 1.9.3                     # 版本号，格式为 major.minor.patch
icon:            # 图标文件，可以是 png 或 lottie json 文件，或为空
author: kid-king-x
desc: dart begin
desc[zh]: 飞镖启动涵道

#### 包含文件方法 1：
# 默认情况下，会包含项目目录中的所有文件，除了排除文件
exclude:       # 不支持正则表达式，.git 和 __pycache__ 总是会被排除
  - .vscode
  - compile
  - build
  - dist
```

然后命令行

```
maixcdk release
```

打包完成后，跟着dist文件一起将打包好的传输过去

然后，有个比较难绷的来了，他们家的安装文件的自带的命令行，我尝试过很多的方法都不能用，最后，最好用的就是打开他们家的ide

然后里面tools里面有个install_app.py

用那个安装你打包好的app，接着就是使用set_autostart.py去设置自启动，虽然听起来有点唐，但确实可以用
