//============================================================================
// Name        : OpenCV_test.cpp
// Author      : morris
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;

int main() {
	// 读入一张图片
	Mat img = imread("author.jpg");
	// 创建一个名为 "作者头像"窗口
	namedWindow("author");
	// 在窗口中显示游戏原画
	imshow("author", img);
	// 等待6000 ms后窗口自动关闭
	waitKey(6000);
	return 0;
}
