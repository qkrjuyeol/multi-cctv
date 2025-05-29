#ifdef _WIN32
// ① 반드시 <winsock2.h> 먼저, 그 다음에 <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <windows.h>
#else
#include <sys/stat.h>
#include <errno.h>
#endif

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <future>
#include <map>
#include <algorithm>
#include <fstream>  // 추가됨: 로그 저장용
#include <iomanip>  // 날짜/시간 포맷팅용
#include <sstream> 

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
}

void SaveLogToCSV_UTF8(const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm timeinfo;
#ifdef _WIN32
    localtime_s(&timeinfo, &time);
#else
    localtime_r(&time, &timeinfo);
#endif

    std::ostringstream ossDate;
    ossDate << std::put_time(&timeinfo, "%Y-%m-%d");
    std::string dateStr = ossDate.str();

    std::ostringstream ossTime;
    ossTime << std::put_time(&timeinfo, "%H:%M:%S");
    std::string timeStr = ossTime.str();

    std::string logDir = "C:/logs";
#ifdef _WIN32
    CreateDirectoryA(logDir.c_str(), NULL);
#else
    mkdir(logDir.c_str(), 0777);
#endif
    std::string filePath = logDir + "/" + dateStr + ".csv";

    bool writeBOM = false;
    std::ifstream checkFile(filePath, std::ios::binary);
    if (!checkFile.good()) {
        writeBOM = true;
    }
    checkFile.close();

    std::ofstream ofs(filePath, std::ios::app | std::ios::binary);
    if (ofs.is_open()) {
        if (writeBOM) {
            ofs << "\xEF\xBB\xBF";                     // UTF-8 BOM
            ofs << "time,camera_id,event\n";           // ✅ 헤더 추가
        }

        ofs << timeStr << "," << message << "\n";
    }
}





// 전방 선언
class ONVIFCameraRecorder;

// ONVIF 카메라 설정 구조체 - 최적화된 기본값 적용
struct ONVIFCameraConfig {
    std::string name;           // 카메라 식별 이름
    std::string ip;             // IP 주소
    int port;                   // RTSP 포트 (보통 554)
    std::string username;       // 사용자 이름
    std::string password;       // 비밀번호
    std::string rtspPath;       // RTSP 경로 (ONVIF 검색으로 얻거나 수동 설정)
    std::string outputDir;      // 녹화 파일 저장 디렉토리
    int width;                  // 출력 비디오 너비
    int height;                 // 출력 비디오 높이
    int fps;                    // 프레임 레이트
    int bitrate;                // 비트레이트 (bps)
    int segmentDuration;        // 세그먼트 길이 (초, 0 = 분할 없음)
    std::string preset;         // 인코딩 프리셋 (ultrafast, veryfast, fast 등)
    bool dayNightMode;          // 주/야간 모드 자동 전환 사용 여부
    int streamPort;

    // 생성자 - 최적화된 기본값으로 변경
    ONVIFCameraConfig(
        const std::string& _name,
        const std::string& _ip,
        int _port = 8554,
        const std::string& _username = "",
        const std::string& _password = "",
        const std::string& _rtspPath = "rtsp://127.0.0.1:8554/test",
        const std::string& _outputDir = "C:/video",
        int _width = 640,            // 낮은 해상도로 변경 (854->640)
        int _height = 360,           // 낮은 해상도로 변경 (480->360)
        int _fps = 30,               // 낮은 프레임 레이트로 변경 (30)
        int _bitrate = 2000000,      // 낮은 비트레이트로 변경 (4M->2M)
        int _segmentDuration = 600,  // 10분 세그먼트 (15초->600초->15초)
        const std::string& _preset = "fast", // 더 좋은 압축률의 프리셋으로 변경 (ultrafast->fast)
        bool _dayNightMode = true,    // 기본적으로 주/야간 모드 활성화
        int _streamPort = 10000
    ) : name(_name), ip(_ip), port(_port), username(_username), password(_password),
        rtspPath(_rtspPath), outputDir(_outputDir), width(_width), height(_height),
        fps(_fps), bitrate(_bitrate), segmentDuration(_segmentDuration), preset(_preset),
        dayNightMode(_dayNightMode), streamPort(_streamPort) {
    }

    // RTSP URL 생성
    std::string getRtspUrl() const {
        return rtspPath;
    }

    // 출력 파일 이름 생성
    std::string getOutputFileName(const std::string& suffix = "") const {
        // 현재 시간 기반 파일 이름 생성
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char timeString[100];

#ifdef _WIN32
        // Windows에서는 localtime_s 사용
        struct tm timeinfo;
        localtime_s(&timeinfo, &time);
        std::strftime(timeString, sizeof(timeString), "%Y%m%d_%H%M%S", &timeinfo);
#else
        // Linux/Unix에서는 localtime 사용
        std::strftime(timeString, sizeof(timeString), "%Y%m%d_%H%M%S", std::localtime(&time));
#endif

        return outputDir + "/" + name + "_" + timeString + suffix + ".mp4";
    }

    // 주/야간 모드에 따른 비트레이트 결정
    int getCurrentBitrate() const {
        if (!dayNightMode) return bitrate;

        // 현재 시간 확인
        auto now = std::chrono::system_clock::now();
        auto time_point = std::chrono::system_clock::to_time_t(now);
        struct tm timeinfo;

#ifdef _WIN32
        localtime_s(&timeinfo, &time_point);
#else
        timeinfo = *std::localtime(&time_point);
#endif

        // 야간 시간대 (22시 ~ 06시)에는 더 낮은 비트레이트 사용
        if (timeinfo.tm_hour >= 22 || timeinfo.tm_hour < 6) {
            return bitrate / 2; // 야간에는 절반 비트레이트
        }

        return bitrate; // 주간 시간대에는 설정된 비트레이트
    }

