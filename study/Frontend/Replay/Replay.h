#pragma once
#include "afxdialogex.h"
#include <opencv2/opencv.hpp>

// Replay 대화 상자
class Replay : public CDialogEx
{
	DECLARE_DYNAMIC(Replay)

public:
	Replay(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~Replay();

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG1 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()

	// 이벤트 핸들러
	afx_msg void OnLbnSelChangeDateList();
	afx_msg void OnLogItemSelected();
	afx_msg void OnDestroy();
	afx_msg void OnStnClickedVideoView();
	afx_msg void SeekVideo(int seconds);
	
	static UINT VideoPlayThread(LPVOID pParam);

public:
	CListBox m_dateListBox;       // 날짜 리스트 박스
	CStringArray m_fullFileNames;  // 전체 파일명 저장
	CListBox m_logListBox;        // 로그 출력용 에디트
	CString m_selectedCsvFile;    // 선택된 CSV 파일 이름
	CString m_csvFolderPath;      // CSV 경로
	CString m_videoFolderPath;    // 영상 경로
	CStatic m_videoStatic;

	cv::VideoCapture m_videoCapture;  // OpenCV 캡처 객체
	CWinThread* m_pVideoThread;
	bool m_bStopVideo;

	CSliderCtrl m_videoSlider;

	bool m_bPaused = false;
	bool m_bSeeking = false;

};
