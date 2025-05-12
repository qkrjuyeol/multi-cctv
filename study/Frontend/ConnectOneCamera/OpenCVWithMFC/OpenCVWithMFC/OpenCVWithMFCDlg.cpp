#include "pch.h"
#include "framework.h"
#include <opencv2/opencv.hpp>
#include "OpenCVWithMFC.h"
#include "OpenCVWithMFCDlg.h"
#include "afxdialogex.h"

#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment(lib, "gdiplus.lib")

// ── Winsock2/Windows 헤더 (한 번만!) ──
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <windows.h>
#endif

#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")



#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace cv;
static ULONG_PTR gdiToken;

BEGIN_MESSAGE_MAP(COpenCVWithMFCDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_DESTROY()
    ON_WM_TIMER()
    ON_STN_CLICKED(IDC_PICTURE, &COpenCVWithMFCDlg::OnClickedPicture)
    ON_STN_CLICKED(IDC_PICTURE1, &COpenCVWithMFCDlg::OnClickedPicture1)
    ON_STN_CLICKED(IDC_PICTURE2, &COpenCVWithMFCDlg::OnClickedPicture2)
    ON_STN_CLICKED(IDC_PICTURE3, &COpenCVWithMFCDlg::OnClickedPicture3)
    ON_STN_CLICKED(IDC_LOG_BOX, &COpenCVWithMFCDlg::OnStnClickedLogBox)
    ON_EN_CHANGE(IDC_LOG_BOX, &COpenCVWithMFCDlg::OnEnChangeLogBox)
    ON_MESSAGE(WM_APP + 1, &COpenCVWithMFCDlg::OnAddLogMessage) // 추가!
    // 실시간 스트림 프레임 표시용 메시지
    ON_MESSAGE(WM_APP + 100, &COpenCVWithMFCDlg::OnStreamFrame)
    ON_WM_DRAWITEM()

END_MESSAGE_MAP()

// CAboutDlg
class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    DECLARE_MESSAGE_MAP()
};
CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}
void CAboutDlg::DoDataExchange(CDataExchange* pDX) { CDialogEx::DoDataExchange(pDX); }
BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// COpenCVWithMFCDlg 생성
COpenCVWithMFCDlg::COpenCVWithMFCDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_OPENCVWITHMFC_DIALOG, pParent)
    , stopLogThread(false)  // atomic 초기값
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    // --- 스트리밍 소켓/스레드 배열 초기화 ---
    for (int i = 0; i < 4; ++i) {
        m_streamSock[i] = INVALID_SOCKET;
        m_hRecvThread[i] = nullptr;
    }
    // -------------------------------------------
}

void COpenCVWithMFCDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PICTURE, m_picture);
    DDX_Control(pDX, IDC_PICTURE1, m_picture1);
    DDX_Control(pDX, IDC_PICTURE2, m_picture2);
    DDX_Control(pDX, IDC_PICTURE3, m_picture3);
}


