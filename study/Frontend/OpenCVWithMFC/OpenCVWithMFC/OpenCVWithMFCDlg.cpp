
// OpenCVWithMFCDlg.cpp: 구현 파일
//

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

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

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
	ON_STN_CLICKED(IDC_PICTURE, &COpenCVWithMFCDlg::OnStnClickedPicture)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_STN_CLICKED(IDC_PICTURE3, &COpenCVWithMFCDlg::OnStnClickedPicture3)
	ON_STN_CLICKED(IDC_LOG_BOX, &COpenCVWithMFCDlg::OnStnClickedLogBox)
END_MESSAGE_MAP()


// COpenCVWithMFCDlg 메시지 처리기

BOOL COpenCVWithMFCDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// CCTV 스트림 URL 리스트
	streamURLs = {
		"http://cctvsec.ktict.co.kr/138/7ZIeMPWKXQSsPPtEk/L7cZD32MojYyR+t2aPMLmTGIvQwu3zmjLddC2Kk6HC2YxjLZtdOIAiAEpLComas04c/IcJ9jNGE5Bx51hdStrzVl0=",
		"http://cctvsec.ktict.co.kr/139/YdKKm/oXGB3YG8GJZiiEZUcYFycOZHiyC5eDZjSz6u5xVq1J1yi/pMC78nJ4+8eDlOOBw+/Xd+pjatRk5d20oC7Alc2wqHQc+7ZYJvrR/Hg=",
		"http://cctvsec.ktict.co.kr/141/9NZjrrlOAEqKrjdlyKbr6j4jpGkg2cKZq1x5xc1BiakObXU1o2B8j978DWJpUKIrFecaj3D699UWlvCPYjg71MgyAoqpZOQk8TjPqyq8sCM=",
		"http://cctvsec.ktict.co.kr/2060/KvR9/XSs58LVgCeJEdoCjnaRVu53Rg2kqseQsB6HwtzweyNenOcXwQoIkzMM7qVQQd1YhFrV9WwP75W+jb/JnJ//n4j4C66RsPlDBPMep4M="
	};

	// URL 개수만큼 VideoCapture 객체 생성
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

	SetTimer(1000, 30, NULL); // 30ms마다 프레임 업데이트

	return TRUE;
}

void COpenCVWithMFCDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void COpenCVWithMFCDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR COpenCVWithMFCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void COpenCVWithMFCDlg::OnStnClickedPicture()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}


void COpenCVWithMFCDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}




void COpenCVWithMFCDlg::OnTimer(UINT_PTR nIDEvent)
{
	std::vector<cv::Mat> frames(captures.size());

	for (size_t i = 0; i < captures.size(); i++)
	{
		if (!captures[i].isOpened()) continue;  // 스트림이 열려있는지 확인

		captures[i].read(frames[i]);
		if (frames[i].empty()) continue;

		// 영상 출력
		switch (i)
		{
		case 0: DisplayFrame(frames[i], m_picture); break;
		case 1: DisplayFrame(frames[i], m_picture1); break;
		case 2: DisplayFrame(frames[i], m_picture2); break;
		case 3: DisplayFrame(frames[i], m_picture3); break;
		default: break;
		}
	}

	CDialogEx::OnTimer(nIDEvent);
}

void COpenCVWithMFCDlg::DisplayFrame(cv::Mat& frame, CStatic& pictureControl)
{
	if (frame.empty()) return;

	int bpp = 8 * frame.elemSize();
	int border = (bpp < 32) ? 4 - (frame.cols % 4) : 0;
	if (border == 4) border = 0;

	cv::Mat temp;
	if (border > 0 || frame.isContinuous() == false)
		cv::copyMakeBorder(frame, temp, 0, 0, 0, border, cv::BORDER_CONSTANT, 0);
	else
		temp = frame;

	RECT r;
	pictureControl.GetClientRect(&r);
	cv::Size winSize(r.right, r.bottom);

	CImage cimage_mfc;
	cimage_mfc.Create(winSize.width, winSize.height, 24);

	BITMAPINFO* bitInfo = (BITMAPINFO*)malloc(sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD));
	bitInfo->bmiHeader.biBitCount = bpp;
	bitInfo->bmiHeader.biWidth = temp.cols;
	bitInfo->bmiHeader.biHeight = -temp.rows;
	bitInfo->bmiHeader.biPlanes = 1;
	bitInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitInfo->bmiHeader.biCompression = BI_RGB;

	if (bpp == 8) {
		RGBQUAD* palette = bitInfo->bmiColors;
		for (int i = 0; i < 256; i++) {
			palette[i].rgbBlue = palette[i].rgbGreen = palette[i].rgbRed = (BYTE)i;
			palette[i].rgbReserved = 0;
		}
	}

	StretchDIBits(cimage_mfc.GetDC(),
		0, 0, winSize.width, winSize.height,
		0, 0, temp.cols - border, temp.rows,
		temp.data, bitInfo, DIB_RGB_COLORS, SRCCOPY);

	HDC dc = ::GetDC(pictureControl.m_hWnd);
	cimage_mfc.BitBlt(dc, 0, 0);
	::ReleaseDC(pictureControl.m_hWnd, dc);
	cimage_mfc.ReleaseDC();
	cimage_mfc.Destroy();
}


void COpenCVWithMFCDlg::OnStnClickedPicture3()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}


void COpenCVWithMFCDlg::OnStnClickedLogBox()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}
