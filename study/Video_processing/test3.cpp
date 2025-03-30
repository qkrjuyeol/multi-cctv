// Title: 실시간 CCTV 스트리밍 (2x2 그리드 + 클릭 확대)

#include <opencv2/opencv.hpp>
#include <vector>
#include <iostream>

using namespace cv;
using namespace std;

vector<string> stream_urls = {
    "http://cctvsec.ktict.co.kr/138/7ZIeMPWKXQSsPPtEk/L7cZD32MojYyR+t2aPMLmTGIvQwu3zmjLddC2Kk6HC2YxjDL5VScAplTOpAGJGsbszNi8cUBVQxoheI2g82Vi1IbY=",
    "http://cctvsec.ktict.co.kr/139/YdKKm/oXGB3YG8GJZiiEZUcYFycOZHiyC5eDZjSz6u5xVq1J1yi/pMC78nJ4+8eD2EX+GKnaJn9p0GLOdaXSvs+GVJWjyyYX20yaTEf+aMo=",
    "http://cctvsec.ktict.co.kr/140/9dggZLtg9LWod5ZJkJFGVdqiKffMaVPJ+nmBjWG2+LfsRvbWChpl7/79K6Yh/bFxONc9W9V9eMwXCCyJkAj2kSvjjCl8OWUqGxZ98r0jlww=",
    "http://cctvsec.ktict.co.kr/141/9NZjrrlOAEqKrjdlyKbr6j4jpGkg2cKZq1x5xc1BiakObXU1o2B8j978DWJpUKIrVagfgPytq0YWnzx6OlN0ol0+UoQyG+uD1vmIK9SqmDk="
};

vector<VideoCapture> captures(4);
vector<Mat> images(4, Mat::zeros(240, 320, CV_8UC3));
int clickedIndex = -1;
const int IMG_WIDTH = 320;
const int IMG_HEIGHT = 240;
const string WINDOW_NAME = "CCTV Viewer";

void showGrid() {
    Mat grid(IMG_HEIGHT * 2, IMG_WIDTH * 2, CV_8UC3, Scalar(0, 0, 0));
    for (int i = 0; i < 4; ++i) {
        Mat resized;
        resize(images[i], resized, Size(IMG_WIDTH, IMG_HEIGHT));
        int row = i / 2, col = i % 2;
        resized.copyTo(grid(Rect(col * IMG_WIDTH, row * IMG_HEIGHT, IMG_WIDTH, IMG_HEIGHT)));
    }
    imshow(WINDOW_NAME, grid);
}

void showSingle(int index) {
    Mat resized;
    resize(images[index], resized, Size(IMG_WIDTH * 2, IMG_HEIGHT * 2));
    imshow(WINDOW_NAME, resized);
}

void onMouse(int event, int x, int y, int, void*) {
    if (event != EVENT_LBUTTONDOWN) return;

    if (clickedIndex == -1) {
        int col = x / IMG_WIDTH;
        int row = y / IMG_HEIGHT;
        int index = row * 2 + col;
        if (index >= 0 && index < 4) {
            clickedIndex = index;
            showSingle(clickedIndex);
        }
    }
    else {
        clickedIndex = -1;
        showGrid();
    }
}

int main() {
    for (int i = 0; i < 4; ++i) {
        captures[i].open(stream_urls[i]);
        if (!captures[i].isOpened()) {
            cerr << "스트림 열기 실패: " << stream_urls[i] << endl;
        }
    }

    namedWindow(WINDOW_NAME);
    setMouseCallback(WINDOW_NAME, onMouse);

    while (true) {
        for (int i = 0; i < 4; ++i) {
            if (captures[i].isOpened()) {
                captures[i] >> images[i];
                if (images[i].empty()) {
                    images[i] = Mat::zeros(Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC3);
                }
            }
        }

        if (clickedIndex == -1)
            showGrid();
        else
            showSingle(clickedIndex);

        if (waitKey(30) == 27) break; // ESC로 종료
    }

    destroyAllWindows();
    return 0;
}