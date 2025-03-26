#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

void onMouse(int event, int x, int y, int flags, void* param) {
	if (event == EVENT_LBUTTONUP) {// ���콺�� ���� ��ư�� �� ��
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
	if (src.empty()) { cout << "������ ���� �� ����" << endl; }
	imshow("src", src);

	setMouseCallback("src", onMouse, &src);
	waitKey(0);
	return 0;
}

// 3/25 ȭ: ȯ�漳�� �� ���� ����
// 3/26 ��: ���� ������ ����. ���콺 �̺�Ʈ ó�� -> ������ �� �׸���
// 3/27 ��: Ŭ���ϸ� �� ȭ�� Ŀ���� �ϱ