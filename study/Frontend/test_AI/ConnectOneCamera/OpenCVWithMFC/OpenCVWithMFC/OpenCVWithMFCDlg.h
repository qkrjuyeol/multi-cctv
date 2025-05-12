#pragma once
#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
#endif
#include "opencv2/opencv.hpp"
#include <vector>
#include <array>
#include <thread>
#include <atomic>

using namespace cv;
HBITMAP MatToHBITMAP(const cv::Mat& mat);
std::vector<BYTE> HBITMAPToJpegBytes(HBITMAP hBmp, ULONG quality);

// COpenCVWithMFCDlg 대화 상자
class COpenCVWithMFCDlg : public CDialogEx
{
public:
	COpenCVWithMFCDlg(CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_OPENCVWITHMFC_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);

protected:
	HICON m_hIcon;

	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT OnAddLogMessage(WPARAM wParam, LPARAM lParam); // 추가!

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnClickedPicture();
	afx_msg void OnClickedPicture1();
	afx_msg void OnClickedPicture2();
	afx_msg void OnClickedPicture3();
	afx_msg void OnStnClickedLogBox();
	afx_msg void OnEnChangeLogBox();

	CStatic m_picture;
	CStatic m_picture1;
	CStatic m_picture2;
	CStatic m_picture3;
	CEdit m_logBox;

private:
	void DisplayFrame(cv::Mat& frame, CStatic& pictureControl);
	void ToggleZoom(int index);
	void AddLog(const CString& log);
	std::string EncodeMatToBase64(const cv::Mat& mat);
	std::string Base64Encode(const std::vector<uchar>& buf);
	bool PostFrameAndGetDetections(const std::string& b64, std::string& outJson);

	std::vector<cv::VideoCapture> captures;
	std::vector<std::string> streamURLs;
	std::array<CRect, 4> originalRects;
	bool isZoomed[4] = { false, false, false, false };

	std::atomic<bool> stopLogThread;
	std::thread logThread;

	// --- 스트리밍용 소켓/스레드 ---
	   // 스트리밍 소켓/스레드 – 크기만 선언
	SOCKET m_streamSock[4];
	HANDLE m_hRecvThread[4];
	
	   // 스레드 함수 & 메시지 핸들러
	static unsigned __stdcall RecvThread(LPVOID pParam);
	afx_msg LRESULT OnStreamFrame(WPARAM wParam, LPARAM lParam);
	
	   // WM_APP 계열 숫자는 헤더에서만 정의
	static constexpr UINT WM_STREAM_FRAME = WM_APP + 100;
	CStatic* camViews[4] = { nullptr,nullptr,nullptr,nullptr };
	HBITMAP    m_bitmaps[4] = { nullptr,nullptr,nullptr,nullptr };
};