BOOL COpenCVWithMFCDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    m_logBox.SubclassDlgItem(IDC_LOG_BOX, this);
    GdiplusStartupInput gdiplusInit;
    GdiplusStartup(&gdiToken, &gdiplusInit, nullptr);

    // Picture 컨트롤 OWNERDRAW/NOTIFY
    m_picture.ModifyStyle(SS_TYPEMASK, SS_OWNERDRAW | SS_NOTIFY);
    DWORD style = ::GetWindowLong(m_picture.m_hWnd, GWL_STYLE);
    TRACE("IDC_PICTURE 스타일: 0x%08X (OWNERDRAW=%s, NOTIFY=%s)\n",
        style,
        (style & SS_OWNERDRAW) ? "O" : "X",
        (style & SS_NOTIFY) ? "O" : "X");
    m_picture1.ModifyStyle(SS_TYPEMASK, SS_OWNERDRAW | SS_NOTIFY);
    m_picture2.ModifyStyle(SS_TYPEMASK, SS_OWNERDRAW | SS_NOTIFY);
    m_picture3.ModifyStyle(SS_TYPEMASK, SS_OWNERDRAW | SS_NOTIFY);

    // ─── TCP 스트림 연결 ───
    WSADATA wsa;
    int wsaErr = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (wsaErr != 0) {
        CString msg;
        msg.Format(_T("WSAStartup failed with error: %d"), wsaErr);
        AfxMessageBox(msg);
        return FALSE;
    }

    for (int i = 0; i < 1; ++i)
    {
        m_streamSock[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        sockaddr_in srv = {};
        srv.sin_family = AF_INET;
        srv.sin_port = htons(10000 + i);
        inet_pton(AF_INET, "127.0.0.1", &srv.sin_addr);

        if (connect(m_streamSock[i], (sockaddr*)&srv, sizeof(srv)) == 0)
        {
            TRACE("MFC: connected to server on port %d\n", 10000 + i);

            auto param = new std::pair<COpenCVWithMFCDlg*, int>(this, i);
            m_hRecvThread[i] = (HANDLE)_beginthreadex(
                nullptr, 0,
                RecvThread,
                param,
                0, nullptr
            );
            TRACE("MFC: started RecvThread[%d], handle=0x%p\n", i, m_hRecvThread[i]);
        }
        else
        {
            int err = WSAGetLastError();
            TRACE("MFC: connect failed on port %d, err=%d\n", 10000 + i, err);
            CString msg;
            msg.Format(_T("스트림 %d 연결 실패 (err=%d)"), i + 1, err);
            MessageBox(msg);
        }
    }
    // ───────────────────────

    // camViews 초기화 등…
    camViews[0] = &m_picture;
    camViews[1] = &m_picture1;
    camViews[2] = &m_picture2;
    camViews[3] = &m_picture3;
    for (int i = 0; i < 4; ++i) {
        CRect wr;
        camViews[i]->GetWindowRect(&wr);
        ScreenToClient(&wr);
        originalRects[i] = wr;
    }

    return TRUE;
}


void COpenCVWithMFCDlg::OnDestroy()
{
    CDialogEx::OnDestroy();
    GdiplusShutdown(gdiToken);
    stopLogThread = true;
    if (logThread.joinable())
        logThread.join();
    for (auto& bmp : m_bitmaps) {
        if (bmp) ::DeleteObject(bmp);
    }
}

void COpenCVWithMFCDlg::OnTimer(UINT_PTR nIDEvent)
{
    // 4개 CCTV 영상 실시간 표시
    for (size_t i = 0; i < captures.size(); i++)
    {
        if (!captures[i].isOpened()) continue;

        cv::Mat frame;
        captures[i].read(frame);
        if (frame.empty()) continue;

        switch (i) {
        case 0: DisplayFrame(frame, m_picture); break;
        case 1: DisplayFrame(frame, m_picture1); break;
        case 2: DisplayFrame(frame, m_picture2); break;
        case 3: DisplayFrame(frame, m_picture3); break;
        }
    }
    CDialogEx::OnTimer(nIDEvent);
}


void COpenCVWithMFCDlg::DisplayFrame(cv::Mat& frame, CStatic& pictureControl)
{
    if (frame.empty()) return;

    CRect rect;
    pictureControl.GetClientRect(&rect);
    cv::Mat frameRGB;
    cv::resize(frame, frameRGB, cv::Size(rect.Width(), rect.Height()));
    cv::cvtColor(frameRGB, frameRGB, cv::COLOR_BGR2RGB);

    CImage cimage_mfc;
    cimage_mfc.Create(frameRGB.cols, frameRGB.rows, 24);

    uchar* pBuffer = (uchar*)cimage_mfc.GetBits();
    int step = cimage_mfc.GetPitch();
    for (int y = 0; y < frameRGB.rows; y++)
        memcpy(pBuffer + y * step, frameRGB.ptr(y), frameRGB.cols * 3);

    HDC dc = ::GetDC(pictureControl.m_hWnd);
    cimage_mfc.Draw(dc, 0, 0);
    ::ReleaseDC(pictureControl.m_hWnd, dc);
}