    // 주/야간 모드에 따른 프레임 레이트 결정
    int getCurrentFps() const {
//        if (!dayNightMode) return fps;
//
//        // 현재 시간 확인
//        auto now = std::chrono::system_clock::now();
//        auto time_point = std::chrono::system_clock::to_time_t(now);
//        struct tm timeinfo;
//
//#ifdef _WIN32
//        localtime_s(&timeinfo, &time_point);
//#else
//        timeinfo = *std::localtime(&time_point);
//#endif
//
//        // 야간 시간대 (22시 ~ 06시)에는 더 낮은 프레임 레이트 사용
//        if (timeinfo.tm_hour >= 22 || timeinfo.tm_hour < 6) {
//            return max(10, fps / 2); // 야간에는 낮은 프레임 레이트, 최소 10fps
//        }
//
//        return fps; // 주간 시간대에는 설정된 프레임 레이트
        return 5;
    }
};
// 인코더 출력하는 함수
void print_encoders() {
    const AVCodec* codec = nullptr;
    void* iter = nullptr;
    while ((codec = av_codec_iterate(&iter))) {
        if (av_codec_is_encoder(codec)) {
            printf("Encoder: %s\n", codec->name);
        }
    }
}

// Forward declaration of MultiCameraRecorder for cleanupOldRecordings function
class MultiCameraRecorder;

// 카메라 레코더 클래스
class ONVIFCameraRecorder {
private:
    ONVIFCameraConfig config;
    std::atomic<bool> isRunning;
    std::atomic<bool> isPaused;
    std::thread recorderThread;
    std::mutex statusMutex;
    std::string currentFileName;
    std::atomic<int> frameCount;
    std::atomic<int> errorCount;
    std::chrono::system_clock::time_point startTime;
#ifdef _WIN32
    SOCKET listenSock = INVALID_SOCKET, clientSock = INVALID_SOCKET;
#else
    int listenSock = -1, clientSock = -1;
#endif

    bool initSocketServer(int port) {
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
        listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
        listenSock = socket(AF_INET, SOCK_STREAM, 0);
#endif
        std::cout << "[백엔드:init] socket() -> " << listenSock << std::endl;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        int b = bind(listenSock, (sockaddr*)&addr, sizeof(addr));
        if (b == SOCKET_ERROR) {
            std::cerr << "[백엔드:init] bind() failed, err=" << WSAGetLastError() << std::endl;
            return false;
        }
        std::cout << "[백엔드:init] bind() succeeded" << std::endl;

        int l = listen(listenSock, 1);
        if (l == SOCKET_ERROR) {
            std::cerr << "[백엔드:init] listen() failed, err=" << WSAGetLastError() << std::endl;
            return false;
        }
        std::cout << "[백엔드:init] listen() succeeded, listening on port " << port << std::endl;
        return true;
    }



    void handleLogMessage(const std::string& log) {
        // 예: "Camera1: AI 응답: {\"class\":\"burglary\" confidence:0.9169489145278931}"
        std::string cameraId = config.name;
        std::string eventStr = log;

        // class 추출
        std::size_t classPos = log.find("\"class\":\"");
        if (classPos != std::string::npos) {
            std::size_t start = classPos + 9; // 길이 of "class":" = 9
            std::size_t end = log.find("\"", start);
            if (end != std::string::npos) {
                std::string className = log.substr(start, end - start);
                eventStr = className + " detected";
            }
        }

        // 최종 저장 (CSV)
        SaveLogToCSV_UTF8(cameraId + "," + eventStr);
    }

    void acceptClient() {
        sockaddr_in clientAddr;
        int len = sizeof(clientAddr);
        clientSock = accept(listenSock, (sockaddr*)&clientAddr, &len);
        if (clientSock == INVALID_SOCKET) {
            std::cerr << "[" << config.name << "] 클라이언트 연결 실패: " << WSAGetLastError() << std::endl;
            return;
        }

        // 클라이언트 IP 문자열로 변환
        char ipStr[INET6_ADDRSTRLEN] = {};
        InetNtopA(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));

        std::cout << "[" << config.name << "] 클라이언트 연결 완료, clientSock=" << clientSock
            << ", peer=" << ipStr
            << ":" << ntohs(clientAddr.sin_port) << std::endl;
    }


