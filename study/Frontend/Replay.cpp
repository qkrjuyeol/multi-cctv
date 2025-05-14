// Replay.cpp: 구현 파일
//

#include "pch.h"
#include "OpenCVWithMFC.h"
#include "afxdialogex.h"
#include "resource.h"
#include "Replay.h"


// Replay 대화 상자
IMPLEMENT_DYNAMIC(Replay, CDialogEx)

Replay::Replay(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG1, pParent)
{
    m_csvFolderPath = _T("D:\\project2\\log\\logs"); // CSV 저장 경로
    m_videoFolderPath = _T("D:\\project2\\videos");  // 영상 경로 (추가)
}

Replay::~Replay()
{
}


void Replay::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_DATE, m_dateListBox);
    DDX_Control(pDX, IDC_LIST_LOG, m_logListBox);
}


BEGIN_MESSAGE_MAP(Replay, CDialogEx)
    ON_LBN_SELCHANGE(IDC_LIST_DATE, &Replay::OnLbnSelChangeDateList)
    //ON_BN_CLICKED(IDC_BUTTON1, &Replay::OnBnClickedButton1)
    ON_LBN_SELCHANGE(IDC_LIST_LOG, &Replay::OnLogItemSelected)

END_MESSAGE_MAP()


// Replay 메시지 처리기
BOOL Replay::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    WIN32_FIND_DATA findFileData;
    CString searchPath = m_csvFolderPath + _T("\\*.csv");

    HANDLE hFind = FindFirstFile(searchPath, &findFileData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do {
            CString fileName(findFileData.cFileName);
            //fileName.Replace(_T(".csv"), _T(""));
            m_dateListBox.AddString(fileName);
        } while (FindNextFile(hFind, &findFileData));
        FindClose(hFind);
    }
    else
    {
        AfxMessageBox(_T("CSV 파일을 찾을 수 없습니다."));
    }

    return TRUE;
}


// 리스트 박스에서 날짜를 선택했을 때 호출되는 함수
void Replay::OnLbnSelChangeDateList()
{
    if (!::IsWindow(m_dateListBox.m_hWnd) || !::IsWindow(m_logListBox.m_hWnd)) return;

    int sel = m_dateListBox.GetCurSel();
    if (sel == LB_ERR) return;

    m_dateListBox.GetText(sel, m_selectedCsvFile);  // 선택된 파일 이름 저장
    CString csvPath = m_csvFolderPath + _T("\\") + m_selectedCsvFile;

    CStdioFile file;
    CString line;

    if (!file.Open(csvPath, CFile::modeRead | CFile::typeText)) {
        AfxMessageBox(_T("CSV 파일을 열 수 없습니다: ") + csvPath);
        return;
    }

    m_logListBox.ResetContent();  // 이전 항목 초기화

    bool isFirstLine = true;
    while (file.ReadString(line)) {
        if (isFirstLine) {
            isFirstLine = false; // 헤더는 무시
            continue;
        }
        CString time, cam, desc;
        AfxExtractSubString(time, line, 0, ',');
        AfxExtractSubString(cam, line, 1, ',');
        AfxExtractSubString(desc, line, 2, ',');

        CString displayLine = time + _T("    ") + cam + _T("    ") + desc;
        m_logListBox.AddString(displayLine);
    }

    file.Close();
}


void Replay::OnLogItemSelected()
{
    int sel = m_logListBox.GetCurSel();
    if (sel != LB_ERR) {
        CString logLine;
        m_logListBox.GetText(sel, logLine);

        // 예: "15:32:24,cam04,suitcase detected" → cam ID 추출
        CString camId;
        AfxExtractSubString(camId, logLine, 1, ',');  // cam04

        CString videoPath = m_videoFolderPath + _T("\\") + camId + _T(".mp4");
        ShellExecute(NULL, _T("open"), videoPath, NULL, NULL, SW_SHOWNORMAL);
    }
}