LRESULT COpenCVWithMFCDlg::OnAddLogMessage(WPARAM wParam, LPARAM lParam)
{
    CString* pLog = (CString*)wParam;
    AddLog(*pLog);
    delete pLog;
    return 0;
}

void COpenCVWithMFCDlg::ToggleZoom(int index)
{
    if (isZoomed[index])
    {
        // 원래 크기로 복원
        for (int i = 0; i < 4; i++)
        {
            camViews[i]->MoveWindow(&originalRects[i]);
            camViews[i]->ShowWindow(SW_SHOW);
        }
    }
    else
    {
        CRect dialogRect;
        GetClientRect(&dialogRect);

        CRect baseRect = originalRects[index];
        const float zoomFactor = 2.0f;

        int newWidth = static_cast<int>(baseRect.Width() * zoomFactor);
        int newHeight = static_cast<int>(baseRect.Height() * zoomFactor);

        CRect zoomRect;

        switch (index)
        {
        case 0: // 왼쪽 위 기준 (기본)
            zoomRect.left = baseRect.left;
            zoomRect.top = baseRect.top;
            zoomRect.right = baseRect.left + newWidth;
            zoomRect.bottom = baseRect.top + newHeight;
            break;

        case 1: // 오른쪽 위 기준
            zoomRect.right = baseRect.right;
            zoomRect.top = baseRect.top;
            zoomRect.left = baseRect.right - newWidth;
            zoomRect.bottom = baseRect.top + newHeight;
            break;

        case 2: // 왼쪽 아래 기준
            zoomRect.left = baseRect.left;
            zoomRect.bottom = baseRect.bottom;
            zoomRect.right = baseRect.left + newWidth;
            zoomRect.top = baseRect.bottom - newHeight;
            break;

        case 3: // 오른쪽 아래 기준
            zoomRect.right = baseRect.right;
            zoomRect.bottom = baseRect.bottom;
            zoomRect.left = baseRect.right - newWidth;
            zoomRect.top = baseRect.bottom - newHeight;
            break;
        }

        // 다이얼로그 범위를 넘지 않도록 조정
        if (zoomRect.left < 0) zoomRect.left = 0;
        if (zoomRect.top < 0) zoomRect.top = 0;
        if (zoomRect.right > dialogRect.right) zoomRect.right = dialogRect.right;
        if (zoomRect.bottom > dialogRect.bottom) zoomRect.bottom = dialogRect.bottom;

        camViews[index]->MoveWindow(&zoomRect);

        // 다른 카메라 숨기기
        for (int i = 0; i < 4; i++)
        {
            if (i != index)
                camViews[i]->ShowWindow(SW_HIDE);
        }
    }

    isZoomed[index] = !isZoomed[index];
}




void COpenCVWithMFCDlg::OnClickedPicture()
{
    std::cout << "Picture clicked!" << std::endl;  // 디버깅용 출력
    AddLog(_T("첫 번째 화면이 클릭되었습니다."));
    ToggleZoom(0);
}

void COpenCVWithMFCDlg::OnClickedPicture1()
{
    std::cout << "Picture clicked1!" << std::endl;  // 디버깅용 출력
    AddLog(_T("두 번째 화면이 클릭되었습니다."));
    ToggleZoom(1);
}

void COpenCVWithMFCDlg::OnClickedPicture2()
{
    std::cout << "Picture clicked2!" << std::endl;  // 디버깅용 출력
    AddLog(_T("세 번째 화면이 클릭되었습니다."));
    ToggleZoom(2);
}

void COpenCVWithMFCDlg::OnClickedPicture3()
{
    std::cout << "Picture clicked3!" << std::endl;  // 디버깅용 출력
    AddLog(_T("네 번째 화면이 클릭되었습니다."));
    ToggleZoom(3);
}


void COpenCVWithMFCDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xfff0) == SC_KEYMENU) // Disable the ALT application menu
    {
        return;
    }
    CDialogEx::OnSysCommand(nID, lParam);
}