public:
    ONVIFCameraRecorder(const ONVIFCameraConfig& cfg)
        : config(cfg), isRunning(false), isPaused(false), frameCount(0), errorCount(0) {

        // FFmpeg 네트워크 초기화 (한 번만 호출되도록 정적으로 설정)
        static std::once_flag onceFlag;
        std::call_once(onceFlag, []() {
            avformat_network_init();
            });
    }

    ~ONVIFCameraRecorder() {
        stop();
    }

    // 레코딩 시작
    bool start() {
        if (isRunning) {
            std::cout << "[" << config.name << "] 이미 실행 중입니다." << std::endl;
            return false;
        }

        // 녹화 디렉토리 확인 및 생성
        try {
            std::string dirPath = config.outputDir;
#ifdef _WIN32
            if (!CreateDirectoryA(dirPath.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
                std::cerr << "디렉토리 생성 오류. 경로를 확인하세요: " << dirPath << std::endl;
                return false;
            }
#else
            int status = mkdir(dirPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            if (status != 0 && errno != EEXIST) {
                std::cerr << "디렉토리 생성 오류. 경로를 확인하세요: " << dirPath << std::endl;
                return false;
            }
#endif
            std::cout << "저장 디렉토리 확인 완료: " << dirPath << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "디렉토리 생성 오류: " << e.what() << std::endl;
            std::cerr << "경로를 확인하세요: " << config.outputDir << std::endl;
            return false;
        }

        // ① 스트리밍 소켓 초기화
        if (!initSocketServer(config.streamPort)) {
            std::cerr << "[" << config.name << "] 스트리밍 소켓 초기화 실패\n";
        }

        // ② 클라이언트 연결(accept) – detached 스레드 대신 블로킹 호출
        // ② 클라이언트 연결(accept) – 별도 스레드에서 비동기 처리
        std::thread([this]() {
            std::cout << "[백엔드] 클라이언트 연결 대기 중(port " << config.streamPort << ")\n";

            acceptClient();  // 이 함수가 리턴되어야 clientSock가 유효

            if (clientSock != INVALID_SOCKET) {
                std::cout << "[백엔드] 클라이언트 연결 완료\n";

                std::thread([this]() {
                    char buffer[1024] = {};

                    std::cout << "[" << config.name << "] 로그 수신 스레드 시작\n";
                    while (isRunning && clientSock != INVALID_SOCKET) {
                        int received = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
                        if (received > 0) {
                            buffer[received] = '\0';  // null-terminate
                            std::string receivedStr(buffer);

                            // 로그 메시지 저장
                            handleLogMessage(receivedStr);
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // CPU 낭비 방지
                    }
                    std::cout << "[" << config.name << "] 로그 수신 스레드 종료\n";
                    }).detach();
            }
            else {
                std::cerr << "[백엔드] 클라이언트 연결 실패. 로그 수신 스레드를 시작하지 않습니다.\n";
            }
            }).detach();


        isRunning = true;
        isPaused = false;
        frameCount = 0;
        errorCount = 0;
        startTime = std::chrono::system_clock::now();

        recorderThread = std::thread(&ONVIFCameraRecorder::recordThreadFunc, this);
        return true;
    }


    // 레코딩 중지
    void stop() {
        if (isRunning) {
            isRunning = false;
            if (recorderThread.joinable()) {
                recorderThread.join();
            }
        }
    }

    // 일시 정지
    void pause() {
        isPaused = true;
    }

    // 재개
    void resume() {
        isPaused = false;
    }

    // 상태 조회
    std::map<std::string, std::string> getStatus() {
        std::lock_guard<std::mutex> lock(statusMutex);

        auto now = std::chrono::system_clock::now();
        auto runningSeconds = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();

        std::map<std::string, std::string> status;
        status["name"] = config.name;
        status["rtsp_url"] = config.getRtspUrl();
        status["current_file"] = currentFileName;
        status["running_time"] = std::to_string(runningSeconds) + "초";
        status["frames_recorded"] = std::to_string(frameCount);
        status["state"] = isRunning ? (isPaused ? "일시정지" : "녹화중") : "중지됨";
        status["error_count"] = std::to_string(errorCount);
        status["current_bitrate"] = std::to_string(config.getCurrentBitrate() / 1000) + " Kbps";
        status["current_fps"] = std::to_string(config.getCurrentFps());

        return status;
    }

private:
    // 레코딩 스레드 함수
    void recordThreadFunc() {
        std::cout << "[" << config.name << "] recordThreadFunc 시작, 초기 clientSock=" << clientSock << std::endl;

        // ② acceptClient() 가 채워줄 때까지 최대 10초간 대기
        auto waitStart = std::chrono::steady_clock::now();
        while (clientSock == INVALID_SOCKET &&
            std::chrono::steady_clock::now() - waitStart < std::chrono::seconds(10)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << "[" << config.name << "] 소켓 준비 완료, clientSock=" << clientSock << std::endl;

        std::cout << "[" << config.name << "] 테스트 시작: " << std::endl;
        std::cout << "  RTSP URL: " << config.getRtspUrl() << std::endl;
        std::cout << "  저장 경로: " << config.outputDir << std::endl;
        std::cout << "  해상도: " << config.width << "x" << config.height << std::endl;
        std::cout << "  프레임 레이트: " << config.fps << "fps" << std::endl;
        std::cout << "  비트레이트: " << config.bitrate / 1000 << " Kbps" << std::endl;
        std::cout << "  인코딩 프리셋: " << config.preset << std::endl;
        std::cout << "  주/야간 모드: " << (config.dayNightMode ? "활성화" : "비활성화") << std::endl;
        std::cout << "[" << config.name << "] 녹화 시작: " << config.getRtspUrl() << std::endl;

        // 입력 컨텍스트 초기화
        AVFormatContext* inputFormatContext = nullptr;

        // RTSP 연결 옵션 설정 - UDP 전송 방식 사용
        AVDictionary* options = nullptr;
        // UDP 전송 방식 명시적으로 설정
        av_dict_set(&options, "rtsp_transport", "udp", 0);
        av_dict_set(&options, "max_delay", "500000", 0);
        av_dict_set(&options, "stimeout", "5000000", 0);
        av_dict_set(&options, "reconnect", "1", 0);
        av_dict_set(&options, "reconnect_streamed", "1", 0);
        av_dict_set(&options, "reconnect_delay_max", "5", 0);
        // UDP 버퍼 크기 설정 (패킷 손실 방지)
        av_dict_set(&options, "buffer_size", "1024000", 0);  // 약 1MB 버퍼
        av_dict_set(&options, "reorder_queue_size", "5000", 0);  // 패킷 재정렬 큐 크기

        while (isRunning) {
            try {
                // RTSP 스트림 열기 - UDP 방식 사용
                std::cout << "[" << config.name << "] UDP 전송 방식으로 연결 시도..." << std::endl;
                int ret = avformat_open_input(&inputFormatContext, config.getRtspUrl().c_str(), nullptr, &options);

                if (ret < 0) {
                    char errBuf[AV_ERROR_MAX_STRING_SIZE];
                    av_strerror(ret, errBuf, AV_ERROR_MAX_STRING_SIZE);
                    throw std::runtime_error("UDP로 스트림을 열 수 없습니다: " + std::string(errBuf));
                }

                std::cout << "[" << config.name << "] UDP 전송 방식으로 연결 성공!" << std::endl;

                // 스트림 정보 찾기
                if (avformat_find_stream_info(inputFormatContext, nullptr) < 0) {
                    throw std::runtime_error("스트림 정보를 찾을 수 없습니다.");
                }

                // 비디오 스트림 찾기
                int videoStreamIndex = -1;
                for (unsigned int i = 0; i < inputFormatContext->nb_streams; i++) {
                    if (inputFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                        videoStreamIndex = i;
                        break;
                    }
                }

                if (videoStreamIndex == -1) {
                    throw std::runtime_error("비디오 스트림을 찾을 수 없습니다.");
                }

                // 코덱 파라미터
                AVCodecParameters* codecParameters = inputFormatContext->streams[videoStreamIndex]->codecpar;

                // 디코더 찾기
                const AVCodec* decoder = avcodec_find_decoder(codecParameters->codec_id);
                if (!decoder) {
                    throw std::runtime_error("디코더를 찾을 수 없습니다.");
                }

                // 디코더 컨텍스트
                AVCodecContext* decoderContext = avcodec_alloc_context3(decoder);
                if (!decoderContext) {
                    throw std::runtime_error("디코더 컨텍스트를 할당할 수 없습니다.");
                }

                if (avcodec_parameters_to_context(decoderContext, codecParameters) < 0) {
                    avcodec_free_context(&decoderContext);
                    throw std::runtime_error("디코더 컨텍스트를 설정할 수 없습니다.");
                }

                if (avcodec_open2(decoderContext, decoder, nullptr) < 0) {
                    avcodec_free_context(&decoderContext);
                    throw std::runtime_error("디코더를 열 수 없습니다.");
                }

                // 실제 비디오 크기 출력
                std::cout << "[" << config.name << "] 실제 비디오 해상도: "
                    << decoderContext->width << "x" << decoderContext->height << std::endl;
                const AVCodec* mjpegCodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
                AVCodecContext* mjpegCtx = avcodec_alloc_context3(mjpegCodec);
                mjpegCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
                mjpegCtx->width = decoderContext->width;
                mjpegCtx->height = decoderContext->height;
                mjpegCtx->time_base.num = 1;
                mjpegCtx->time_base.den = 30;

                avcodec_open2(mjpegCtx, mjpegCodec, NULL);

                SwsContext* swsJpegCtx = sws_getContext(
                    decoderContext->width, decoderContext->height, decoderContext->pix_fmt,
                    mjpegCtx->width, mjpegCtx->height, mjpegCtx->pix_fmt,
                    SWS_BICUBIC, NULL, NULL, NULL
                );

                AVFrame* jpegFrame = av_frame_alloc();
                jpegFrame->format = mjpegCtx->pix_fmt;
                jpegFrame->width = mjpegCtx->width;
                jpegFrame->height = mjpegCtx->height;
                av_frame_get_buffer(jpegFrame, 0);
                 
                AVPacket* jpegPkt = av_packet_alloc();

                // 현재 시간 기반 인코딩 설정 적용 (주/야간 모드)
                int currentBitrate = config.getCurrentBitrate();
                int currentFps = config.getCurrentFps();

                std::cout << "[" << config.name << "] 현재 적용 설정: "
                    << currentBitrate / 1000 << "Kbps, "
                    << currentFps << "fps" << std::endl;

                // 출력 파일 이름 설정
                std::string outputFileName = config.getOutputFileName();
                {
                    std::lock_guard<std::mutex> lock(statusMutex);
                    currentFileName = outputFileName;
                }

                // 출력 컨텍스트 초기화
                AVFormatContext* outputFormatContext = nullptr;
                if (avformat_alloc_output_context2(&outputFormatContext, nullptr, "mp4", outputFileName.c_str()) < 0) {
                    avcodec_free_context(&decoderContext);
                    throw std::runtime_error("출력 컨텍스트를 생성할 수 없습니다.");
                }

                // 소프트웨어 HEVC 인코더 사용 (하드웨어 가속 대신)
                const AVCodec* encoder = avcodec_find_encoder_by_name("h264_mf");
                if (!encoder) {
                    printf("사용 가능한 인코더 목록:\n");
                    print_encoders();  // 위에서 정의한 함수

                    avcodec_free_context(&decoderContext);
                    avformat_free_context(outputFormatContext);
                    throw std::runtime_error("H.265 인코더(hevc_mf)를 찾을 수 없습니다.");
                }

                // 출력 스트림 생성
                AVStream* outputStream = avformat_new_stream(outputFormatContext, nullptr);
                if (!outputStream) {
                    avcodec_free_context(&decoderContext);
                    avformat_free_context(outputFormatContext);
                    throw std::runtime_error("출력 스트림을 생성할 수 없습니다.");
                }

                // 인코더 컨텍스트
                AVCodecContext* encoderContext = avcodec_alloc_context3(encoder);
                if (!encoderContext) {
                    avcodec_free_context(&decoderContext);
                    avformat_free_context(outputFormatContext);
                    throw std::runtime_error("인코더 컨텍스트를 할당할 수 없습니다.");
                }

                // 인코더 설정 - h264_mf에 맞게 수정
                encoderContext->height = config.height;
                encoderContext->width = config.width;
                encoderContext->sample_aspect_ratio = decoderContext->sample_aspect_ratio;
                encoderContext->pix_fmt = AV_PIX_FMT_NV12;  // Media Foundation H.264 인코더는 NV12 형식 선호
                encoderContext->bit_rate = currentBitrate;
                encoderContext->time_base.num = 1;
                encoderContext->time_base.den = currentFps;
                encoderContext->framerate.num = currentFps;
                encoderContext->framerate.den = 1;
                encoderContext->gop_size = currentFps * 2;  // 2초마다 키프레임
                encoderContext->max_b_frames = 0;  // B-프레임 비활성화 (지연 시간 감소)

                if (outputFormatContext->oformat->flags & AVFMT_GLOBALHEADER) {
                    encoderContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
                }

                // Media Foundation 인코더 옵션
                AVDictionary* encoderOptions = nullptr;

                // h264_mf 인코더용 옵션
                av_dict_set(&encoderOptions, "rate_control", "cbr", 0);       // 일정 비트레이트
                av_dict_set(&encoderOptions, "level", "4.1", 0);              // 레벨 설정


                // Media Foundation 특화 설정
                // 키프레임 간격 설정 방법 변경
                av_dict_set(&encoderOptions, "keyint", std::to_string(currentFps * 2).c_str(), 0);  // 키프레임 간격
                // 낮은 지연 설정 시도
                av_dict_set(&encoderOptions, "delay", "0", 0);                // 지연 최소화


                // 인코더 오픈 시 옵션 전달
                ret = avcodec_open2(encoderContext, encoder, &encoderOptions);
                if (ret < 0) {
                    char errbuf[AV_ERROR_MAX_STRING_SIZE];
                    av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);

                    // 디버그 정보 출력
                    fprintf(stderr, "인코더 오픈 실패: %s\n", errbuf);

                    // 모든 옵션 제거하고 기본 설정으로 시도
                    av_dict_free(&encoderOptions);
                    encoderOptions = nullptr;

                    // 기본 설정으로 다시 시도
                    fprintf(stderr, "기본 설정으로 인코더 오픈 재시도...\n");
                    ret = avcodec_open2(encoderContext, encoder, NULL);
                    if (ret < 0) {
                        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
                        throw std::runtime_error(std::string("인코더를 열 수 없습니다: ") + errbuf);
                    }
                }

                // 사용하지 않은 옵션 확인 (디버깅용)
                if (av_dict_count(encoderOptions) > 0) {
                    char* unused_opts = NULL;
                    av_dict_get_string(encoderOptions, &unused_opts, '=', ',');
                    printf("미사용 인코더 옵션: %s\n", unused_opts);
                    av_free(unused_opts);
                }

                av_dict_free(&encoderOptions);

                // 스트림 파라미터 설정
                if (avcodec_parameters_from_context(outputStream->codecpar, encoderContext) < 0) {
                    avcodec_free_context(&encoderContext);
                    avcodec_free_context(&decoderContext);
                    avformat_free_context(outputFormatContext);
                    throw std::runtime_error("코덱 파라미터를 복사할 수 없습니다.");
                }

                outputStream->time_base = encoderContext->time_base;

                // 출력 파일 열기
                if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
                    if (avio_open(&outputFormatContext->pb, outputFileName.c_str(), AVIO_FLAG_WRITE) < 0) {
                        avcodec_free_context(&encoderContext);
                        avcodec_free_context(&decoderContext);
                        avformat_free_context(outputFormatContext);
                        throw std::runtime_error("출력 파일을 열 수 없습니다: " + outputFileName);
                    }
                }

                // 파일 헤더 쓰기
                if (avformat_write_header(outputFormatContext, nullptr) < 0) {
                    avcodec_free_context(&encoderContext);
                    avcodec_free_context(&decoderContext);
                    if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
                        avio_closep(&outputFormatContext->pb);
                    }
                    avformat_free_context(outputFormatContext);
                    throw std::runtime_error("파일 헤더를 쓸 수 없습니다.");
                }

                // 스케일러 초기화
                SwsContext* swsContext = sws_getContext(
                    decoderContext->width, decoderContext->height, decoderContext->pix_fmt,
                    encoderContext->width, encoderContext->height, encoderContext->pix_fmt,
                    SWS_BICUBIC, nullptr, nullptr, nullptr
                );

                if (!swsContext) {
                    avcodec_free_context(&encoderContext);
                    avcodec_free_context(&decoderContext);
                    if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
                        avio_closep(&outputFormatContext->pb);
                    }
                    avformat_free_context(outputFormatContext);
                    throw std::runtime_error("스케일러 컨텍스트를 초기화할 수 없습니다.");
                }

                // 패킷 및 프레임 초기화
                AVPacket* packet = av_packet_alloc();
                AVPacket* outPacket = av_packet_alloc();
                AVFrame* frame = av_frame_alloc();
                AVFrame* outFrame = av_frame_alloc();

                if (!packet || !outPacket || !frame || !outFrame) {
                    sws_freeContext(swsContext);
                    av_packet_free(&packet);
                    av_packet_free(&outPacket);
                    av_frame_free(&frame);
                    av_frame_free(&outFrame);
                    avcodec_free_context(&encoderContext);
                    avcodec_free_context(&decoderContext);
                    if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
                        avio_closep(&outputFormatContext->pb);
                    }
                    avformat_free_context(outputFormatContext);
                    throw std::runtime_error("패킷이나 프레임을 할당할 수 없습니다.");
                }

                outFrame->format = encoderContext->pix_fmt;
                outFrame->width = encoderContext->width;
                outFrame->height = encoderContext->height;

                if (av_frame_get_buffer(outFrame, 0) < 0) {
                    sws_freeContext(swsContext);
                    av_packet_free(&packet);
                    av_packet_free(&outPacket);
                    av_frame_free(&frame);
                    av_frame_free(&outFrame);
                    avcodec_free_context(&encoderContext);
                    avcodec_free_context(&decoderContext);
                    if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
                        avio_closep(&outputFormatContext->pb);
                    }
                    avformat_free_context(outputFormatContext);
                    throw std::runtime_error("출력 프레임 버퍼를 할당할 수 없습니다.");
                }

                // 녹화 시작 시간 및 세그먼트 시간 초기화
                int64_t startTime = av_gettime();
                int64_t segmentStartTime = startTime;
                int64_t pts = 0;
                int localFrameCount = 0;
                int frameSkipCounter = 0; // 프레임 스킵 카운터 (FPS 조절용)

                // 임시 로컬 변수들
                bool createNewSegment = false;
                bool reconnectRequired = false;

                // 메인 인코딩 루프
                while (isRunning) {
                    // 일시 정지 상태 확인
                    if (isPaused) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        continue;
                    }

                    // 세그먼트 시간 체크 (파일 분할)
                    int64_t currentTime = av_gettime();
                    if (config.segmentDuration > 0 &&
                        (currentTime - segmentStartTime) / 1000000 > config.segmentDuration) {
                        createNewSegment = true;
                        break;
                    }

                    // 패킷 읽기 (UDP를 통해)
                    int ret = av_read_frame(inputFormatContext, packet);

                    if (ret < 0) {
                        // 스트림 끝이거나 오류 발생
                        if (ret == AVERROR_EOF || avio_feof(inputFormatContext->pb)) {
                            std::cout << "[" << config.name << "] 스트림 끝에 도달" << std::endl;
                            break;
                        }

                        if (ret == AVERROR(EAGAIN)) {
                            // 데이터가 아직 없으므로 대기
                            std::this_thread::sleep_for(std::chrono::milliseconds(10));
                            continue;
                        }

                        // 네트워크 오류 등의 경우 재연결 필요
                        errorCount++;
                        std::cerr << "[" << config.name << "] 패킷 읽기 오류: " << ret
                            << " (에러 " << errorCount << "회)" << std::endl;

                        if (errorCount > 5) {
                            std::cerr << "[" << config.name << "] 너무 많은 오류 발생, 재연결 시도" << std::endl;
                            reconnectRequired = true;
                            break;
                        }

                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        continue;
                    }
                    else {
                        std::cout << "[" << config.name << "] 패킷 수신 성공: stream_index=" << packet->stream_index
                            << ", size=" << packet->size << std::endl;
                    }

                    // 비디오 패킷만 처리
                    if (packet->stream_index == videoStreamIndex) {
                        int fps = config.getCurrentFps();
                        if (fps <= 0) fps = 1;  // 0 또는 음수 방지

                        if (frameSkipCounter % (30 / fps) != 0) {
                            av_packet_unref(packet);
                            continue;
                        }

                        ret = avcodec_send_packet(decoderContext, packet);
                        if (ret < 0) {
                            av_packet_unref(packet);
                            continue;
                        }

                        while (ret >= 0) {
                            ret = avcodec_receive_frame(decoderContext, frame);
                            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                                break;
                            }
                            else if (ret < 0) {
                                break;
                            }
                            // --- MJPEG 인코딩 및 전송 ---
                            static int mjpegCounter = 0;
                            if (++mjpegCounter % 3 == 0) {  // 3프레임에 1번만 전송 (약 10fps)

                                sws_scale(
                                    swsJpegCtx,
                                    frame->data, frame->linesize,
                                    0, frame->height,
                                    jpegFrame->data, jpegFrame->linesize
                                );
                                jpegFrame->pts = frame->pts;

                                // MJPEG 인코딩
                                int ret = avcodec_send_frame(mjpegCtx, jpegFrame);
                                if (ret < 0) {
                                    std::cerr << "[MJPEG 인코딩] avcodec_send_frame 실패: " << ret << std::endl;
                                }

                                ret = avcodec_receive_packet(mjpegCtx, jpegPkt);
                                if (ret == AVERROR(EAGAIN)) {
                                    std::cerr << "[MJPEG 인코딩] MJPEG 패킷 준비 안됨(EAGAIN)" << std::endl;
                                }
                                else if (ret == AVERROR_EOF) {
                                    std::cerr << "[MJPEG 인코딩] MJPEG 인코더 EOF 상태" << std::endl;
                                }
                                else if (ret < 0) {
                                    std::cerr << "[MJPEG 인코딩] MJPEG 인코딩 실패: " << ret << std::endl;
                                }
                                else {
                                    std::cout << "[MJPEG 인코딩] MJPEG 패킷 생성 성공, 크기: " << jpegPkt->size << std::endl;

                                    if (clientSock != INVALID_SOCKET) {
                                        uint32_t netLen = htonl(jpegPkt->size);

                                        int sentPrefix = send(clientSock, reinterpret_cast<char*>(&netLen), sizeof(netLen), 0);
                                        std::cout << "[TCP 전송] prefix 전송: " << sentPrefix
                                            << " 바이트, 에러코드: " << WSAGetLastError() << std::endl;

                                        int sentBody = send(clientSock, reinterpret_cast<char*>(jpegPkt->data), jpegPkt->size, 0);
                                        std::cout << "[TCP 전송] body 전송: " << sentBody
                                            << " 바이트, 에러코드: " << WSAGetLastError() << std::endl;

                                        av_packet_unref(jpegPkt);
                                    }
                                }
                            }


                            // 프레임 변환
                            sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height,
                                outFrame->data, outFrame->linesize);

                            outFrame->pts = pts++;

                            // 인코딩
                            ret = avcodec_send_frame(encoderContext, outFrame);
                            if (ret < 0) {
                                break;
                            }

                            while (ret >= 0) {
                                ret = avcodec_receive_packet(encoderContext, outPacket);
                                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                                    break;
                                }
                                else if (ret < 0) {
                                    break;
                                }

                                outPacket->stream_index = 0;
                                av_packet_rescale_ts(outPacket, encoderContext->time_base, outputStream->time_base);

                                ret = av_interleaved_write_frame(outputFormatContext, outPacket);
                                if (ret < 0) {
                                    break;
                                }

                                localFrameCount++;
                                frameCount++;

                                // 진행 상황 표시 (500프레임마다)
                                if (localFrameCount % 500 == 0) {
                                    int64_t elapsedMs = (av_gettime() - startTime) / 1000;
                                    float fps = (elapsedMs > 0) ? (localFrameCount * 1000.0f / elapsedMs) : 0;
                                    std::cout << "[" << config.name << "] 프레임: " << localFrameCount
                                        << ", FPS: " << fps << std::endl;
                                }
                            }
                        }
                    }

                    av_packet_unref(packet);
                }

                // 남은 프레임 플러시
                avcodec_send_frame(encoderContext, nullptr);
                while (true) {
                    int ret = avcodec_receive_packet(encoderContext, outPacket);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                        break;
                    }
                    else if (ret < 0) {
                        break;
                    }

                    outPacket->stream_index = 0;
                    av_packet_rescale_ts(outPacket, encoderContext->time_base, outputStream->time_base);

                    av_interleaved_write_frame(outputFormatContext, outPacket);
                    frameCount++;
                }

                // 파일 트레일러 쓰기
                av_write_trailer(outputFormatContext);

                // 리소스 정리
                sws_freeContext(swsContext);
                av_packet_free(&packet);
                av_packet_free(&outPacket);
                av_frame_free(&frame);
                av_frame_free(&outFrame);
                avcodec_free_context(&encoderContext);
                avcodec_free_context(&decoderContext);

                if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
                    avio_closep(&outputFormatContext->pb);
                }

                avformat_free_context(outputFormatContext);
                avformat_close_input(&inputFormatContext);

                // 진행 상황 및 상태 업데이트
                std::cout << "[" << config.name << "] "
                    << (createNewSegment ? "세그먼트 완료" : "녹화 완료")
                    << ": " << outputFileName << " (" << localFrameCount << " 프레임)" << std::endl;

                // 세그먼트 모드에서 새 세그먼트 생성
                if (createNewSegment && isRunning) {
                    segmentStartTime = av_gettime();
                    continue; // 새 세그먼트를 위해 루프 계속
                }

                // 재연결 필요 시 처리
                if (reconnectRequired) {
                    std::cout << "[" << config.name << "] 재연결 중..." << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(2)); // 약간의 지연 후 재시도
                    continue;
                }

                // 다른 상황에서는 루프 종료
                break;
            }
            catch (const std::exception& e) {
                errorCount++;
                std::cerr << "[" << config.name << "] 오류 발생: " << e.what() << std::endl;

                // 입력 컨텍스트가 열려 있으면 닫기
                if (inputFormatContext) {
                    avformat_close_input(&inputFormatContext);
                }

                // 잠시 대기 후 재시도
                std::this_thread::sleep_for(std::chrono::seconds(5));

                if (errorCount > 20) {
                    std::cerr << "[" << config.name << "] 오류가 너무 많이 발생하여 녹화를 중단합니다." << std::endl;
                    break;
                }
            }
        }

        std::cout << "[" << config.name << "] 녹화 스레드 종료" << std::endl;
    }
};

