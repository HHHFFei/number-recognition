# 载入固件
1. 下载arduino
2. 安装驱动程序
	Windows系统中，你需要为Arduino安装驱动配置文件，才可正常驱动Arduino。
	打开arduino文件夹——>driver
	运行dpinst-amd64.exe（32位系统运行dpinst-x86.exe）
3. 设备驱动器——>其他设备——>CP2102...——>在arduino文件夹搜索驱动安装

4. 用arduino打开固件文件
	[esp32-cam使用教程[Arduino IDE开发]_哔哩哔哩_bilibili](https://www.bilibili.com/video/BV1Lp4y1W7M3)

5. 配置ESP32-CAM环境
	[arduino-esp32-cam环境配置](https://blog.csdn.net/akk41397/article/details/106419396)
	文件——>首选项——>附加开发板管理器
	添加网址：https://dl.espressif.com/dl/package_esp32_index.json
	
	工具——>开发板——>开发板管理器
	搜索ESP32，安装

	工具→端口
	选择对应端口

6. 上传程序。
	确认接线正确后点击上传按钮。编译结束后开始上传程序。

7. 运行固件
	打开串口监视器，断开GND和IO0接线，按下复位按键
	等待串口返回网址
	
8. 查看监控
	与开发板在同一局域网中的设备可在浏览器中输入串口返回的网址启动视频服务。
	在网站界面最下方点击“Start Stream”开始传输视频流

---
# 固件修改
根据自己的实际需要，可对代码进行修改

## 1. 删除OV3660
若不使用3660摄像头，可删除一下运行OV3660代码
1. app_httpd.cpp中
```C++
 if (s->id.PID == OV3660_PID) {
	 return httpd_resp_send(req, (const char *)index_ov3660_html_gz, index_ov3660_html_gz_len);
 }
```
2. camera_index.h中
```html
//File: index_ov3660.html.gz, Size: 4408
#define index_ov3660_html_gz_len 4408
const uint8_t index_ov3660_html_gz[] = {
......
......
};
```
> OV2640同理

## 2. 将ESP32作为热点软连接
第130行
```
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
```
改为
```
  WiFi.softAP(ssid, password);
```
> 此时之前输入的名称和密码为该热点的名称和密码

## 3. 将camera_index.h文件解压为html
html文件传到浏览器过程中是可以先压缩为GZIP格式的文件再传输的，压缩文件传输到浏览器后，会自动解压后再渲染出来，这样可以大大减少网络传输的压力。
若需要查看或修改网页，可以利用[Split, From Hex, Gunzip - CyberChef (gchq.github.io)](https://gchq.github.io/CyberChef/#recipe=Split(',0x','0x')From_Hex('0x')Gunzip())网站工具将camera_index.h中的十六进制代码转化为html语言。
> 输入的十六进制代码不应当含有空格和换行，否则会转换失败


---
# 可能的问题
## 1. 连接失败
可能是由于上传代码时IO0没有接地。

## 2. 串口监视器输出乱码
串口监视器选择115200波特率

## 3. 串口输出省略号
若串口一直输出省略号
`....................................`
而不输出网址，可能是由于开发板未连接上对应网络。
检查网络是否开启，网络名称、密码是否有误

## 4. 无法进行人脸检测
将app_httpd.cpp中286行开始的四个free改成dl_lib_free，第385行开始的四个free改成dl_lib_free
```
        draw_face_boxes(image_matrix, net_boxes, face_id);
        free(net_boxes->score);
        free(net_boxes->box);
        free(net_boxes->landmark);
        free(net_boxes);
```
改为
```
        draw_face_boxes(image_matrix, net_boxes, face_id);
        dl_lib_free(net_boxes->score);
        dl_lib_free(net_boxes->box);
        dl_lib_free(net_boxes->landmark);
        dl_lib_free(net_boxes);
```
以及第385行
```
                                dl_lib_free(net_boxes->score);
                                dl_lib_free(net_boxes->box);
                                dl_lib_free(net_boxes->landmark);
                                dl_lib_free(net_boxes);
```


---
# 数字识别
## 1. 图像预处理
### 1.1 转化为灰度图
图像灰度化的目的是为了简化矩阵，剔除对图像识别无用的色彩信息，提高运算速度。或是为图像二值化做准备。可通过以下两种方法将RGB图像转化为灰度图
1. 直接将RGB图像转化为灰度图
	适用于数字与背景亮度相差较大的图像
```python
gray_img = cv.cvtColor(img, cv.COLOR_RGB2GRAY)
```
2. 将RGB图像中某一通道提取出来作为灰度图
	适用于数字与背景颜色相差较大的图像
```python
# 将绿色通道图像转化为灰度图
a,gray_img,b=  cv.split(img)
```

### 1.2 高斯滤波
高斯滤波的目的是消除图像中的噪声，避免噪声对数字识别造成干扰
```python
gauss_img = cv.GaussianBlur(gray_img, (5, 5), 0)
```

### 1.3 自适应均衡化
自适应均衡化的目的是提高图像的对比度，使数字的辨识度更高
```python
# clahe = cv.createCLAHE(clipLimit=2.0, tileGridSize=(8, 8))
# clahe_img = clahe.apply(gauss_img)
# bf.cv_show('gray_img && gauss_img', np.hstack((gray_img, gauss_img)))
```

### 1.4 二值化
二值化是图像变得简单，数据量减小，有利于计算机对图像进一步处理。同时还能突出ROI
```python
_, binary_img = cv.threshold(gauss_img, 0, 255, cv.THRESH_OTSU | cv.THRESH_BINARY)
```
### 1.5 透视变换


### 1.6* 膨胀
对于类似于数码管的数字，数字本身并不是一个连通图，这会程序无法通过轮廓正确识别数字范围。利用膨胀可以将一个数字的各个部分连通为一个整体。
> 对于白色的数字，应该使用膨胀使其各部分相连。若数字是黑色，则应该使用腐蚀
> 注意，过度膨胀可能会导致相邻数字相连，无法分割

```c++
    Mat dilateImage;
    Mat element = getStructuringElement(MORPH_RECT, Size(10, 10));                      
    dilate(binaryImage, dilateImage, element);
```

## 2. 识别数字
### 2.1 识别轮廓
识别二值化图像中的轮廓
```python
contours, hierachy = cv.findContours(morph_close_1, cv.RETR_EXTERNAL, cv.CHAIN_APPROX_SIMPLE)
```
### 2.2 定位轮廓位置

### 2.3 识别轮廓中的数字
以下是我归纳的三种数字识别的方法，三种方法的适用范围不同，读者可自行选用合适的方法
1. 穿线法
	对于八段数码管式数字，可以通过判断每个数码管区域是否被点亮，进而判断出数字。
```
int TubeIdentification(Mat inputmat) // 穿线法判断数码管a、b、c、d、e、f、g、  
{  
    int tube = 0;  
    int tubo_roi[7][4] =  
    {  
        { inputmat.rows * 0 / 3, inputmat.rows * 1 / 3, inputmat.cols * 1 / 2, inputmat.cols * 1 / 2 }, // a  
        { inputmat.rows * 1 / 3, inputmat.rows * 1 / 3, inputmat.cols * 2 / 3, inputmat.cols - 1     }, // b  
        { inputmat.rows * 2 / 3, inputmat.rows * 2 / 3, inputmat.cols * 2 / 3, inputmat.cols - 1     }, // c  
        { inputmat.rows * 2 / 3, inputmat.rows - 1    , inputmat.cols * 1 / 2, inputmat.cols * 1 / 2 }, // d  
        { inputmat.rows * 2 / 3, inputmat.rows * 2 / 3, inputmat.cols * 0 / 3, inputmat.cols * 1 / 3 }, // e  
        { inputmat.rows * 1 / 3, inputmat.rows * 1 / 3, inputmat.cols * 0 / 3, inputmat.cols * 1 / 3 }, // f  
        { inputmat.rows * 1 / 3, inputmat.rows * 2 / 3, inputmat.cols * 1 / 2, inputmat.cols * 1 / 2 }, // g  
    };  
  
    if (inputmat.rows / inputmat.cols > 2)   // 1 is special, which is much narrower than others  
    {  
        tube = 6;  
    }  
    else  
    {  
        for (int i = 0; i < 7; i++)  
        {  
  
            if (Iswhite(inputmat, tubo_roi[i][0] , tubo_roi[i][1], tubo_roi[i][2], tubo_roi[i][3]))  
                tube = tube + (int)pow(2, i);  
        }  
    }  
  
    switch (tube)  
    {  
        case  63: return 0;  break;  
        case   6: return 1;  break;  
        case  91: return 2;  break;  
        case  79: return 3;  break;  
        case 102: return 4;  break;  
        case 109: return 5;  break;  
        case 125: return 6;  break;  
        case   7: return 7;  break;  
        case 127: return 8;  break;  
        case 111: return 9;  break;  
  
        default: return -1;  
    }  
}
```
2. 模板匹配

2. 神经网络


---
# 补充
## 1. 引脚
VCC：模拟信号电源，电路供电电压
VDD：数字信号电源，芯片供电电压
**5V**：5V供电引脚
**3V3**：3.3V供电引脚
**VCC**：输出电源引脚。默认输出3.3V。若要输出5V电压，需要将VCC引脚旁的焊盘中3.3V连接线断开并焊接5V焊盘
**U0R、U0T**：（GPIO3、GPIO1）用于上传代码并与上位机通信。上传代码后可以连接其他外围设备（输出或传感器），但是将无法打开串行监视器
**IO0**：（GPIO0）用于确定开发板是否处于闪烁模式，改引脚内部连接至10k欧姆电阻。当GPIO0连接至GND时，ESP32进入闪烁模式，此时可以将代码上传至开发板上。ESP32正常运行之前，需要将GPIO0与GND断开。


```C++
#define PWDN_GPIO_NUM  32  
#define RESET_GPIO_NUM -1  // 重置
#define XCLK_GPIO_NUM  0  // GPIO 22：时钟
#define SIOD_GPIO_NUM  26  // GPIO 26：SDA
#define SIOC_GPIO_NUM  27  // GPIO 27：SCL
#define Y9_GPIO_NUM    35  // GPIO 35：D7
#define Y8_GPIO_NUM    34  // GPIO 34：D6
#define Y7_GPIO_NUM    39  // GPIO 39：D5
#define Y6_GPIO_NUM    36  // GPIO 36：D4
#define Y5_GPIO_NUM    21  // GPIO 21：D3
#define Y4_GPIO_NUM    19  // GPIO 19：D2
#define Y3_GPIO_NUM    18  // GPIO 18：D1
#define Y2_GPIO_NUM    5  // GPIO 5：D0
#define VSYNC_GPIO_NUM 25  // GPIO 25：垂直同步
#define HREF_GPIO_NUM  23  // GPIO 23：HREF
#define PCLK_GPIO_NUM  22  // GPIO 22：时钟
```


---


# 参考文章

## 1. 配置
[Ai-Thinker-Open/Ai-Thinker-Open_ESP32-CAMERA_LAN: 深圳市安信可科技有限中心-摄像头局域网解决方案 (github.com)](https://github.com/Ai-Thinker-Open/Ai-Thinker-Open_ESP32-CAMERA_LAN)
[【安信可IDE 1.5模板专题1】安信可windows一体化环境IDE V1.5 版本降临，体积更小，兼容新旧版本SDK编译_安信可科技 -CSDN博客_安信可ide](https://blog.csdn.net/Boantong_/article/details/106229281)
[开始创建工程 - ESP32 - — ESP-IDF 编程指南 latest 文档 (espressif.com)](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/start-project.html#id7)
[开发工具清单 | 安信可科技 (ai-thinker.com)](https://docs.ai-thinker.com/tools)
[Arduino驱动的安装 (dfrobot.com.cn)](https://wiki.dfrobot.com.cn/Arduino%E9%A9%B1%E5%8A%A8%E7%9A%84%E5%AE%89%E8%A3%85)
[ESP32CAM 介绍及刷机_bilibili](https://www.bilibili.com/video/BV13y4y1D7u2/?spm_id_from=333.788.recommend_more_video.4)
[Arduino教程——手动添加库并使用-Arduino中文社区 - Powered by Discuz!](https://www.arduino.cn/thread-31720-1-1.html)
[VS2019+OpenCV安装与配置教程-CSDN博客](https://blog.csdn.net/Creama_/article/details/107238475)

## 2. 代码分析
[ESP32-CAM源码分析 (eet-china.com)](https://www.eet-china.com/mp/a37631.html)
[Esp32Cam WebServer 网页源代码查看与编辑 - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/456387183)
[ESP32-CAM AI-Thinker引脚指南：GPIO使用说明 – 趣讨教 (qutaojiao.com)](https://www.qutaojiao.com/24272.html)
[ESP32-CAM + micropython学习笔记 - 代码先锋网 (codeleading.com)](https://www.codeleading.com/article/57156204931/)
[自行编译micropython固件刷入ESP32 cam，并测试拍照及图传 - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/406128630)
[ESP32 摄像头：人脸检测 - 技术教程 (techtutorialsx.com)](https://techtutorialsx.com/2020/06/13/esp32-camera-face-detection/)

## 2. 数字识别
[C++调用Asprise OCR识别图片 - reyzal - 博客园 (cnblogs.com)](https://www.cnblogs.com/Reyzal/p/5022690.html)
[数字仪表识别之基于轮廓定位数字并提取 - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/99536516)
[数码管OCR数字识别_bilibili](https://www.bilibili.com/video/BV1vb4y1i7L5?from=search&seid=4607415289898485298&spm_id_from=333.337.0.0)
[图像抄表_开发者文档_OneNET (10086.cn)](https://open.iot.10086.cn/doc/ai/book/ImageIdentification/readMeter.html)
[stm32-camera: 通过移远EC600S和ESP32-CAM来实现远程拍照抄表的功能。这套方案针对的是对现有传统燃气表、水表和其他仪表的智能化改造，在不需要更换表的前提下实现远程抄表功能。 (gitee.com)](https://gitee.com/lantsang/stm32-camera?_from=gitee_search)
[OpenCV——识别印刷体数字 - Not-Bad - 博客园 (cnblogs.com)](https://www.cnblogs.com/farewell-farewell/p/5887068.html)
[基于OpenCV的数码管数字识别 - 代码天地 (codetd.com)](https://www.codetd.com/article/3826735)
[彩色数字仪表表盘读数的自动识别系统-【维普期刊官网】- 中文期刊服务平台 (cqvip.com)](https://ccd1.cqvip.com/Qikan/Article/Detail?id=24411022)
[基于OpenCV和LSSVM的数字仪表读数自动识别-AET-电子技术应用 (chinaaet.com)](http://m.chinaaet.com/article/3000060505)

## 4. 改进
[(esp32cam 人脸检测死机解决_bilibili](https://www.bilibili.com/video/BV1Qq4y1b7Ta?from=search&seid=14645504330297200451&spm_id_from=333.337.0.0)
[ESP32cam软连接方案_bilibili](https://www.bilibili.com/video/BV1jS4y167LK#reply104285598400)
[ESP32-Cam 智能抄表_bilibili](https://www.bilibili.com/video/BV1aL4y1J7Ux/?spm_id_from=333.788.recommend_more_video.18)
[ESP-32抄表固件 (github.com)](https://github.com/jomjol/AI-on-the-edge-device)
[带有 OpenCV.js 的 ESP32-CAM Web 服务器：颜色识别和跟踪 – 趣讨教 (qutaojiao.com)](https://www.qutaojiao.com/24507.html)
[巴法云 mixly (bemfa.com)](https://bemfa.com/m/c.html)
[ESP32加Python实现无线视频传输，技术宅开源_哔哩哔哩_bilibili](https://www.bilibili.com/video/BV1da4y1W7dn/)
[ESP32 CAM 多相机监控系统_bilibili](https://www.bilibili.com/video/BV1MT4y1M7pR/?spm_id_from=333.788.recommend_more_video.9)

# 待阅读文章
[【Node-Red】图形化编程_bilibili](https://www.bilibili.com/video/BV1Qt411E7uo?from=search&seid=13794671124095217023&spm_id_from=333.337.0.0)
[(13条消息) ESP32-IDF CAMERA OpenCV移植 研究 【doing】_knowform的博客-CSDN博客_esp32 opencv](https://blog.csdn.net/forest_world/article/details/116380605?ops_request_misc=%257B%2522request%255Fid%2522%253A%2522165002768916780357233329%2522%252C%2522scm%2522%253A%252220140713.130102334.pc%255Fall.%2522%257D&request_id=165002768916780357233329&biz_id=0&spm=1018.2226.3001.4187)
[(13条消息) ESP32 开发笔记（十）使用 ESP32+Camera 二维码识别_InfiniteYuan的博客-CSDN博客_esp32二维码识别](https://infiniteyuan.blog.csdn.net/article/details/88553507)
[esp32-camera-qr-recoginize/examples/single_chip/qrcode_recoginize at master ·InfiniteYuan/esp32-camera-qr-recoginize ·GitHub](https://github.com/InfiniteYuan/esp32-camera-qr-recoginize/tree/master/examples/single_chip/qrcode_recoginize)
[GitHub - dlbeer/quirc： QR decoder library](https://github.com/dlbeer/quirc)
[Get Started - ESP32 - — ESP-IDF Programming Guide latest documentation (espressif.com)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)

[GitHub - joachimBurket/esp32-opencv： Shrinked OpenCV for ESP32](https://github.com/joachimBurket/esp32-opencv)
OpenCV 在 ESP32 上进行交叉编译

[(13条消息) ESP32 开发笔记（十）使用 ESP32+Camera 二维码识别](https://blog.csdn.net/qq_27114397/article/details/88553507)
ESP32-CAM 二维码识别

