#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

void onMouse(int event, int x, int y, int flags, void* param) {
	if (event == EVENT_LBUTTONUP) {// 마우스의 왼쪽 버튼을 뗄 때
		Mat& img = *(Mat*)(param);
		try {
			circle(img, Point(x, y), 200, Scalar(0, 255, 0), 10);
			putText(img, "I found a dog!", Point(x, y), FONT_HERSHEY_PLAIN, 2.0, Scalar(255, 255, 255), 2);
			imshow("src", img);
		}
		catch (const cv::Exception& e) {
			cerr << "OpenCV Error: " << e.what() << endl;
		}
	}
}

int main() {
	Mat src = imread("lenna.jpg", IMREAD_COLOR);;
	if (src.empty()) { cout << "영상을 읽을 수 없음" << endl; }
	imshow("src", src);

	setMouseCallback("src", onMouse, &src);
	waitKey(0);
	return 0;
}

// 3/25 화: 환경설정 및 사진 띄우기
// 3/26 수: 사진 여러개 띄우기. 마우스 이벤트 처리 -> 누르면 원 그리기
// 3/27 목: 클릭하면 그 화면 커지게 하기