// 다중 카메라 관리 클래스
class MultiCameraRecorder {
private:
    std::vector<ONVIFCameraConfig> cameraConfigs;
    std::vector<std::unique_ptr<ONVIFCameraRecorder>> recorders;

    // 오래된 녹화 파일 정리 함수
    void cleanupOldRecordings(const std::string& directory, int daysToKeep) {
        std::cout << "오래된 녹화 파일 정리 중... (" << daysToKeep << "일 이상 보관)" << std::endl;

        // 현재 시간 계산
        auto now = std::chrono::system_clock::now();
        auto cutoffTime = now - std::chrono::hours(24 * daysToKeep);
        auto cutoffTimeT = std::chrono::system_clock::to_time_t(cutoffTime);

#ifdef _WIN32
        // Windows에서 디렉토리 탐색
        WIN32_FIND_DATAA findData;
        std::string searchPath = directory + "\\*.mp4";
        HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                std::string filename = findData.cFileName;
                std::string fullPath = directory + "\\" + filename;

                // 파일 시간 확인
                FILETIME ftCreate, ftAccess, ftWrite;
                HANDLE hFile = CreateFileA(fullPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, 0, NULL);
                if (hFile != INVALID_HANDLE_VALUE) {
                    if (GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite)) {
                        ULARGE_INTEGER uli;
                        uli.LowPart = ftWrite.dwLowDateTime;
                        uli.HighPart = ftWrite.dwHighDateTime;

                        // 파일 시간을 time_t로 변환 (Windows FILETIME은 1601년부터 시작, time_t는 1970년부터 시작)
                        const int64_t WINDOWS_TICK = 10000000;
                        const int64_t SEC_TO_UNIX_EPOCH = 11644473600LL;
                        time_t fileTime = (uli.QuadPart / WINDOWS_TICK - SEC_TO_UNIX_EPOCH);

                        // 오래된 파일 삭제
                        if (fileTime < cutoffTimeT) {
                            CloseHandle(hFile);
                            if (DeleteFileA(fullPath.c_str())) {
                                std::cout << "삭제됨: " << filename << std::endl;
                            }
                            else {
                                std::cerr << "삭제 실패: " << filename << std::endl;
                            }
                            continue;
                        }
                    }
                    CloseHandle(hFile);
                }
            } while (FindNextFileA(hFind, &findData));
            FindClose(hFind);
        }
