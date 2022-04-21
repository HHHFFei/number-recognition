#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <time.h>
using namespace cv;
using namespace std;

// 矩形框中心点，用于比较位置
struct con {
    double x, y;                    //轮廓中心位置
    int order;                      //轮廓向量contours中的第几个

    // 比较中点坐标以定位。只比较左右位置
    bool operator<(con& m) {
        //if (y > m.y) return false;
        //else  if (y == m.y) {
        if (x < m.x) return true;
        else return false;
        //}
        // else return true;
    }
    //con operator=(con& m) {
    //    x = m.x;
    //    y = m.y;
    //    order = m.order;
    //    return *this;
    //}

}con[15];

// 从视频流中提取每一帧图像
Mat getVideoFrame(string filepath) {
    VideoCapture capture(filepath);
    Mat frame;
    capture.read(frame);			//提取视频

    // 视频信息
    int frame_width = capture.get(CAP_PROP_FRAME_WIDTH);
    int frame_height = capture.get(CAP_PROP_FRAME_HEIGHT);
    int count = capture.get(CAP_PROP_FRAME_COUNT);
    double fps = capture.get(CAP_PROP_FPS);

    //cout << "frame width:" << frame_width << std::endl;
    //cout << "frame height:" << frame_height << std::endl;
    //cout << "FPS:" << fps << std::endl;
    //cout << "Number of Frames:" << count << std::endl << std::endl;
    return frame;

}

// 保存匹配模板的结果。bi：匹配度。num：匹配的数字
struct result {
    double bi;
    int num;

    bool operator<(result& m) {
        if (bi < m.bi)return true;
        else return false;
    }
}result[15];

Mat num[15];
Mat sample;

void myImshow(Mat& srcImage, string text);
void myConversion(Mat& srcImage, Mat& sample, int i);
void myResize(Mat& srcImage, Mat& binaryImage);
void myRecognition(Mat& srcImage, Mat& binaryImage);
int deal(Mat& src, int order);
void Threshold(Mat& src, Mat& sample, int m);
double compare(Mat& src, Mat& sample);
void myRelocation(vector<Point2f>& corner);
bool Contour_Area(vector<Point> contour1, vector<Point> contour2);


