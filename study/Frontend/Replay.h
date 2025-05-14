#pragma once
#include "afxdialogex.h"

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
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	virtual BOOL OnInitDialog();                         // ← 수정됨

	DECLARE_MESSAGE_MAP()

	// 이벤트 핸들러
	afx_msg void OnLbnSelChangeDateList();
	afx_msg void OnLogItemSelected();
		
public:
	CListBox m_dateListBox;       // 날짜 리스트 박스
	CStringArray m_fullFileNames;  // 전체 파일명 저장
	CListBox m_logListBox;        // 로그 출력용 에디트
	CString m_selectedCsvFile;    // 선택된 CSV 파일 이름
	CString m_csvFolderPath;      // CSV 경로
	CString m_videoFolderPath;    // 영상 경로
};