void COpenCVWithMFCDlg::OnStnClickedLogBox()
{
    // 로그 박스를 클릭했을 때의 동작을 구현
    // 예시: 로그를 출력하는 대화 상자 열기
    AfxMessageBox(_T("Log box clicked!"));
}

void COpenCVWithMFCDlg::OnEnChangeLogBox()
{
    // TODO:  RICHEDIT 컨트롤인 경우, 이 컨트롤은
    // CDialogEx::OnInitDialog() 함수를 재지정 
    //하고 마스크에 OR 연산하여 설정된 ENM_CHANGE 플래그를 지정하여 CRichEditCtrl().SetEventMask()를 호출하지 않으면
    // 이 알림 메시지를 보내지 않습니다.

    // TODO:  여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

void COpenCVWithMFCDlg::AddLog(const CString& log)
{
    CString currentText;
    m_logBox.GetWindowText(currentText);

    currentText += log + _T("\r\n");
    m_logBox.SetWindowText(currentText);

    // 가장 아래로 스크롤
    m_logBox.LineScroll(m_logBox.GetLineCount());
}


bool COpenCVWithMFCDlg::PostFrameAndGetDetections(const std::string& b64, std::string& outJson)
{
    HINTERNET hSession = WinHttpOpen(L"MFCApp", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, L"127.0.0.1", 5000, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/api/detect",
        NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    std::string body = "{\"image\":\"data:image/jpeg;base64," + b64 + "\"}";
    BOOL bResults = WinHttpSendRequest(hRequest,
        L"Content-Type: application/json\r\n", -1L,
        (LPVOID)body.c_str(), (DWORD)body.length(), (DWORD)body.length(), 0);

    if (!bResults || !WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD dwSize = 0;
    std::string response;
    do {
        DWORD dwDownloaded = 0;
        WinHttpQueryDataAvailable(hRequest, &dwSize);
        if (!dwSize) break;

        std::vector<char> buffer(dwSize + 1);
        ZeroMemory(buffer.data(), dwSize + 1);

        WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded);
        response.append(buffer.data(), dwDownloaded);
    } while (dwSize > 0);

    outJson = response;

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return true;
}

// Base64 인코딩 (표준)
static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

std::string COpenCVWithMFCDlg::Base64Encode(const std::vector<uchar>& buf)
{
    std::string result;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    int in_len = static_cast<int>(buf.size());
    const unsigned char* bytes_to_encode = buf.data();

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) +
                ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) +
                ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
                result += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) +
            ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) +
            ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; j < i + 1; j++)
            result += base64_chars[char_array_4[j]];

        while (i++ < 3)
            result += '=';
    }

    return result;
}

std::string COpenCVWithMFCDlg::EncodeMatToBase64(const cv::Mat& mat)
{
    HBITMAP hBmp = MatToHBITMAP(mat);
    auto jpeg = HBITMAPToJpegBytes(hBmp, 90);
    DeleteObject(hBmp);
    std::vector<uchar> u8(jpeg.begin(), jpeg.end());
    return Base64Encode(u8);
}


void COpenCVWithMFCDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this);
        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;
        dc.DrawIcon(x, y, m_hIcon);
        return;
    }

    // 1) 기본 페인트
    CDialogEx::OnPaint();

    // 2) Picture 컨트롤들을 SS_BITMAP 으로 바꿔놨다면
    //    DrawEdge 로 테두리만 수동으로 그려준다
    CPaintDC dc(this);

    auto drawBorder = [&](CStatic& ctrl) {
        CRect r;
        ctrl.GetWindowRect(&r);
        ScreenToClient(&r);
        DrawEdge(dc, &r, EDGE_SUNKEN, BF_RECT);
        };

    drawBorder(m_picture);
    drawBorder(m_picture1);
    drawBorder(m_picture2);
    drawBorder(m_picture3);
}


HCURSOR COpenCVWithMFCDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

