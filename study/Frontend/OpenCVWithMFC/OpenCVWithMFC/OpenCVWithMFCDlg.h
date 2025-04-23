// OpenCVWithMFCDlg.h: 헤더 파일
//

#pragma once
#include "opencv2/opencv.hpp"
#include <vector>
#include <array>

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
	afx_msg void OnClickedPicture();
	afx_msg void OnClickedPicture1();
	afx_msg void OnClickedPicture2();
	afx_msg void OnClickedPicture3();
	afx_msg void OnStnClickedLogBox();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	CStatic m_picture;
	CStatic m_picture1;
	CStatic m_picture2;
	CStatic m_picture3;


private:
	void DisplayFrame(cv::Mat& frame, CStatic& pictureControl);
	void ToggleZoom(int index);
	void AddLog(const CString& log);

	std::vector<cv::VideoCapture> captures;
	std::vector<std::string> streamURLs;
	std::array<CRect, 4> originalRects;
	bool isZoomed[4] = { false, false, false, false };
	CStatic* camViews[4];
	CEdit m_logBox;

	std::array<ULONGLONG, 4> lastLogTime{}; // ✅ 여기 추가

	std::string EncodeMatToBase64(const cv::Mat& mat);
	std::string Base64Encode(const std::vector<uchar>& buf);
	bool PostFrameAndGetDetections(const std::string& b64, std::string& outJson);

public:
	afx_msg void OnEnChangeLogBox();
};