void videoTest() {
    Mat srcImage = getVideoFrame("http://192.168.43.143:81/stream");
    while (true) {
        Mat srcImage = getVideoFrame("http://192.168.43.143:81/stream");
        //imshow("test", srcImage);
        //waitKey(0);

        Mat dilateImage;
        myConversion(srcImage, dilateImage, 1);

        // 定义轮廓和层次结构
        vector<vector<Point>> contours;                                                     // 点的集合构成轮廓，contours用于保存轮廓的集合
        vector<Vec4i> hierarchy;                                                            // 4维int向量，用于保存线段两个端点的坐标
        // 识别轮廓
        findContours(dilateImage, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE);   // 边界识别（二值图像，边界，边界索引，），定位的数字的外轮廓
        int index = 0;
        // 绘制轮廓
        // 备份原图像，用于作图
        Mat dstImage;
        srcImage.copyTo(dstImage);                                                          // 把srcImage的内容复制粘贴到dstImage上
        for (; index >= 0; index = hierarchy[index][0])
        {
            drawContours(dstImage, contours, index, (255, 0, 0), 3, (255, 0, 0), hierarchy);  // 绘制边界
        }

        int i = 0;
        Point2f pp[5][4];                                                                   // Point2f为点的浮点数坐标
        vector<vector<Point>>::iterator It;
        Rect rect[10];                                                                      // opencv的矩形类，（左上角x坐标，左上角y坐标，右下角x坐标，右下角y坐标）
        for (It = contours.begin(); It < contours.end(); It++) {                            // 使用contours迭代器遍历每一个轮廓，找到并画出包围这个轮廓的最小矩阵。
            Point2f vertex[4];                                                              // 四个元素的浮点坐标组，用于保存矩形的四个角坐标
            rect[i] = boundingRect(*It);                                                    // 计算轮廓的垂直边界的最小矩形，并赋值给rect[i]
            vertex[0] = rect[i].tl();                                                       // 矩阵左上角的点坐标赋值给vertex[0]
            vertex[1].x = (float)rect[i].tl().x, vertex[1].y = (float)rect[i].br().y;       // 矩阵左下方的点
            vertex[2] = rect[i].br();                                                       // 矩阵右下角的点坐标赋值给vertex[2]
            vertex[3].x = (float)rect[i].br().x, vertex[3].y = (float)rect[i].tl().y;       // 矩阵右上方的点

            for (int j = 0; j < 4; j++)
                line(dstImage, vertex[j], vertex[(j + 1) % 4], Scalar(0, 0, 255), 10);      // 绘制矩形，每个循环分别绘制矩形的左、下、右、上边

            con[i].x = (vertex[0].x + vertex[1].x + vertex[2].x + vertex[3].x) / 4.0;       // 计算矩形中心点的x坐标
            con[i].y = (vertex[0].y + vertex[1].y + vertex[2].y + vertex[3].y) / 4.0;       // 计算矩形中心点的y坐标
            con[i].order = i;                                                               // 记录中心点的编号
            i++;
            //cout << rect[i].area() << endl;                                                 // test
        }
        sort(con, con + i);

        int deal_result[4];

        // 模板匹配
        for (int j = 0; j < i; j++) {
            int k = con[j].order;
            srcImage(rect[k]).copyTo(num[j]);
            cvtColor(num[j], num[j], COLOR_BGR2GRAY);
            threshold(num[j], num[j], 120, 255, THRESH_BINARY_INV);

            deal_result[j] = deal(num[j], j + 1);
            //imshow("test", num[j]);
            //waitKey(0);
        }
        float num_result = 100 * deal_result[0] + 10 * deal_result[1] + deal_result[2] + 0.1 * deal_result[3];
        cout << "示数为：" << num_result << endl;

    }
}
void pictureTest() {
    Mat srcImage = imread("../test1_fit.jpg");
    //imshow("test", srcImage);
    //waitKey(0);

    Mat dilateImage;
    myConversion(srcImage, dilateImage, 1);

    // 定义轮廓和层次结构
    vector<vector<Point>> contours;                                                     // 点的集合构成轮廓，contours用于保存轮廓的集合 
    vector<Vec4i> hierarchy;                                                            // 4维int向量，用于保存线段两个端点的坐标
    // 识别轮廓
    findContours(dilateImage, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE);   // 边界识别（二值图像，边界，边界索引，），定位的数字的外轮廓
    int index = 0;
    // 绘制轮廓
    // 备份原图像，用于作图
    Mat dstImage;
    srcImage.copyTo(dstImage);                                                          // 把srcImage的内容复制粘贴到dstImage上
    for (; index >= 0; index = hierarchy[index][0])
    {
        drawContours(dstImage, contours, index, (255, 0, 0), 3, (255, 0, 0), hierarchy);  // 绘制边界
    }

    int i = 0;
    Point2f pp[5][4];                                                                   // Point2f为点的浮点数坐标
    vector<vector<Point>>::iterator It;
    Rect rect[10];                                                                      // opencv的矩形类，（左上角x坐标，左上角y坐标，右下角x坐标，右下角y坐标）
    for (It = contours.begin(); It < contours.end(); It++) {                            // 使用contours迭代器遍历每一个轮廓，找到并画出包围这个轮廓的最小矩阵。
        Point2f vertex[4];                                                              // 四个元素的浮点坐标组，用于保存矩形的四个角坐标
        rect[i] = boundingRect(*It);                                                    // 计算轮廓的垂直边界的最小矩形，并赋值给rect[i]
        vertex[0] = rect[i].tl();                                                       // 矩阵左上角的点坐标赋值给vertex[0]
        vertex[1].x = (float)rect[i].tl().x, vertex[1].y = (float)rect[i].br().y;       // 矩阵左下方的点
        vertex[2] = rect[i].br();                                                       // 矩阵右下角的点坐标赋值给vertex[2]
        vertex[3].x = (float)rect[i].br().x, vertex[3].y = (float)rect[i].tl().y;       // 矩阵右上方的点

        for (int j = 0; j < 4; j++)
            line(dstImage, vertex[j], vertex[(j + 1) % 4], Scalar(0, 0, 255), 10);      // 绘制矩形，每个循环分别绘制矩形的左、下、右、上边

        con[i].x = (vertex[0].x + vertex[1].x + vertex[2].x + vertex[3].x) / 4.0;       // 计算矩形中心点的x坐标
        con[i].y = (vertex[0].y + vertex[1].y + vertex[2].y + vertex[3].y) / 4.0;       // 计算矩形中心点的y坐标
        con[i].order = i;                                                               // 记录中心点的编号
        i++;
        //cout << rect[i].area() << endl;                                                 // test
    }
    sort(con, con + i);

    int deal_result[4];

    // 模板匹配
    for (int j = 0; j < i; j++) {
        int k = con[j].order;
        srcImage(rect[k]).copyTo(num[j]);
        cvtColor(num[j], num[j], COLOR_BGR2GRAY);
        threshold(num[j], num[j], 120, 255, THRESH_BINARY_INV);

        deal_result[j] = deal(num[j], j + 1);
        //imshow("test", num[j]);
        //waitKey(0);
    }
    float num_result = 100 * deal_result[0] + 10 * deal_result[1] + deal_result[2] + 0.1 * deal_result[3];
    cout << "示数为：" << num_result << endl;
}
void test() {
    for (int test = 1; test < 7; test++) {
        string url = "../test/test" + to_string(test);
        url = url + +".jpg";
        cout << url << endl;
        Mat srcImage = imread(url);
    //    myImshow(srcImage,"原图");
    //Mat srcImage = imread("../test/test6.jpg");
    Mat dstImage;
    myConversion(srcImage, dstImage, 0);
    //myImshow(dstImage);
    myResize(srcImage, dstImage);
    //myImshow(srcImage,"重定位图像");

    Mat dstImage2;
    myConversion(srcImage, dstImage2, 1);
    //myImshow(dstImage2,"重定位二值图");
    myRecognition(srcImage, dstImage2);
    }

}
int main()
{
    //videoTest();
    //pictureTest();
    test();
}