// RecvThread: 소켓에서 4바이트 길이 + JPEG 바이트 읽고 메인 스레드로 전달
unsigned __stdcall COpenCVWithMFCDlg::RecvThread(LPVOID pParam)
{
    auto p = static_cast<std::pair<COpenCVWithMFCDlg*, int>*>(pParam);
    COpenCVWithMFCDlg* pDlg = p->first;
    int                idx = p->second;
    delete p;

    TRACE("RecvThread[%d]: entry\n", idx);    // ← 이 로그가 반드시 보여야 합니다.

    SOCKET sock = pDlg->m_streamSock[idx];
    while (true) {
        uint32_t netLen = 0;
        char* lenPtr = reinterpret_cast<char*>(&netLen);
        int  received = 0;
        while (received < 4) {
            int n = recv(sock, lenPtr + received, 4 - received, 0);
            if (n <= 0) {
                TRACE("RecvThread[%d]: prefix recv failed, n=%d err=%d\n",
                    idx, n, WSAGetLastError());
                goto DONE;
            }
            received += n;
            TRACE("RecvThread[%d]: got %d/%d prefix bytes\n", idx, received, 4);
        }
        uint32_t dataLen = ntohl(netLen);
        TRACE("RecvThread[%d]: dataLen=%u\n", idx, dataLen);

        std::vector<uchar> buf(dataLen);
        int rec = 0;
        while (rec < (int)dataLen) {
            int r = recv(sock, (char*)buf.data() + rec, dataLen - rec, 0);
            if (r <= 0) {
                TRACE("RecvThread[%d]: body recv failed, r=%d err=%d\n",
                    idx, r, WSAGetLastError());
                goto DONE;
            }
            rec += r;
            TRACE("RecvThread[%d]: got %d/%u body bytes\n", idx, rec, dataLen);
        }

        cv::Mat frame = cv::imdecode(buf, cv::IMREAD_COLOR);
        if (frame.empty()) {
            TRACE("RecvThread[%d]: imdecode failed (empty frame)\n", idx);
            continue;
        }
        TRACE("RecvThread[%d]: decoded frame %dx%d\n",
            idx, frame.cols, frame.rows);

        // — 원래 있던 PostMessage 코드 —
        cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
        CImage img;
        img.Create(frame.cols, frame.rows, 24);

        BYTE* pBits = (BYTE*)img.GetBits();
        int pitch = img.GetPitch();  // 한 줄의 바이트 수 (패딩 포함)

        // 안전하게 한 줄씩 복사
        for (int y = 0; y < frame.rows; y++) {
            memcpy(pBits + y * pitch, frame.ptr(y), frame.cols * 3);
        }
        HBITMAP hBmp = (HBITMAP)img.Detach();
        TRACE("RecvThread[%d]: posting HBITMAP=0x%p\n", idx, hBmp);
        pDlg->PostMessage(WM_APP + 100, (WPARAM)hBmp, idx);
    }
DONE:
    closesocket(sock);
    return 0;
}


LRESULT COpenCVWithMFCDlg::OnStreamFrame(WPARAM wParam, LPARAM lParam)
{
    TRACE("OnStreamFrame(): called! wParam=0x%p, lParam=%d\n", (void*)wParam, (int)lParam);
    int     idx = (int)lParam;
    HBITMAP hBmp = (HBITMAP)wParam;

    TRACE("OnStreamFrame(): idx=%d, hBmp=0x%p\n", idx, hBmp);

    // ① 이전 비트맵 지우기
    if (m_bitmaps[idx]) {
        ::DeleteObject(m_bitmaps[idx]);
    }
    // ② 새 비트맵 저장
    m_bitmaps[idx] = hBmp;

    // ③ 해당 컨트롤을 즉시 다시 그리기
    CStatic* ctrl = &m_picture;
    if (idx == 1) ctrl = &m_picture1;
    else if (idx == 2) ctrl = &m_picture2;
    else if (idx == 3) ctrl = &m_picture3;

    ctrl->Invalidate(FALSE);
    ctrl->UpdateWindow();    // <- 여기서 바로 WM_DRAWITEM을 트리거

    return 0;
}


