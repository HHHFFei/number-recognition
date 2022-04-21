#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <time.h>
using namespace cv;
using namespace std;

// ���ο����ĵ㣬���ڱȽ�λ��
struct con {
    double x, y;                    //��������λ��
    int order;                      //��������contours�еĵڼ���

    // �Ƚ��е������Զ�λ��ֻ�Ƚ�����λ��
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

// ����Ƶ������ȡÿһ֡ͼ��
Mat getVideoFrame(string filepath) {
    VideoCapture capture(filepath);
    Mat frame;
    capture.read(frame);			//��ȡ��Ƶ

    // ��Ƶ��Ϣ
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

// ����ƥ��ģ��Ľ����bi��ƥ��ȡ�num��ƥ�������
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

        // ���������Ͳ�νṹ
        vector<vector<Point>> contours;                                                     // ��ļ��Ϲ���������contours���ڱ��������ļ���
        vector<Vec4i> hierarchy;                                                            // 4άint���������ڱ����߶������˵������
        // ʶ������
        findContours(dilateImage, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE);   // �߽�ʶ�𣨶�ֵͼ�񣬱߽磬�߽�������������λ�����ֵ�������
        int index = 0;
        // ��������
        // ����ԭͼ��������ͼ
        Mat dstImage;
        srcImage.copyTo(dstImage);                                                          // ��srcImage�����ݸ���ճ����dstImage��
        for (; index >= 0; index = hierarchy[index][0])
        {
            drawContours(dstImage, contours, index, (255, 0, 0), 3, (255, 0, 0), hierarchy);  // ���Ʊ߽�
        }

        int i = 0;
        Point2f pp[5][4];                                                                   // Point2fΪ��ĸ���������
        vector<vector<Point>>::iterator It;
        Rect rect[10];                                                                      // opencv�ľ����࣬�����Ͻ�x���꣬���Ͻ�y���꣬���½�x���꣬���½�y���꣩
        for (It = contours.begin(); It < contours.end(); It++) {                            // ʹ��contours����������ÿһ���������ҵ���������Χ�����������С����
            Point2f vertex[4];                                                              // �ĸ�Ԫ�صĸ��������飬���ڱ�����ε��ĸ�������
            rect[i] = boundingRect(*It);                                                    // ���������Ĵ�ֱ�߽����С���Σ�����ֵ��rect[i]
            vertex[0] = rect[i].tl();                                                       // �������Ͻǵĵ����긳ֵ��vertex[0]
            vertex[1].x = (float)rect[i].tl().x, vertex[1].y = (float)rect[i].br().y;       // �������·��ĵ�
            vertex[2] = rect[i].br();                                                       // �������½ǵĵ����긳ֵ��vertex[2]
            vertex[3].x = (float)rect[i].br().x, vertex[3].y = (float)rect[i].tl().y;       // �������Ϸ��ĵ�

            for (int j = 0; j < 4; j++)
                line(dstImage, vertex[j], vertex[(j + 1) % 4], Scalar(0, 0, 255), 10);      // ���ƾ��Σ�ÿ��ѭ���ֱ���ƾ��ε����¡��ҡ��ϱ�

            con[i].x = (vertex[0].x + vertex[1].x + vertex[2].x + vertex[3].x) / 4.0;       // ����������ĵ��x����
            con[i].y = (vertex[0].y + vertex[1].y + vertex[2].y + vertex[3].y) / 4.0;       // ����������ĵ��y����
            con[i].order = i;                                                               // ��¼���ĵ�ı��
            i++;
            //cout << rect[i].area() << endl;                                                 // test
        }
        sort(con, con + i);

        int deal_result[4];

        // ģ��ƥ��
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
        cout << "ʾ��Ϊ��" << num_result << endl;

    }
}
void pictureTest() {
    Mat srcImage = imread("../test1_fit.jpg");
    //imshow("test", srcImage);
    //waitKey(0);

    Mat dilateImage;
    myConversion(srcImage, dilateImage, 1);

    // ���������Ͳ�νṹ
    vector<vector<Point>> contours;                                                     // ��ļ��Ϲ���������contours���ڱ��������ļ��� 
    vector<Vec4i> hierarchy;                                                            // 4άint���������ڱ����߶������˵������
    // ʶ������
    findContours(dilateImage, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE);   // �߽�ʶ�𣨶�ֵͼ�񣬱߽磬�߽�������������λ�����ֵ�������
    int index = 0;
    // ��������
    // ����ԭͼ��������ͼ
    Mat dstImage;
    srcImage.copyTo(dstImage);                                                          // ��srcImage�����ݸ���ճ����dstImage��
    for (; index >= 0; index = hierarchy[index][0])
    {
        drawContours(dstImage, contours, index, (255, 0, 0), 3, (255, 0, 0), hierarchy);  // ���Ʊ߽�
    }

    int i = 0;
    Point2f pp[5][4];                                                                   // Point2fΪ��ĸ���������
    vector<vector<Point>>::iterator It;
    Rect rect[10];                                                                      // opencv�ľ����࣬�����Ͻ�x���꣬���Ͻ�y���꣬���½�x���꣬���½�y���꣩
    for (It = contours.begin(); It < contours.end(); It++) {                            // ʹ��contours����������ÿһ���������ҵ���������Χ�����������С����
        Point2f vertex[4];                                                              // �ĸ�Ԫ�صĸ��������飬���ڱ�����ε��ĸ�������
        rect[i] = boundingRect(*It);                                                    // ���������Ĵ�ֱ�߽����С���Σ�����ֵ��rect[i]
        vertex[0] = rect[i].tl();                                                       // �������Ͻǵĵ����긳ֵ��vertex[0]
        vertex[1].x = (float)rect[i].tl().x, vertex[1].y = (float)rect[i].br().y;       // �������·��ĵ�
        vertex[2] = rect[i].br();                                                       // �������½ǵĵ����긳ֵ��vertex[2]
        vertex[3].x = (float)rect[i].br().x, vertex[3].y = (float)rect[i].tl().y;       // �������Ϸ��ĵ�

        for (int j = 0; j < 4; j++)
            line(dstImage, vertex[j], vertex[(j + 1) % 4], Scalar(0, 0, 255), 10);      // ���ƾ��Σ�ÿ��ѭ���ֱ���ƾ��ε����¡��ҡ��ϱ�

        con[i].x = (vertex[0].x + vertex[1].x + vertex[2].x + vertex[3].x) / 4.0;       // ����������ĵ��x����
        con[i].y = (vertex[0].y + vertex[1].y + vertex[2].y + vertex[3].y) / 4.0;       // ����������ĵ��y����
        con[i].order = i;                                                               // ��¼���ĵ�ı��
        i++;
        //cout << rect[i].area() << endl;                                                 // test
    }
    sort(con, con + i);

    int deal_result[4];

    // ģ��ƥ��
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
    cout << "ʾ��Ϊ��" << num_result << endl;
}
void test() {
    for (int test = 1; test < 7; test++) {
        string url = "../test/test" + to_string(test);
        url = url + +".jpg";
        cout << url << endl;
        Mat srcImage = imread(url);
    //    myImshow(srcImage,"ԭͼ");
    //Mat srcImage = imread("../test/test6.jpg");
    Mat dstImage;
    myConversion(srcImage, dstImage, 0);
    //myImshow(dstImage);
    myResize(srcImage, dstImage);
    //myImshow(srcImage,"�ض�λͼ��");

    Mat dstImage2;
    myConversion(srcImage, dstImage2, 1);
    //myImshow(dstImage2,"�ض�λ��ֵͼ");
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
        bitwise_not(srcImage, dstImage);                                                    //ȡ��
    }
    else
        srcImage.copyTo(dstImage);
    //myImshow(dstImage);
    cvtColor(dstImage, dstImage, COLOR_BGR2GRAY);                                       // �Ҷ�ͼ
    GaussianBlur(dstImage, dstImage, Size(5, 5), 0);                                    // ��˹�˲�
    threshold(dstImage, dstImage, 0, 255, THRESH_OTSU | THRESH_BINARY);                 // ��ֵͼ��Image��0��Ϊ��ֵ������Ҷȸ���0����ô�õ�ᱻ��Ϊ��255������Ϊ0��
    //myImshow(dstImage,"����ǰ");
    Mat element = getStructuringElement(MORPH_RECT, Size(10, 10));                      // ���ͺ�
    if (i == 0) {
        erode(dstImage, dstImage, element);                                                 //��ʴ
        erode(dstImage, dstImage, element);                                                 //????
    }
    else if (i == 1) {
        dilate(dstImage, dstImage, element);                                              // ����
        dilate(dstImage, dstImage, element);                                              // ����
        dilate(dstImage, dstImage, element);                                              // ����
    }
    //myImshow(dstImage, "���ͺ�");
    return;
}