void myImshow(Mat& srcImage, string text = "test") {
    namedWindow(text, 0);
    resizeWindow(text, 640, 480);
    imshow(text, srcImage);
    waitKey(0);
}

void myConversion(Mat& srcImage, Mat& dstImage, int i) {
    if (i == 0) {
        line(srcImage, Point(0, srcImage.size().height), Point(srcImage.size().width, srcImage.size().height), Scalar(255, 255, 255), 10);
        bitwise_not(srcImage, dstImage);                                                    //取反
    }
    else
        srcImage.copyTo(dstImage);
    //myImshow(dstImage);
    cvtColor(dstImage, dstImage, COLOR_BGR2GRAY);                                       // 灰度图
    GaussianBlur(dstImage, dstImage, Size(5, 5), 0);                                    // 高斯滤波
    threshold(dstImage, dstImage, 0, 255, THRESH_OTSU | THRESH_BINARY);                 // 二值图像Image。0即为阈值，如果灰度高于0，那么该点会被认为是255，否则为0。
    //myImshow(dstImage,"膨胀前");
    Mat element = getStructuringElement(MORPH_RECT, Size(10, 10));                      // 膨胀核
    if (i == 0) {
        erode(dstImage, dstImage, element);                                                 //腐蚀
        erode(dstImage, dstImage, element);                                                 //????
    }
    else if (i == 1) {
        dilate(dstImage, dstImage, element);                                              // 膨胀
        dilate(dstImage, dstImage, element);                                              // 膨胀
        dilate(dstImage, dstImage, element);                                              // 膨胀
    }
    //myImshow(dstImage, "膨胀后");
    return;
}

void myResize(Mat& srcImage, Mat& binaryImage) {
    // 定义轮廓和层次结构
    vector<vector<Point>> contours;                                                     // 点的集合构成轮廓，contours用于保存轮廓的集合 
    vector<Vec4i> hierarchy;                                                            // 4维int向量，用于保存线段两个端点的坐标
    // 定位轮廓
    findContours(binaryImage, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE);   // 边界识别（二值图像，边界，边界索引，），定位的数字的外轮廓
    //myImshow(srcImage);
    //for (int index = 0; index >= 0; index = hierarchy[index][0]){
    //    drawContours(srcImage, contours, -1, Scalar(0, 0, 255), 10, LINE_8, hierarchy);         // 绘制边界
    //}

    // 选取最大轮廓
    double maxarea = 0;
    int maxAreaIdx = 0;
    for (int index = contours.size() - 1; index >= 0; index--)
    {
        double tmparea = fabs(contourArea(contours[index]));
        if (tmparea > maxarea)
        {
            maxarea = tmparea;
            maxAreaIdx = index;//记录最大轮廓的索引号  
        }
    }
    //drawContours(srcImage, contours, maxAreaIdx, Scalar(0, 0, 255), 10, LINE_8, hierarchy);
    //myImshow(srcImage);


    // 定位矩形四个角
    vector<Point2f> roi_point_approx(4);
    approxPolyDP(contours[maxAreaIdx], roi_point_approx, 7, 1);
    myRelocation(roi_point_approx);
    //for (auto a : roi_point_approx) {
    //    circle(srcImage, a, 10, Scalar(0, 0, 255),10);
    //    myImshow(srcImage);
    //}

    vector<Point2f>src_coners(4);
    src_coners[0] = Point2f(0, 0);
    src_coners[1] = Point2f(srcImage.size().width, 0);
    src_coners[2] = Point2f(0, srcImage.size().height);
    src_coners[3] = Point2f(srcImage.size().width, srcImage.size().height);
    Mat warpMatrix = getPerspectiveTransform(roi_point_approx, src_coners);
    //Mat dstImage;
    warpPerspective(srcImage, srcImage, warpMatrix, srcImage.size(), INTER_LINEAR, BORDER_CONSTANT);
    //myImshow(dstImage);
}

