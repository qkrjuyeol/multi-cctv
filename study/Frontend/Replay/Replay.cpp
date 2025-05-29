// Replay.cpp: 구현 파일
#include "pch.h"
#include "OpenCVWithMFC.h"
#include "afxdialogex.h"
#include "resource.h"
#include "Replay.h"

#include <vector>
#include <utility>
#include <algorithm>
#include <atlconv.h>


// Replay 대화 상자
IMPLEMENT_DYNAMIC(Replay, CDialogEx)

Replay::Replay(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_DIALOG1, pParent)
{
    m_csvFolderPath = _T("C:\\logs");     // CSV 저장 경로
    m_videoFolderPath = _T("C:\\video");  // 영상 경로
}

Replay::~Replay()
{
}

void Replay::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_DATE, m_dateListBox);
    DDX_Control(pDX, IDC_LIST_LOG, m_logListBox);
    DDX_Control(pDX, IDC_VIDEO_VIEW, m_videoStatic);
    DDX_Control(pDX, IDC_SLIDER_VIDEO, m_videoSlider);

}

BEGIN_MESSAGE_MAP(Replay, CDialogEx)
    ON_LBN_SELCHANGE(IDC_LIST_DATE, &Replay::OnLbnSelChangeDateList)
    ON_LBN_SELCHANGE(IDC_LIST_LOG, &Replay::OnLogItemSelected)
    ON_WM_DESTROY()
    ON_STN_CLICKED(IDC_VIDEO_VIEW, &Replay::OnStnClickedVideoView)
END_MESSAGE_MAP()


// Replay 메시지 처리기
BOOL Replay::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    m_videoSlider.SetRange(0, 1000);

    WIN32_FIND_DATA findFileData;
    CString searchPath = m_csvFolderPath + _T("\\*.csv");

    HANDLE hFind = FindFirstFile(searchPath, &findFileData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do {
            CString fileName(findFileData.cFileName);
            m_fullFileNames.Add(fileName);

            CString nameWithoutExt = fileName;
            int dotPos = nameWithoutExt.ReverseFind('.');
            if (dotPos != -1)
                nameWithoutExt = nameWithoutExt.Left(dotPos);

            m_dateListBox.AddString(nameWithoutExt);
        } while (FindNextFile(hFind, &findFileData));
        FindClose(hFind);
    }
    else
    {
        AfxMessageBox(_T("CSV 파일을 찾을 수 없습니다."));
    }

    return TRUE;
}

