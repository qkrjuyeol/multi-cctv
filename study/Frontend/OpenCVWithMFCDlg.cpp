// opencv 라이브러리 341d.dll 파일로 구현된 파일. 버전에 맞게 변경 필요
// 로그파일은 D:\project2\log\logs에 저장됨

#include "pch.h"
#include "framework.h"
#include "OpenCVWithMFC.h"
#include "OpenCVWithMFCDlg.h"
#include "afxdialogex.h"
#include <winhttp.h>
#include "Replay.h"
#pragma comment(lib, "winhttp.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(COpenCVWithMFCDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_STN_CLICKED(IDC_PICTURE, &COpenCVWithMFCDlg::OnClickedPicture)
	ON_STN_CLICKED(IDC_PICTURE1, &COpenCVWithMFCDlg::OnClickedPicture1)
	ON_STN_CLICKED(IDC_PICTURE2, &COpenCVWithMFCDlg::OnClickedPicture2)
	ON_STN_CLICKED(IDC_PICTURE3, &COpenCVWithMFCDlg::OnClickedPicture3)
	ON_STN_CLICKED(IDC_LOG_BOX, &COpenCVWithMFCDlg::OnStnClickedLogBox)
	ON_EN_CHANGE(IDC_LOG_BOX, &COpenCVWithMFCDlg::OnEnChangeLogBox)
	ON_MESSAGE(WM_APP + 1, &COpenCVWithMFCDlg::OnAddLogMessage) // 추가!
	ON_COMMAND(ID_MENU_REPLAY, &COpenCVWithMFCDlg::OnMenuReplay)
END_MESSAGE_MAP()



// COpenCVWithMFCDlg 생성
COpenCVWithMFCDlg::COpenCVWithMFCDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_OPENCVWITHMFC_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
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

	// 4개 CCTV 스트림 여는 부분 유지
	m_picture.GetWindowRect(&originalRects[0]);
	m_picture1.GetWindowRect(&originalRects[1]);
	m_picture2.GetWindowRect(&originalRects[2]);
	m_picture3.GetWindowRect(&originalRects[3]);
	camViews[0] = &m_picture;
	camViews[1] = &m_picture1;
	camViews[2] = &m_picture2;
	camViews[3] = &m_picture3;
	ScreenToClient(&originalRects[0]);
	ScreenToClient(&originalRects[1]);
	ScreenToClient(&originalRects[2]);
	ScreenToClient(&originalRects[3]);

	streamURLs = {
		"http://cctvsec.ktict.co.kr/95186/4uRjiTeOP+MlVg7IpzQotBceAIMhDdn/hDxYgs3Xkyo7zPh5aVE4AQAN4kcOSi+20QTHm1MmJlNa97qR91VibSb6cFwzj3seByeu2vhSx1I=",
		"http://cctvsec.ktict.co.kr/95185//O2jWi9UkCvVlY/u7aZQDQ3PMb1yDyjtXW4d14x0BcJuPw012wtIm9yfBYK4icnAjDy273NIrweBDb87AFfNGAiLxd+Oytufs1iIMDf/PZU=",
		"http://cctvsec.ktict.co.kr/8554/cOkcOOv8XkBcqkUcp20G+yLExUcWQMk6tFLZGaEeAJJ1UXgK8jaHkE3bGNYFvL9vs0qPEcmzdeKCMgwXCokZQ23RcizZkjvJ4qqOsUnF6wc=",
		"http://cctvsec.ktict.co.kr/8553/L6ouPHlXOysHv4BlVqI82nkiH+AmDAtz263C3hPjIe+UCS76YMGAiloQVnqpn2Ytp+9fyT1AoT2htmBsyVuppmOSpTXxiLzS0+4tKIFFMQM="
	};

	for (const auto& url : streamURLs)
	{
		cv::VideoCapture cap(url);
		if (!cap.isOpened())
		{
			CString msg;
			msg.Format(_T("CCTV 스트림을 열 수 없습니다: %s"), CString(url.c_str()));
			MessageBox(msg);
		}
		captures.push_back(std::move(cap));
	}

	SetTimer(1000, 30, NULL); // 30ms 주기로 영상 갱신

	// --- (여기부터) 로그 쓰레드 ---
	stopLogThread = false;
	logThread = std::thread([this]() {
		while (!stopLogThread) {
			Sleep(5000); // 5초마다

			for (size_t i = 0; i < captures.size(); i++) {
				if (!captures[i].isOpened())
					continue;

				cv::Mat frame;
				captures[i].read(frame);

				if (frame.empty())
					continue;

				std::string base64 = EncodeMatToBase64(frame);

				// ✅ camera_id 생성 ("cam01" ~ "cam04")
				std::ostringstream oss;
				oss << "cam" << std::setfill('0') << std::setw(2) << (i + 1);
				std::string camera_id = oss.str();

				std::string resultJson;
				if (PostFrameAndGetDetections(base64, camera_id, resultJson)) {
					if (resultJson.find("\"detections\":[]") == std::string::npos) {
						CString* pLog = new CString;
						pLog->Format(_T("#%d 카메라 분석 결과: "), (int)i + 1);

						std::string::size_type pos = 0;
						while ((pos = resultJson.find("{\"class\":\"", pos)) != std::string::npos) {
							pos += 10;
							auto end = resultJson.find("\"", pos);
							std::string className = resultJson.substr(pos, end - pos);
							pos = resultJson.find("\"confidence\":", end);
							pos += 13;
							end = resultJson.find("}", pos);
							std::string confStr = resultJson.substr(pos, end - pos);

							CString formatted;
							formatted.Format(_T("%S (%.0f%%), "), className.c_str(), std::stof(confStr) * 100);
							*pLog += formatted;
						}
						pLog->TrimRight(_T(", "));
						PostMessage(WM_APP + 1, (WPARAM)pLog);
					}
				}
			}
		}
		});

	return TRUE;
}


void COpenCVWithMFCDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
	stopLogThread = true;
	if (logThread.joinable())
		logThread.join();
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


bool COpenCVWithMFCDlg::PostFrameAndGetDetections(const std::string& b64, const std::string& camera_id, std::string& outJson)
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

	// camera_id 포함한 JSON body 생성
	std::string body = "{\"camera_id\":\"" + camera_id + "\","
	                   "\"image\":\"data:image/jpeg;base64," + b64 + "\"}";

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
	std::vector<uchar> buf;
	cv::imencode(".jpg", mat, buf);
	return Base64Encode(buf);
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
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

void COpenCVWithMFCDlg::OnMenuReplay()
{
	Replay dlg;
	dlg.DoModal();
}