void myRecognition(Mat& srcImage, Mat& binaryImage) {
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    // 定位轮廓
    findContours(binaryImage, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE);   // 边界识别（二值图像，边界，边界索引，），定位的数字的外轮廓
    //myImshow(srcImage);
    //for (int index = 0; index >= 0; index = hierarchy[index][0])
    //    drawContours(srcImage, contours, -1, Scalar(0, 0, 255), 10, LINE_8, hierarchy);         // 绘制边界
    //myImshow(srcImage);

    // 轮廓排序
    sort(contours.begin(), contours.end(), Contour_Area);
    for (int i = 0; i < contours.size(); i++)
    {
        double area = contourArea(contours[i]);
        //cout << area << endl;
    }


    Mat dstImage;
    srcImage.copyTo(dstImage);
    int i = 0;
    Point2f pp[5][4];                                                                   // Point2f为点的浮点数坐标
    vector<vector<Point>>::iterator It;
    Rect rect[10];                                                                      // opencv的矩形类，（左上角x坐标，左上角y坐标，右下角x坐标，右下角y坐标）
    for (It = contours.begin(); It < contours.begin() + 6; It++) {                            // 使用contours迭代器遍历每一个轮廓，找到并画出包围这个轮廓的最小矩阵。
        Point2f vertex[4];                                                              // 四个元素的浮点坐标组，用于保存矩形的四个角坐标
        rect[i] = boundingRect(*It);                                                    // 计算轮廓的垂直边界的最小矩形，并赋值给rect[i]
        vertex[0] = rect[i].tl();                                                       // 矩阵左上角的点坐标赋值给vertex[0]
        vertex[1].x = (float)rect[i].tl().x, vertex[1].y = (float)rect[i].br().y;       // 矩阵左下方的点
        vertex[2] = rect[i].br();                                                       // 矩阵右下角的点坐标赋值给vertex[2]
        vertex[3].x = (float)rect[i].br().x, vertex[3].y = (float)rect[i].tl().y;       // 矩阵右上方的点

        //for (int j = 0; j < 4; j++)
        //    line(dstImage, vertex[j], vertex[(j + 1) % 4], Scalar(0, 0, 255), 10);      // 绘制矩形，每个循环分别绘制矩形的左、下、右、上边
        //myImshow(dstImage,"数字区域");
        con[i].x = (vertex[0].x + vertex[1].x + vertex[2].x + vertex[3].x) / 4.0;       // 计算矩形中心点的x坐标
        con[i].y = (vertex[0].y + vertex[1].y + vertex[2].y + vertex[3].y) / 4.0;       // 计算矩形中心点的y坐标
        con[i].order = i;                                                               // 记录中心点的编号
        i++;
        //cout << rect[i].area() << endl;                                               // test
    }

    int temp;
    double x, y;
    for (int j = 0; j < i - 1; j++) {
        for (int k = j + 1; k < i; k++) {
            if (con[k].y < con[j].y) {
                x = con[j].x;
                y = con[j].y;
                temp = con[j].order;
                con[j].x = con[k].x;
                con[j].y = con[k].y;
                con[j].order = con[k].order;
                con[k].x = x;
                con[k].y = y;
                con[k].order = temp;
            }
        }
    }

    //for (int j = 0; j < i; j++) {
    //    circle(dstImage, Point(con[j].x, con[j].y), 10, Scalar(0, 255, 0), 10);
    //    myImshow(dstImage, "排序后");
    //}

    for (int j = 0; j < 3; j++) {
        for (int k = j + 1; k < 4; k++) {
            if (con[k].x < con[j].x) {
                x = con[j].x;
                y = con[j].y;
                temp = con[j].order;
                con[j].x = con[k].x;
                con[j].y = con[k].y;
                con[j].order = con[k].order;
                con[k].x = x;
                con[k].y = y;
                con[k].order = temp;
            }
        }
    }


    //for (int j = 0; j < i; j++) {
    //    circle(dstImage, Point(con[j].x,con[j].y), 10, Scalar(0, 0, 255), 10);
    //    myImshow(dstImage, "排序后");
    //}

    int deal_result[4];

    // 模板匹配
    for (int j = 0; j < 4; j++) {
        int k = con[j].order;
        srcImage(rect[k]).copyTo(num[j]);
        cvtColor(num[j], num[j], COLOR_BGR2GRAY);
        threshold(num[j], num[j], 120, 255, THRESH_BINARY_INV);
        //myImshow(num[j]);
        deal_result[j] = deal(num[j], j + 1);

        //waitKey(0);
    }
    float num_result = 100 * deal_result[0] + 10 * deal_result[1] + deal_result[2] + 0.1 * deal_result[3];
    cout << "示数为：" << num_result << endl;

    return;
}