// 날짜 선택 시 호출
void Replay::OnLbnSelChangeDateList()
{
    if (!::IsWindow(m_dateListBox.m_hWnd) || !::IsWindow(m_logListBox.m_hWnd)) return;

    int sel = m_dateListBox.GetCurSel();
    if (sel == LB_ERR || sel >= m_fullFileNames.GetSize()) return;

    m_selectedCsvFile = m_fullFileNames[sel];
    CString csvPath = m_csvFolderPath + _T("\\") + m_selectedCsvFile;

    CStdioFile file;
    CString line;

    if (!file.Open(csvPath, CFile::modeRead | CFile::typeText)) {
        AfxMessageBox(_T("CSV 파일을 열 수 없습니다: ") + csvPath);
        return;
    }

    m_logListBox.ResetContent();
    bool isFirstLine = true;
    while (file.ReadString(line)) {
        if (isFirstLine) {
            isFirstLine = false;
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

// 로그 항목 선택 시
void Replay::OnLogItemSelected()
{
    int sel = m_logListBox.GetCurSel();
    if (sel == LB_ERR) return;

    CString logLine;
    m_logListBox.GetText(sel, logLine);

    CString timeStr, camId, desc;
    int curPos = 0;

    timeStr = logLine.Tokenize(_T(" "), curPos);
    camId = logLine.Tokenize(_T(" "), curPos);
    desc = logLine.Tokenize(_T(" "), curPos);

    CString dateStr = m_selectedCsvFile;
    int dotPos = dateStr.ReverseFind('.');
    if (dotPos != -1)
        dateStr = dateStr.Left(dotPos);
    dateStr.Remove('-');

    CString logTimeStr = timeStr;
    logTimeStr.Remove(':');
    int logTime = _ttoi(logTimeStr);

    std::vector<std::pair<int, CString>> videoFiles;


    WIN32_FIND_DATA findFileData;
    CString searchPattern = m_videoFolderPath + _T("\\") + camId + _T("_") + dateStr + _T("_*.mp4");
    // AfxMessageBox(searchPattern);  // 경로가 올바른지 확인 
    HANDLE hFind = FindFirstFile(searchPattern, &findFileData);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do {
            CString fileName = findFileData.cFileName;

            int lastUnderscore = fileName.ReverseFind('_');
            int dot = fileName.ReverseFind('.');
            if (lastUnderscore != -1 && dot != -1 && dot > lastUnderscore) {
                CString videoTimeStr = fileName.Mid(lastUnderscore + 1, dot - lastUnderscore - 1);
                int videoTime = _ttoi(videoTimeStr);
                videoFiles.push_back({ videoTime, fileName });
            }
        } while (FindNextFile(hFind, &findFileData));
        FindClose(hFind);
    }

    // 시간순 정렬
    std::sort(videoFiles.begin(), videoFiles.end());

    CString matchedFile;
    int closestTime = -1;

    for (const auto& file : videoFiles) {
        if (file.first <= logTime && file.first > closestTime) {
            closestTime = file.first;
            matchedFile = file.second;
        }
    }

    if (matchedFile.IsEmpty() && !videoFiles.empty() && logTime >= videoFiles.back().first) {
        matchedFile = videoFiles.back().second;
    }

    if (!matchedFile.IsEmpty()) {
        if (m_pVideoThread != nullptr)
        {
            m_bStopVideo = true;
            WaitForSingleObject(m_pVideoThread->m_hThread, INFINITE);
            m_pVideoThread = nullptr;
        }

        CString videoPath = m_videoFolderPath + _T("\\") + matchedFile;

        if (m_videoCapture.isOpened())
            m_videoCapture.release();
        CT2A asciiPath(videoPath);
        m_videoCapture.open((LPCSTR)asciiPath);

        if (!m_videoCapture.isOpened()) {
            AfxMessageBox(_T("영상을 열 수 없습니다."));
            return;
        }

        m_bStopVideo = false;
        m_pVideoThread = AfxBeginThread(VideoPlayThread, this);
    }
}


UINT Replay::VideoPlayThread(LPVOID pParam)
{
    Replay* pDlg = reinterpret_cast<Replay*>(pParam);
    cv::Mat frame;
    CRect rect;
    pDlg->m_videoStatic.GetClientRect(&rect);
    int width = rect.Width();
    int height = rect.Height();

    while (!pDlg->m_bStopVideo && pDlg->m_videoCapture.isOpened())
    {
        if (pDlg->m_bPaused || pDlg->m_bSeeking) {
            Sleep(10);  // 일시정지 또는 시킹 중이면 기다림
            continue;
        }

        if (!pDlg->m_videoCapture.read(frame))
            break;

        cv::resize(frame, frame, cv::Size(width, height));

        // RGB로 변환
        cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);

        // MFC에 표시하기 위한 bitmap 생성
        CImage image;
        image.Create(frame.cols, frame.rows, 24);

        for (int y = 0; y < frame.rows; ++y)
        {
            uchar* src = frame.ptr(y);
            uchar* dest = (uchar*)image.GetBits() + y * image.GetPitch();
            memcpy(dest, src, frame.cols * 3);
        }

        // 화면에 출력
        CClientDC dc(&pDlg->m_videoStatic);
        image.Draw(dc.m_hDC, 0, 0);

        double pos = pDlg->m_videoCapture.get(cv::CAP_PROP_POS_FRAMES);
        double total = pDlg->m_videoCapture.get(cv::CAP_PROP_FRAME_COUNT);
        int sliderPos = static_cast<int>((pos / total) * 1000);
        pDlg->m_videoSlider.SetPos(sliderPos);

        Sleep(30);  // 약 30 FPS

    }

    pDlg->m_videoCapture.release();
    return 0;
}

void Replay::OnDestroy()
{
    CDialogEx::OnDestroy();

    m_bStopVideo = true;
    if (m_pVideoThread != nullptr) {
        WaitForSingleObject(m_pVideoThread->m_hThread, INFINITE);
        m_pVideoThread = nullptr;
    }

    if (m_videoCapture.isOpened())
        m_videoCapture.release();
}

void Replay::OnStnClickedVideoView()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

BOOL Replay::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        switch (pMsg->wParam)
        {
        case VK_LEFT:  // ←
            SeekVideo(-10); // 10초 뒤로
            return TRUE;
        case VK_RIGHT: // →
            SeekVideo(10);  // 10초 앞으로
            return TRUE;
        case VK_SPACE:
            m_bPaused = !m_bPaused;  // 토글 일시정지
            return TRUE;
        }
    }
    return CDialogEx::PreTranslateMessage(pMsg);
}

void Replay::SeekVideo(int seconds)
{
    if (!m_videoCapture.isOpened()) return;

	m_bSeeking = true;

    double fps = m_videoCapture.get(cv::CAP_PROP_FPS);
    double curFrame = m_videoCapture.get(cv::CAP_PROP_POS_FRAMES);
    double totalFrames = m_videoCapture.get(cv::CAP_PROP_FRAME_COUNT);

    double newFrame = curFrame + fps * seconds;
    if (newFrame < 0) newFrame = 0;
    if (newFrame >= totalFrames) newFrame = totalFrames - 1;

    m_videoCapture.set(cv::CAP_PROP_POS_FRAMES, newFrame);

	m_bSeeking = false;
}