#else
        // Linux/Unix 시스템에서 디렉토리 탐색 (dirent.h 사용)
        DIR* dir = opendir(directory.c_str());
        if (dir != NULL) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != NULL) {
                std::string filename = entry->d_name;

                // mp4 파일만 처리
                if (filename.length() < 4 || filename.substr(filename.length() - 4) != ".mp4") {
                    continue;
                }

                std::string fullPath = directory + "/" + filename;

                // 파일 정보 확인
                struct stat fileStat;
                if (stat(fullPath.c_str(), &fileStat) == 0) {
                    // 수정 시간 확인
                    if (fileStat.st_mtime < cutoffTimeT) {
                        if (remove(fullPath.c_str()) == 0) {
                            std::cout << "삭제됨: " << filename << std::endl;
                        }
                        else {
                            std::cerr << "삭제 실패: " << filename << std::endl;
                        }
                    }
                }
            }
            closedir(dir);
        }
#endif
    }

public:
    // 정기적인 유지 관리 작업 수행 (디스크 공간 관리)
    void performMaintenance() {
        std::cout << "디스크 유지 관리 작업 실행 중..." << std::endl;

        // 각 카메라 디렉토리 정리
        for (const auto& config : cameraConfigs) {
            cleanupOldRecordings(config.outputDir, 7);
        }
    }

    // 카메라 구성 추가
    void addCamera(const ONVIFCameraConfig& config) {
        cameraConfigs.push_back(config);
    }

    // 모든 카메라 녹화 시작
    void startAll() {
        for (const auto& config : cameraConfigs) {
            auto recorder = std::make_unique<ONVIFCameraRecorder>(config);
            if (recorder->start()) {
                recorders.push_back(std::move(recorder));
            }
        }

        // 오래된 파일 정리 (7일 이상 된 파일)
        for (const auto& config : cameraConfigs) {
            cleanupOldRecordings(config.outputDir, 7);
        }
    }

    // 모든 카메라 녹화 중지
    void stopAll() {
        for (auto& recorder : recorders) {
            recorder->stop();
        }
        recorders.clear();
    }

    // 모든 카메라 상태 출력
    void printAllStatus() {
        std::cout << "===== 카메라 상태 =====" << std::endl;
        for (auto& recorder : recorders) {
            auto status = recorder->getStatus();
            std::cout << "카메라: " << status["name"] << std::endl;
            std::cout << "  상태: " << status["state"] << std::endl;
            std::cout << "  현재 파일: " << status["current_file"] << std::endl;
            std::cout << "  프레임 수: " << status["frames_recorded"] << std::endl;
            std::cout << "  실행 시간: " << status["running_time"] << std::endl;
            std::cout << "  현재 비트레이트: " << status["current_bitrate"] << std::endl;
            std::cout << "  현재 FPS: " << status["current_fps"] << std::endl;
            std::cout << "  오류 수: " << status["error_count"] << std::endl;
            std::cout << std::endl;
        }
    }

    // 디스크 사용량 출력
    void printDiskUsage() {
        std::cout << "===== 디스크 사용량 =====" << std::endl;

        for (const auto& config : cameraConfigs) {
            std::string directory = config.outputDir;
            uint64_t totalSize = 0;
            int fileCount = 0;

#ifdef _WIN32
            // Windows에서 디렉토리 내 파일 크기 계산
            WIN32_FIND_DATAA findData;
            std::string searchPath = directory + "\\*.mp4";
            HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        ULARGE_INTEGER fileSize;
                        fileSize.LowPart = findData.nFileSizeLow;
                        fileSize.HighPart = findData.nFileSizeHigh;
                        totalSize += fileSize.QuadPart;
                        fileCount++;
                    }
                } while (FindNextFileA(hFind, &findData));
                FindClose(hFind);
            }
