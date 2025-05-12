// Title: OpenCV (2x2) �̹��� �׸��� �� Ŭ�� Ȯ��/���
#include <opencv2/opencv.hpp>
#include <vector>

using namespace cv;
using namespace std;

vector<Mat> images(4);
int clickedIndex = -1; // -1�̸� �׸��� ���, 0~3�̸� ��üȭ�� ���
const int IMG_WIDTH = 200;
const int IMG_HEIGHT = 200;
const string WINDOW_NAME = "Image Viewer";

void showGrid() {
    Mat grid(IMG_HEIGHT * 2, IMG_WIDTH * 2, images[0].type(), Scalar(0, 0, 0));

    for (int i = 0; i < 4; ++i) {
        int row = i / 2;
        int col = i % 2;
        Mat resized;
        resize(images[i], resized, Size(IMG_WIDTH, IMG_HEIGHT));
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
    if (event != EVENT_LBUTTONDOWN)
        return;

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
        images[i] = imread("img" + to_string(i + 1) + ".jpg");
        if (images[i].empty()) {
            cerr << "�̹��� �ҷ����� ����: img" << i + 1 << ".jpg" << endl;
            return -1;
        }
    }

    namedWindow(WINDOW_NAME);
    setMouseCallback(WINDOW_NAME, onMouse);

    showGrid();
    while (true) {
        if (waitKey(30) == 27) break; // ESC Ű�� ����
    }
    destroyAllWindows();
    return 0;
}