void myResize(Mat& srcImage, Mat& binaryImage) {
    // ���������Ͳ�νṹ
    vector<vector<Point>> contours;                                                     // ��ļ��Ϲ���������contours���ڱ��������ļ��� 
    vector<Vec4i> hierarchy;                                                            // 4άint���������ڱ����߶������˵������
    // ��λ����
    findContours(binaryImage, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE);   // �߽�ʶ�𣨶�ֵͼ�񣬱߽磬�߽�������������λ�����ֵ�������
    //myImshow(srcImage);
    //for (int index = 0; index >= 0; index = hierarchy[index][0]){
    //    drawContours(srcImage, contours, -1, Scalar(0, 0, 255), 10, LINE_8, hierarchy);         // ���Ʊ߽�
    //}

    // ѡȡ�������
    double maxarea = 0;
    int maxAreaIdx = 0;
    for (int index = contours.size() - 1; index >= 0; index--)
    {
        double tmparea = fabs(contourArea(contours[index]));
        if (tmparea > maxarea)
        {
            maxarea = tmparea;
            maxAreaIdx = index;//��¼���������������  
        }
    }
    //drawContours(srcImage, contours, maxAreaIdx, Scalar(0, 0, 255), 10, LINE_8, hierarchy);
    //myImshow(srcImage);


    // ��λ�����ĸ���
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
    // ��λ����
    findContours(binaryImage, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_NONE);   // �߽�ʶ�𣨶�ֵͼ�񣬱߽磬�߽�������������λ�����ֵ�������
    //myImshow(srcImage);
    //for (int index = 0; index >= 0; index = hierarchy[index][0])
    //    drawContours(srcImage, contours, -1, Scalar(0, 0, 255), 10, LINE_8, hierarchy);         // ���Ʊ߽�
    //myImshow(srcImage);

    // ��������
    sort(contours.begin(), contours.end(), Contour_Area);
    for (int i = 0; i < contours.size(); i++)
    {
        double area = contourArea(contours[i]);
        //cout << area << endl;
    }


    Mat dstImage;
    srcImage.copyTo(dstImage);
    int i = 0;
    Point2f pp[5][4];                                                                   // Point2fΪ��ĸ���������
    vector<vector<Point>>::iterator It;
    Rect rect[10];                                                                      // opencv�ľ����࣬�����Ͻ�x���꣬���Ͻ�y���꣬���½�x���꣬���½�y���꣩
    for (It = contours.begin(); It < contours.begin() + 6; It++) {                            // ʹ��contours����������ÿһ���������ҵ���������Χ�����������С����
        Point2f vertex[4];                                                              // �ĸ�Ԫ�صĸ��������飬���ڱ�����ε��ĸ�������
        rect[i] = boundingRect(*It);                                                    // ���������Ĵ�ֱ�߽����С���Σ�����ֵ��rect[i]
        vertex[0] = rect[i].tl();                                                       // �������Ͻǵĵ����긳ֵ��vertex[0]
        vertex[1].x = (float)rect[i].tl().x, vertex[1].y = (float)rect[i].br().y;       // �������·��ĵ�
        vertex[2] = rect[i].br();                                                       // �������½ǵĵ����긳ֵ��vertex[2]
        vertex[3].x = (float)rect[i].br().x, vertex[3].y = (float)rect[i].tl().y;       // �������Ϸ��ĵ�

        //for (int j = 0; j < 4; j++)
        //    line(dstImage, vertex[j], vertex[(j + 1) % 4], Scalar(0, 0, 255), 10);      // ���ƾ��Σ�ÿ��ѭ���ֱ���ƾ��ε����¡��ҡ��ϱ�
        //myImshow(dstImage,"��������");
        con[i].x = (vertex[0].x + vertex[1].x + vertex[2].x + vertex[3].x) / 4.0;       // ����������ĵ��x����
        con[i].y = (vertex[0].y + vertex[1].y + vertex[2].y + vertex[3].y) / 4.0;       // ����������ĵ��y����
        con[i].order = i;                                                               // ��¼���ĵ�ı��
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
    //    myImshow(dstImage, "�����");
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
    //    myImshow(dstImage, "�����");
    //}

    int deal_result[4];

    // ģ��ƥ��
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
    cout << "ʾ��Ϊ��" << num_result << endl;

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
    //    cout << "��" << order << "������Ϊ " << result[9].num << endl;
    //    cout << "׼ȷ��Ϊ" << result[9].bi << endl;
    //}
    //else cout << "�޷�ʶ��" << endl;

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

//�Ƚ��������(USB_Port_Lean����������������)
bool Contour_Area(vector<Point> contour1, vector<Point> contour2)
{
    return contourArea(contour1) > contourArea(contour2);
}