#else
            // Linux/Unix 시스템에서 디렉토리 내 파일 크기 계산
            DIR* dir = opendir(directory.c_str());
            if (dir != NULL) {
                struct dirent* entry;
                while ((entry = readdir(dir)) != NULL) {
                    std::string filename = entry->d_name;

                    // mp4 파일만 처리
                    if (filename.length() < 4 || filename.substr(filename.length() - 4) != ".mp4") {
                        continue;
                    }

                    std::string fullPath = directory + "/" + filename;

                    // 파일 크기 확인
                    struct stat fileStat;
                    if (stat(fullPath.c_str(), &fileStat) == 0) {
                        totalSize += fileStat.st_size;
                        fileCount++;
                    }
                }
                closedir(dir);
            }
#endif

            // MB 및 GB 단위로 변환하여 출력
            double sizeInMB = totalSize / (1024.0 * 1024.0);
            double sizeInGB = sizeInMB / 1024.0;

            std::cout << "카메라 " << config.name << " (" << directory << "):" << std::endl;
            std::cout << "  파일 수: " << fileCount << std::endl;
            std::cout << "  총 용량: " << sizeInMB << " MB (" << sizeInGB << " GB)" << std::endl;
            std::cout << std::endl;
        }
    }

};

// 메인 함수
int main() {
    MultiCameraRecorder multiRecorder;

    multiRecorder.addCamera(ONVIFCameraConfig("Camera1", "192.168.50.35", 554, "admin", "Windo4101!", "rtsp://admin:Windo4101!@192.168.50.35:554/stream1", "C:/video", 640, 360, 30, 2000000, 600, "fast", true, 10000));
    multiRecorder.addCamera(ONVIFCameraConfig("Camera2", "192.168.50.164", 554, "admin", "Windo4101!", "rtsp://admin:Windo4101!@192.168.50.164:554/stream1", "C:/video", 640, 360, 30, 2000000, 600, "fast", true, 10001));
    multiRecorder.addCamera(ONVIFCameraConfig("Camera3", "192.168.50.39", 554, "admin", "Windo4101!", "rtsp://admin:Windo4101!@192.168.50.39:554/stream1", "C:/video", 640, 360, 30, 2000000, 600, "fast", true, 10002));
    multiRecorder.addCamera(ONVIFCameraConfig("Camera4", "192.168.50.157", 554, "admin", "Windo4101!", "rtsp://admin:Windo4101!@192.168.50.157:554/stream1", "C:/video", 640, 360, 30, 2000000, 600, "fast", true, 10003));

    // 모든 카메라 녹화 시작
    std::cout << "녹화를 시작합니다..." << std::endl;
    multiRecorder.startAll();

    // 메인 루프
    bool running = true;
    while (running) {
        std::cout << "\n명령어: status(상태), disk(디스크 사용량), clean(파일 정리), stop(중지), quit(종료)" << std::endl;
        std::string command;
        std::getline(std::cin, command);

        if (command == "status") {
            multiRecorder.printAllStatus();
        }
        else if (command == "disk") {
            multiRecorder.printDiskUsage();
        }
        else if (command == "clean") {
            multiRecorder.performMaintenance();
        }
        else if (command == "stop") {
            std::cout << "녹화를 중지합니다..." << std::endl;
            multiRecorder.stopAll();
            running = false;
        }
        else if (command == "quit") {
            std::cout << "프로그램을 종료합니다..." << std::endl;
            multiRecorder.stopAll();
            running = false;
        }
        else {
            std::cout << "알 수 없는 명령어입니다. 다시 시도하세요." << std::endl;
        }
    }

    return 0;
}
