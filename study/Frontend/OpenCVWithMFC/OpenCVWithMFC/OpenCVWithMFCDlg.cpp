// OpenCVWithMFCDlg.cpp: 구현 파일

#include "pch.h"
#include "framework.h"
#include "OpenCVWithMFC.h"
#include "OpenCVWithMFCDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// COpenCVWithMFCDlg 대화 상자

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
END_MESSAGE_MAP()

// COpenCVWithMFCDlg 메시지 처리기

BOOL COpenCVWithMFCDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_logBox.SubclassDlgItem(IDC_LOG_BOX, this);

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
		"http://cctvsec.ktict.co.kr/138/7ZIeMPWKXQSsPPtEk/L7cZD32MojYyR+t2aPMLmTGIvQwu3zmjLddC2Kk6HC2YxjLZtdOIAiAEpLComas04c/IcJ9jNGE5Bx51hdStrzVl0=",
		"http://cctvsec.ktict.co.kr/139/YdKKm/oXGB3YG8GJZiiEZUcYFycOZHiyC5eDZjSz6u5xVq1J1yi/pMC78nJ4+8eDlOOBw+/Xd+pjatRk5d20oC7Alc2wqHQc+7ZYJvrR/Hg=",
		"http://cctvsec.ktict.co.kr/141/9NZjrrlOAEqKrjdlyKbr6j4jpGkg2cKZq1x5xc1BiakObXU1o2B8j978DWJpUKIrFecaj3D699UWlvCPYjg71MgyAoqpZOQk8TjPqyq8sCM=",
		"http://cctvsec.ktict.co.kr/2060/KvR9/XSs58LVgCeJEdoCjnaRVu53Rg2kqseQsB6HwtzweyNenOcXwQoIkzMM7qVQQd1YhFrV9WwP75W+jb/JnJ//n4j4C66RsPlDBPMep4M="
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

	SetTimer(1000, 30, NULL);

	return TRUE;
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

HCURSOR COpenCVWithMFCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void COpenCVWithMFCDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
}

void COpenCVWithMFCDlg::OnTimer(UINT_PTR nIDEvent)
{
	std::vector<cv::Mat> frames(captures.size());

	for (size_t i = 0; i < captures.size(); i++)
	{
		if (!captures[i].isOpened()) {
			std::cout << "Camera " << i << " is not opened!" << std::endl;
			continue;
		}

		captures[i].read(frames[i]);

		if (frames[i].empty()) {
			std::cout << "Failed to read frame from camera " << i << std::endl;
			continue;
		}

		switch (i)
		{
		case 0: DisplayFrame(frames[i], m_picture); break;
		case 1: DisplayFrame(frames[i], m_picture1); break;
		case 2: DisplayFrame(frames[i], m_picture2); break;
		case 3: DisplayFrame(frames[i], m_picture3); break;
		}
	}

	CDialogEx::OnTimer(nIDEvent);
}

void COpenCVWithMFCDlg::DisplayFrame(cv::Mat& frame, CStatic& pictureControl)
{
	if (frame.empty()) return;

	// 1. Image Control 크기 가져오기
	CRect rect;
	pictureControl.GetClientRect(&rect);
	int targetWidth = rect.Width();
	int targetHeight = rect.Height();

	// 2. 프레임 크기 조정 (CStatic 크기에 맞춤)
	cv::Mat frameRGB;
	cv::resize(frame, frameRGB, cv::Size(targetWidth, targetHeight));
	cv::cvtColor(frameRGB, frameRGB, cv::COLOR_BGR2RGB);

	std::cout << "Resized Frame size: " << frameRGB.cols << "x" << frameRGB.rows << std::endl;

	// 3. OpenCV Mat → CImage 변환
	CImage cimage_mfc;
	cimage_mfc.Create(frameRGB.cols, frameRGB.rows, 24);

	uchar* pBuffer = (uchar*)cimage_mfc.GetBits();
	int step = cimage_mfc.GetPitch();
	for (int y = 0; y < frameRGB.rows; y++)
		memcpy(pBuffer + y * step, frameRGB.ptr(y), frameRGB.cols * 3);

	// 4. CStatic 컨트롤에 출력
	HDC dc = ::GetDC(pictureControl.m_hWnd);
	cimage_mfc.Draw(dc, 0, 0);
	::ReleaseDC(pictureControl.m_hWnd, dc);
}


void COpenCVWithMFCDlg::ToggleZoom(int index)
{
	if (isZoomed[index])
	{
		// ✅ 원래 크기로 복원
		for (int i = 0; i < 4; i++)
		{
			camViews[i]->MoveWindow(&originalRects[i]);
			camViews[i]->ShowWindow(SW_SHOW);  // 숨겼던 화면 다시 표시
		}
	}
	else
	{
		// ✅ 다이얼로그 전체 크기 가져오기
		CRect dialogRect;
		GetClientRect(&dialogRect);

		// ✅ 텍스트 박스(`IDC_LOG_BOX`)의 위치를 가져오기
		CRect logBoxRect;
		m_logBox.GetWindowRect(&logBoxRect);
		ScreenToClient(&logBoxRect); // 다이얼로그 좌표계로 변환

		// ✅ 클릭한 화면을 텍스트 박스를 제외한 영역으로 확대
		CRect zoomRect = dialogRect;
		zoomRect.right = logBoxRect.left - 10;  // 텍스트 박스를 침범하지 않도록 조정

		camViews[index]->MoveWindow(&zoomRect);

		// ✅ 나머지 카메라는 숨기기
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
	ToggleZoom(0);
}

void COpenCVWithMFCDlg::OnClickedPicture1()
{
	std::cout << "Picture clicked1!" << std::endl;  // 디버깅용 출력
	ToggleZoom(1);
}

void COpenCVWithMFCDlg::OnClickedPicture2()
{
	std::cout << "Picture clicked2!" << std::endl;  // 디버깅용 출력
	ToggleZoom(2);
}

void COpenCVWithMFCDlg::OnClickedPicture3()
{
	std::cout << "Picture clicked3!" << std::endl;  // 디버깅용 출력
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