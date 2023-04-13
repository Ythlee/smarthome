# smarthome

#### 软件架构
软件架构说明


#### 安装教程

sudo apt install make -y

sudo apt install git -y

sudo apt install gcc -y

sudo apt install g++ -y

sudo apt install sqlite3 -y

sudo apt install libsqlite3-dev -y

sudo apt install vlc -y

sudo apt install openssl

sudo apt-get install libssl-dev -y

#### 使用说明

1.  在根目录下运行make，编译所有服务器、客户端、设备端目标程序。
2.  进入output目录，执行./ServerRun.sh命令启动主服务器。
3.  进入output目录，执行./ClientRun.sh命令启动客户端。
4.  客户端添加设备时，进入output目录，执行./AirRun.sh命令启动空调设备端。
5.  客户端添加设备时，进入output目录，执行./RefRun.sh命令启动冰箱设备端。
6.  在根目录下运行./Reset.sh复位系统设置。