int GetEncoderClsid(const WCHAR* mime, CLSID* pClsid) {
    UINT  num = 0, size = 0;
    // 1) 인코더 크기 조회
    if (GetImageEncodersSize(&num, &size) != Ok || size == 0) {
        // JPEG 인코더 정보가 없거나 오류
        return -1;
    }

    // 2) 메모리 할당 & 결과 체크
    ImageCodecInfo* codecs = static_cast<ImageCodecInfo*>(malloc(size));
    if (!codecs) {
        // 메모리 할당 실패
        return -1;
    }

    // 3) 실제 인코더 정보 가져오기
    if (GetImageEncoders(num, size, codecs) != Ok) {
        free(codecs);
        return -1;
    }

    // 4) 원하는 MIME 타입 탐색
    int result = -1;
    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(codecs[i].MimeType, mime) == 0) {
            *pClsid = codecs[i].Clsid;
            result = static_cast<int>(i);
            break;
        }
    }

    // 5) 할당 해제 후 반환
    free(codecs);
    return result;
}


HBITMAP MatToHBITMAP(const cv::Mat& mat) {
    cv::Mat rgb; cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = rgb.cols;
    bmi.bmiHeader.biHeight = -rgb.rows;  // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC hdc = GetDC(NULL);
    HBITMAP hBmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    ReleaseDC(NULL, hdc);
    if (hBmp && bits)
        memcpy(bits, rgb.data, rgb.total() * 3);
    return hBmp;
}

std::vector<BYTE> HBITMAPToJpegBytes(HBITMAP hBmp, ULONG quality = 90) {
    // 메모리 스트림
    IStream* stream = nullptr;
    CreateStreamOnHGlobal(NULL, TRUE, &stream);

    Bitmap bitmap(hBmp, NULL);
    CLSID clsid;
    GetEncoderClsid(L"image/jpeg", &clsid);

    EncoderParameters params = { 1, {{EncoderQuality,
                EncoderParameterValueTypeLong, 1, &quality}} };
    bitmap.Save(stream, &clsid, &params);

    // 스트림에서 데이터 읽기
    HGLOBAL hMem;
    GetHGlobalFromStream(stream, &hMem);
    SIZE_T size = GlobalSize(hMem);
    BYTE* data = (BYTE*)GlobalLock(hMem);
    std::vector<BYTE> buf(data, data + size);
    GlobalUnlock(hMem);
    stream->Release();
    return buf;
}

void COpenCVWithMFCDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS)
{
    int idx = -1;
    switch (nIDCtl) {
    case IDC_PICTURE:  idx = 0; break;
    case IDC_PICTURE1: idx = 1; break;
    case IDC_PICTURE2: idx = 2; break;
    case IDC_PICTURE3: idx = 3; break;
    default:
        CDialogEx::OnDrawItem(nIDCtl, lpDIS);
        return;
    }

    TRACE("OnDrawItem(): nIDCtl=%d idx=%d bmp=0x%p\n",
        nIDCtl, idx, m_bitmaps[idx]);

    CDC dc; dc.Attach(lpDIS->hDC);
    CRect rc(lpDIS->rcItem);

    // 배경 지우기 (없으면 이전 프레임 잔상이 남거나 하얗게 덮임)
    dc.FillSolidRect(&rc, ::GetSysColor(COLOR_3DFACE));

    // 테두리
    dc.DrawEdge(&rc, EDGE_SUNKEN, BF_RECT);

    // 비트맵이 있으면 그리기
    if (m_bitmaps[idx]) {
        BITMAP bm; ::GetObject(m_bitmaps[idx], sizeof(bm), &bm);
        CDC mem; mem.CreateCompatibleDC(&dc);
        HGDIOBJ old = mem.SelectObject(m_bitmaps[idx]);
        dc.BitBlt(rc.left, rc.top, bm.bmWidth, bm.bmHeight, &mem, 0, 0, SRCCOPY);
        mem.SelectObject(old);
    }

    dc.Detach();
}
