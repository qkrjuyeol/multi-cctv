
// OpenCVWithMFCDlg.h: 헤더 파일
//

#pragma once
#include "opencv2/opencv.hpp"
using namespace cv;

// COpenCVWithMFCDlg 대화 상자
class COpenCVWithMFCDlg : public CDialogEx
{
// 생성입니다.
public:
	COpenCVWithMFCDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_OPENCVWITHMFC_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnStnClickedPicture();
	CStatic m_picture;
	CStatic m_picture1;
	CStatic m_picture2;
	CStatic m_picture3;
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	VideoCapture* capture;
	Mat mat_frame;
	CImage cimage_mfc;
private:
	void DisplayFrame(cv::Mat& frame, CStatic& pictureControl); // 함수 선언 추가
public:
	afx_msg void OnStnClickedPicture3();
	afx_msg void OnStnClickedLogBox();
	CStatic m_logBox;
};