int deal(Mat& src, int order)
{
    sample = imread("../0.jpg");
    Threshold(src, sample, 0);

    sample = imread("../1.jpg");
    Threshold(src, sample, 1);

    sample = imread("../2.jpg");
    Threshold(src, sample, 2);

    sample = imread("../3.jpg");
    Threshold(src, sample, 3);

    sample = imread("../4.jpg");
    Threshold(src, sample, 4);

    sample = imread("../5.jpg");
    Threshold(src, sample, 5);

    sample = imread("../6.jpg");
    Threshold(src, sample, 6);

    sample = imread("../7.jpg");
    Threshold(src, sample, 7);

    sample = imread("../8.jpg");
    Threshold(src, sample, 8);

    sample = imread("../9.jpg");
    Threshold(src, sample, 9);


    sort(result, result + 10);

    //for (int i = 0; i < 10; i++)
    //{
    //    cout << result[i].num << ": " << result[i].bi << endl;
    //}


    //if (result[9].bi > 0.3) {
    //    cout << "第" << order << "个数字为 " << result[9].num << endl;
    //    cout << "准确率为" << result[9].bi << endl;
    //}
    //else cout << "无法识别" << endl;

    return result[9].num;
}

void Threshold(Mat& src, Mat& sample, int m)
{
    cvtColor(sample, sample, COLOR_BGR2GRAY);
    threshold(sample, sample, 48, 255, THRESH_BINARY_INV);
    result[m].bi = compare(src, sample);
    result[m].num = m;
}

double compare(Mat& src, Mat& sample)
{
    Mat dst;
    double same = 0.0, difPoint = 0.0;
    Mat now;
    resize(sample, now, src.size());
    /*
    int row = now.rows;
    int col = now.cols * now.channels();
    for (int i = 0; i < 1; i++) {
        uchar* data1 = src.ptr<uchar>(i);
        uchar* data2 = now.ptr<uchar>(i);
        for (int j = 0; j < row * col; j++) {
            int  a = data1[j];
            int b = data2[j];
            if (a == b)same++;
            else difPoint++;
        }
    }
    return same / (difPoint);
    */
    bitwise_xor(src, now, dst);
    //imshow("test", dst);
    //waitKey(0);
    return countNonZero(dst);
}

void myRelocation(vector<Point2f>& corner) {
    //??
    Point2f temp;
    for (int i = 0; i < 3; i++) {
        for (int j = i + 1; j < 4; j++) {
            if (corner[j].y < corner[i].y) {
                temp = corner[i];
                corner[i] = corner[j];
                corner[j] = temp;
            }
        }
    }
    for (int i = 0; i < 3; i += 2) {
        if (corner[i + 1].x < corner[i].x) {
            temp = corner[i + 1];
            corner[i + 1] = corner[i];
            corner[i] = temp;
        }
    }
}

//比较轮廓面积(USB_Port_Lean用来进行轮廓排序)
bool Contour_Area(vector<Point> contour1, vector<Point> contour2)
{
    return contourArea(contour1) > contourArea(contour2);
}
