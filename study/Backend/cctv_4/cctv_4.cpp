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
#include <algorithm>  // std::max �Լ��� ����ϱ� ���� �ʿ�

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <errno.h>
#endif

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
}

// ���� ����
class ONVIFCameraRecorder;

// ONVIF ī�޶� ���� ����ü - ����ȭ�� �⺻�� ����
struct ONVIFCameraConfig {
    std::string name;           // ī�޶� �ĺ� �̸�
    std::string ip;             // IP �ּ�
    int port;                   // RTSP ��Ʈ (���� 554)
    std::string username;       // ����� �̸�
    std::string password;       // ��й�ȣ
    std::string rtspPath;       // RTSP ��� (ONVIF �˻����� ��ų� ���� ����)
    std::string outputDir;      // ��ȭ ���� ���� ���丮
    int width;                  // ��� ���� �ʺ�
    int height;                 // ��� ���� ����
    int fps;                    // ������ ����Ʈ
    int bitrate;                // ��Ʈ����Ʈ (bps)
    int segmentDuration;        // ���׸�Ʈ ���� (��, 0 = ���� ����)
    std::string preset;         // ���ڵ� ������ (ultrafast, veryfast, fast ��)
    bool dayNightMode;          // ��/�߰� ��� �ڵ� ��ȯ ��� ����

    // ������ - ����ȭ�� �⺻������ ����
    ONVIFCameraConfig(
        const std::string& _name,
        const std::string& _ip,
        int _port = 8554,
        const std::string& _username = "",
        const std::string& _password = "",
        const std::string& _rtspPath = "rtsp://127.0.0.1:8554/test",
        const std::string& _outputDir = "C:/Download/savedfiles",
        int _width = 640,            // ���� �ػ󵵷� ���� (854->640)
        int _height = 360,           // ���� �ػ󵵷� ���� (480->360)
        int _fps = 30,               // ���� ������ ����Ʈ�� ���� (30)
        int _bitrate = 2000000,      // ���� ��Ʈ����Ʈ�� ���� (4M->2M)
        int _segmentDuration = 600,  // 10�� ���׸�Ʈ (15��->600��->15��)
        const std::string& _preset = "fast", // �� ���� ������� ���������� ���� (ultrafast->fast)
        bool _dayNightMode = true    // �⺻������ ��/�߰� ��� Ȱ��ȭ
    ) : name(_name), ip(_ip), port(_port), username(_username), password(_password),
        rtspPath(_rtspPath), outputDir(_outputDir), width(_width), height(_height),
        fps(_fps), bitrate(_bitrate), segmentDuration(_segmentDuration), preset(_preset),
        dayNightMode(_dayNightMode) {
    }

    // RTSP URL ����
    std::string getRtspUrl() const {
        return rtspPath;
    }

    // ��� ���� �̸� ����
    std::string getOutputFileName(const std::string& suffix = "") const {
        // ���� �ð� ��� ���� �̸� ����
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char timeString[100];

#ifdef _WIN32
        // Windows������ localtime_s ���
        struct tm timeinfo;
        localtime_s(&timeinfo, &time);
        std::strftime(timeString, sizeof(timeString), "%Y%m%d_%H%M%S", &timeinfo);
#else
        // Linux/Unix������ localtime ���
        std::strftime(timeString, sizeof(timeString), "%Y%m%d_%H%M%S", std::localtime(&time));
#endif

        return outputDir + "/" + name + "_" + timeString + suffix + ".mp4";
    }

    // ��/�߰� ��忡 ���� ��Ʈ����Ʈ ����
    int getCurrentBitrate() const {
        if (!dayNightMode) return bitrate;

        // ���� �ð� Ȯ��
        auto now = std::chrono::system_clock::now();
        auto time_point = std::chrono::system_clock::to_time_t(now);
        struct tm timeinfo;

#ifdef _WIN32
        localtime_s(&timeinfo, &time_point);
#else
        timeinfo = *std::localtime(&time_point);
#endif

        // �߰� �ð��� (22�� ~ 06��)���� �� ���� ��Ʈ����Ʈ ���
        if (timeinfo.tm_hour >= 22 || timeinfo.tm_hour < 6) {
            return bitrate / 2; // �߰����� ���� ��Ʈ����Ʈ
        }

        return bitrate; // �ְ� �ð��뿡�� ������ ��Ʈ����Ʈ
    }

    // ��/�߰� ��忡 ���� ������ ����Ʈ ����
    int getCurrentFps() const {
        if (!dayNightMode) return fps;

        // ���� �ð� Ȯ��
        auto now = std::chrono::system_clock::now();
        auto time_point = std::chrono::system_clock::to_time_t(now);
        struct tm timeinfo;

#ifdef _WIN32
        localtime_s(&timeinfo, &time_point);
#else
        timeinfo = *std::localtime(&time_point);
#endif

        // �߰� �ð��� (22�� ~ 06��)���� �� ���� ������ ����Ʈ ���
        if (timeinfo.tm_hour >= 22 || timeinfo.tm_hour < 6) {
            return max(10, fps / 2); // �߰����� ���� ������ ����Ʈ, �ּ� 10fps
        }

        return fps; // �ְ� �ð��뿡�� ������ ������ ����Ʈ
    }
};
// ���ڴ� ����ϴ� �Լ�
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

// ī�޶� ���ڴ� Ŭ����
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

public:
    ONVIFCameraRecorder(const ONVIFCameraConfig& cfg)
        : config(cfg), isRunning(false), isPaused(false), frameCount(0), errorCount(0) {

        // FFmpeg ��Ʈ��ũ �ʱ�ȭ (�� ���� ȣ��ǵ��� �������� ����)
        static std::once_flag onceFlag;
        std::call_once(onceFlag, []() {
            avformat_network_init();
            });
    }

    ~ONVIFCameraRecorder() {
        stop();
    }

    // ���ڵ� ����
    bool start() {
        if (isRunning) {
            std::cout << "[" << config.name << "] �̹� ���� ���Դϴ�." << std::endl;
            return false;
        }

        // ��ȭ ���丮 Ȯ�� �� ����
        try {
            // ���丮 ��� Ȯ�� �� ����
            std::string dirPath = config.outputDir;

            // ���丮�� �����ϴ��� Ȯ���ϰ� ������ ����
#ifdef _WIN32
            // Windows �ý��� �Լ� ���
            if (!CreateDirectoryA(dirPath.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
                std::cerr << "���丮 ���� ����. ��θ� Ȯ���ϼ���: " << dirPath << std::endl;
                return false;
            }
#else
            // Linux/Unix �ý��ۿ� mkdir ���
            int status = mkdir(dirPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            if (status != 0 && errno != EEXIST) {
                std::cerr << "���丮 ���� ����. ��θ� Ȯ���ϼ���: " << dirPath << std::endl;
                return false;
            }
#endif
            std::cout << "���� ���丮 Ȯ�� �Ϸ�: " << dirPath << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "���丮 ���� ����: " << e.what() << std::endl;
            std::cerr << "��θ� Ȯ���ϼ���: " << config.outputDir << std::endl;
            return false;
        }

        isRunning = true;
        isPaused = false;
        frameCount = 0;
        errorCount = 0;
        startTime = std::chrono::system_clock::now();

        recorderThread = std::thread(&ONVIFCameraRecorder::recordThreadFunc, this);
        return true;
    }

    // ���ڵ� ����
    void stop() {
        if (isRunning) {
            isRunning = false;
            if (recorderThread.joinable()) {
                recorderThread.join();
            }
        }
    }

    // �Ͻ� ����
    void pause() {
        isPaused = true;
    }

    // �簳
    void resume() {
        isPaused = false;
    }

    // ���� ��ȸ
    std::map<std::string, std::string> getStatus() {
        std::lock_guard<std::mutex> lock(statusMutex);

        auto now = std::chrono::system_clock::now();
        auto runningSeconds = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();

        std::map<std::string, std::string> status;
        status["name"] = config.name;
        status["rtsp_url"] = config.getRtspUrl();
        status["current_file"] = currentFileName;
        status["running_time"] = std::to_string(runningSeconds) + "��";
        status["frames_recorded"] = std::to_string(frameCount);
        status["state"] = isRunning ? (isPaused ? "�Ͻ�����" : "��ȭ��") : "������";
        status["error_count"] = std::to_string(errorCount);
        status["current_bitrate"] = std::to_string(config.getCurrentBitrate() / 1000) + " Kbps";
        status["current_fps"] = std::to_string(config.getCurrentFps());

        return status;
    }

private:
    // ���ڵ� ������ �Լ�
    void recordThreadFunc() {
        std::cout << "[" << config.name << "] �׽�Ʈ ����: " << std::endl;
        std::cout << "  RTSP URL: " << config.getRtspUrl() << std::endl;
        std::cout << "  ���� ���: " << config.outputDir << std::endl;
        std::cout << "  �ػ�: " << config.width << "x" << config.height << std::endl;
        std::cout << "  ������ ����Ʈ: " << config.fps << "fps" << std::endl;
        std::cout << "  ��Ʈ����Ʈ: " << config.bitrate / 1000 << " Kbps" << std::endl;
        std::cout << "  ���ڵ� ������: " << config.preset << std::endl;
        std::cout << "  ��/�߰� ���: " << (config.dayNightMode ? "Ȱ��ȭ" : "��Ȱ��ȭ") << std::endl;
        std::cout << "[" << config.name << "] ��ȭ ����: " << config.getRtspUrl() << std::endl;

        // �Է� ���ؽ�Ʈ �ʱ�ȭ
        AVFormatContext* inputFormatContext = nullptr;

        // RTSP ���� �ɼ� ���� - UDP ���� ��� ���
        AVDictionary* options = nullptr;
        // UDP ���� ��� ��������� ����
        av_dict_set(&options, "rtsp_transport", "udp", 0);
        av_dict_set(&options, "max_delay", "500000", 0);
        av_dict_set(&options, "stimeout", "5000000", 0);
        av_dict_set(&options, "reconnect", "1", 0);
        av_dict_set(&options, "reconnect_streamed", "1", 0);
        av_dict_set(&options, "reconnect_delay_max", "5", 0);
        // UDP ���� ũ�� ���� (��Ŷ �ս� ����)
        av_dict_set(&options, "buffer_size", "1024000", 0);  // �� 1MB ����
        av_dict_set(&options, "reorder_queue_size", "5000", 0);  // ��Ŷ ������ ť ũ��

        while (isRunning) {
            try {
                // RTSP ��Ʈ�� ���� - UDP ��� ���
                std::cout << "[" << config.name << "] UDP ���� ������� ���� �õ�..." << std::endl;
                int ret = avformat_open_input(&inputFormatContext, config.getRtspUrl().c_str(), nullptr, &options);

                if (ret < 0) {
                    char errBuf[AV_ERROR_MAX_STRING_SIZE];
                    av_strerror(ret, errBuf, AV_ERROR_MAX_STRING_SIZE);
                    throw std::runtime_error("UDP�� ��Ʈ���� �� �� �����ϴ�: " + std::string(errBuf));
                }

                std::cout << "[" << config.name << "] UDP ���� ������� ���� ����!" << std::endl;

                // ��Ʈ�� ���� ã��
                if (avformat_find_stream_info(inputFormatContext, nullptr) < 0) {
                    throw std::runtime_error("��Ʈ�� ������ ã�� �� �����ϴ�.");
                }

                // ���� ��Ʈ�� ã��
                int videoStreamIndex = -1;
                for (unsigned int i = 0; i < inputFormatContext->nb_streams; i++) {
                    if (inputFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                        videoStreamIndex = i;
                        break;
                    }
                }

                if (videoStreamIndex == -1) {
                    throw std::runtime_error("���� ��Ʈ���� ã�� �� �����ϴ�.");
                }

                // �ڵ� �Ķ����
                AVCodecParameters* codecParameters = inputFormatContext->streams[videoStreamIndex]->codecpar;

                // ���ڴ� ã��
                const AVCodec* decoder = avcodec_find_decoder(codecParameters->codec_id);
                if (!decoder) {
                    throw std::runtime_error("���ڴ��� ã�� �� �����ϴ�.");
                }

                // ���ڴ� ���ؽ�Ʈ
                AVCodecContext* decoderContext = avcodec_alloc_context3(decoder);
                if (!decoderContext) {
                    throw std::runtime_error("���ڴ� ���ؽ�Ʈ�� �Ҵ��� �� �����ϴ�.");
                }

                if (avcodec_parameters_to_context(decoderContext, codecParameters) < 0) {
                    avcodec_free_context(&decoderContext);
                    throw std::runtime_error("���ڴ� ���ؽ�Ʈ�� ������ �� �����ϴ�.");
                }

                if (avcodec_open2(decoderContext, decoder, nullptr) < 0) {
                    avcodec_free_context(&decoderContext);
                    throw std::runtime_error("���ڴ��� �� �� �����ϴ�.");
                }

                // ���� ���� ũ�� ���
                std::cout << "[" << config.name << "] ���� ���� �ػ�: "
                    << decoderContext->width << "x" << decoderContext->height << std::endl;

                // ���� �ð� ��� ���ڵ� ���� ���� (��/�߰� ���)
                int currentBitrate = config.getCurrentBitrate();
                int currentFps = config.getCurrentFps();

                std::cout << "[" << config.name << "] ���� ���� ����: "
                    << currentBitrate / 1000 << "Kbps, "
                    << currentFps << "fps" << std::endl;

                // ��� ���� �̸� ����
                std::string outputFileName = config.getOutputFileName();
                {
                    std::lock_guard<std::mutex> lock(statusMutex);
                    currentFileName = outputFileName;
                }

                // ��� ���ؽ�Ʈ �ʱ�ȭ
                AVFormatContext* outputFormatContext = nullptr;
                if (avformat_alloc_output_context2(&outputFormatContext, nullptr, "mp4", outputFileName.c_str()) < 0) {
                    avcodec_free_context(&decoderContext);
                    throw std::runtime_error("��� ���ؽ�Ʈ�� ������ �� �����ϴ�.");
                }

                // ����Ʈ���� HEVC ���ڴ� ��� (�ϵ���� ���� ���)
                const AVCodec* encoder = avcodec_find_encoder_by_name("h264_mf");
                if (!encoder) {
                    printf("��� ������ ���ڴ� ���:\n");
                    print_encoders();  // ������ ������ �Լ�

                    avcodec_free_context(&decoderContext);
                    avformat_free_context(outputFormatContext);
                    throw std::runtime_error("H.265 ���ڴ�(hevc_mf)�� ã�� �� �����ϴ�.");
                }

                // ��� ��Ʈ�� ����
                AVStream* outputStream = avformat_new_stream(outputFormatContext, nullptr);
                if (!outputStream) {
                    avcodec_free_context(&decoderContext);
                    avformat_free_context(outputFormatContext);
                    throw std::runtime_error("��� ��Ʈ���� ������ �� �����ϴ�.");
                }

                // ���ڴ� ���ؽ�Ʈ
                AVCodecContext* encoderContext = avcodec_alloc_context3(encoder);
                if (!encoderContext) {
                    avcodec_free_context(&decoderContext);
                    avformat_free_context(outputFormatContext);
                    throw std::runtime_error("���ڴ� ���ؽ�Ʈ�� �Ҵ��� �� �����ϴ�.");
                }

                // ���ڴ� ���� - h264_mf�� �°� ����
                encoderContext->height = config.height;
                encoderContext->width = config.width;
                encoderContext->sample_aspect_ratio = decoderContext->sample_aspect_ratio;
                encoderContext->pix_fmt = AV_PIX_FMT_NV12;  // Media Foundation H.264 ���ڴ��� NV12 ���� ��ȣ
                encoderContext->bit_rate = currentBitrate;
                encoderContext->time_base.num = 1;
                encoderContext->time_base.den = currentFps;
                encoderContext->framerate.num = currentFps;
                encoderContext->framerate.den = 1;
                encoderContext->gop_size = currentFps * 2;  // 2�ʸ��� Ű������
                encoderContext->max_b_frames = 0;  // B-������ ��Ȱ��ȭ (���� �ð� ����)

                if (outputFormatContext->oformat->flags & AVFMT_GLOBALHEADER) {
                    encoderContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
                }

                // Media Foundation ���ڴ� �ɼ�
                AVDictionary* encoderOptions = nullptr;

                // h264_mf ���ڴ��� �ɼ�
                av_dict_set(&encoderOptions, "rate_control", "cbr", 0);       // ���� ��Ʈ����Ʈ
                av_dict_set(&encoderOptions, "quality", "80", 0);             // ǰ�� ���� ���ڷ� ���� (�������� ���� ǰ��)
                av_dict_set(&encoderOptions, "profile", "high", 0);           // ������ ���� (baseline, main, high)
                av_dict_set(&encoderOptions, "level", "4.1", 0);              // ���� ����


                // Media Foundation Ưȭ ����
                // Ű������ ���� ���� ��� ����
                av_dict_set(&encoderOptions, "keyint", std::to_string(currentFps * 2).c_str(), 0);  // Ű������ ����
                // ���� ���� ���� �õ�
                av_dict_set(&encoderOptions, "delay", "0", 0);                // ���� �ּ�ȭ


                // ���ڴ� ���� �� �ɼ� ����
                ret = avcodec_open2(encoderContext, encoder, &encoderOptions);
                if (ret < 0) {
                    char errbuf[AV_ERROR_MAX_STRING_SIZE];
                    av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);

                    // ����� ���� ���
                    fprintf(stderr, "���ڴ� ���� ����: %s\n", errbuf);

                    // ��� �ɼ� �����ϰ� �⺻ �������� �õ�
                    av_dict_free(&encoderOptions);
                    encoderOptions = nullptr;

                    // �⺻ �������� �ٽ� �õ�
                    fprintf(stderr, "�⺻ �������� ���ڴ� ���� ��õ�...\n");
                    ret = avcodec_open2(encoderContext, encoder, NULL);
                    if (ret < 0) {
                        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
                        throw std::runtime_error(std::string("���ڴ��� �� �� �����ϴ�: ") + errbuf);
                    }
                }

                // ������� ���� �ɼ� Ȯ�� (������)
                if (av_dict_count(encoderOptions) > 0) {
                    char* unused_opts = NULL;
                    av_dict_get_string(encoderOptions, &unused_opts, '=', ',');
                    printf("�̻�� ���ڴ� �ɼ�: %s\n", unused_opts);
                    av_free(unused_opts);
                }

                av_dict_free(&encoderOptions);

                // ��Ʈ�� �Ķ���� ����
                if (avcodec_parameters_from_context(outputStream->codecpar, encoderContext) < 0) {
                    avcodec_free_context(&encoderContext);
                    avcodec_free_context(&decoderContext);
                    avformat_free_context(outputFormatContext);
                    throw std::runtime_error("�ڵ� �Ķ���͸� ������ �� �����ϴ�.");
                }

                outputStream->time_base = encoderContext->time_base;

                // ��� ���� ����
                if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
                    if (avio_open(&outputFormatContext->pb, outputFileName.c_str(), AVIO_FLAG_WRITE) < 0) {
                        avcodec_free_context(&encoderContext);
                        avcodec_free_context(&decoderContext);
                        avformat_free_context(outputFormatContext);
                        throw std::runtime_error("��� ������ �� �� �����ϴ�: " + outputFileName);
                    }
                }

                // ���� ��� ����
                if (avformat_write_header(outputFormatContext, nullptr) < 0) {
                    avcodec_free_context(&encoderContext);
                    avcodec_free_context(&decoderContext);
                    if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
                        avio_closep(&outputFormatContext->pb);
                    }
                    avformat_free_context(outputFormatContext);
                    throw std::runtime_error("���� ����� �� �� �����ϴ�.");
                }

                // �����Ϸ� �ʱ�ȭ
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
                    throw std::runtime_error("�����Ϸ� ���ؽ�Ʈ�� �ʱ�ȭ�� �� �����ϴ�.");
                }

                // ��Ŷ �� ������ �ʱ�ȭ
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
                    throw std::runtime_error("��Ŷ�̳� �������� �Ҵ��� �� �����ϴ�.");
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
                    throw std::runtime_error("��� ������ ���۸� �Ҵ��� �� �����ϴ�.");
                }

                // ��ȭ ���� �ð� �� ���׸�Ʈ �ð� �ʱ�ȭ
                int64_t startTime = av_gettime();
                int64_t segmentStartTime = startTime;
                int64_t pts = 0;
                int localFrameCount = 0;
                int frameSkipCounter = 0; // ������ ��ŵ ī���� (FPS ������)

                // �ӽ� ���� ������
                bool createNewSegment = false;
                bool reconnectRequired = false;

                // ���� ���ڵ� ����
                while (isRunning) {
                    // �Ͻ� ���� ���� Ȯ��
                    if (isPaused) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        continue;
                    }

                    // ���׸�Ʈ �ð� üũ (���� ����)
                    int64_t currentTime = av_gettime();
                    if (config.segmentDuration > 0 &&
                        (currentTime - segmentStartTime) / 1000000 > config.segmentDuration) {
                        createNewSegment = true;
                        break;
                    }

                    // ��Ŷ �б� (UDP�� ����)
                    int ret = av_read_frame(inputFormatContext, packet);
                    if (ret < 0) {
                        // ��Ʈ�� ���̰ų� ���� �߻�
                        if (ret == AVERROR_EOF || avio_feof(inputFormatContext->pb)) {
                            std::cout << "[" << config.name << "] ��Ʈ�� ���� ����" << std::endl;
                            break;
                        }

                        if (ret == AVERROR(EAGAIN)) {
                            // �����Ͱ� ���� �����Ƿ� ���
                            std::this_thread::sleep_for(std::chrono::milliseconds(10));
                            continue;
                        }

                        // ��Ʈ��ũ ���� ���� ��� �翬�� �ʿ�
                        errorCount++;
                        std::cerr << "[" << config.name << "] ��Ŷ �б� ����: " << ret
                            << " (���� " << errorCount << "ȸ)" << std::endl;

                        if (errorCount > 5) {
                            std::cerr << "[" << config.name << "] �ʹ� ���� ���� �߻�, �翬�� �õ�" << std::endl;
                            reconnectRequired = true;
                            break;
                        }

                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        continue;
                    }

                    // ���� ��Ŷ�� ó��
                    if (packet->stream_index == videoStreamIndex) {
                        // ������ ��ŵ ���� - ���� FPS ����
                        frameSkipCounter++;
                        if (frameSkipCounter % (30 / config.getCurrentFps()) != 0) {
                            // �Ϻ� ������ �ǳʶٱ� (���� 30fps ����)
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

                            // ������ ��ȯ
                            sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height,
                                outFrame->data, outFrame->linesize);

                            outFrame->pts = pts++;

                            // ���ڵ�
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

                                // ���� ��Ȳ ǥ�� (500�����Ӹ���)
                                if (localFrameCount % 500 == 0) {
                                    int64_t elapsedMs = (av_gettime() - startTime) / 1000;
                                    float fps = (elapsedMs > 0) ? (localFrameCount * 1000.0f / elapsedMs) : 0;
                                    std::cout << "[" << config.name << "] ������: " << localFrameCount
                                        << ", FPS: " << fps << std::endl;
                                }
                            }
                        }
                    }

                    av_packet_unref(packet);
                }

                // ���� ������ �÷���
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

                // ���� Ʈ���Ϸ� ����
                av_write_trailer(outputFormatContext);

                // ���ҽ� ����
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

                // ���� ��Ȳ �� ���� ������Ʈ
                std::cout << "[" << config.name << "] "
                    << (createNewSegment ? "���׸�Ʈ �Ϸ�" : "��ȭ �Ϸ�")
                    << ": " << outputFileName << " (" << localFrameCount << " ������)" << std::endl;

                // ���׸�Ʈ ��忡�� �� ���׸�Ʈ ����
                if (createNewSegment && isRunning) {
                    segmentStartTime = av_gettime();
                    continue; // �� ���׸�Ʈ�� ���� ���� ���
                }

                // �翬�� �ʿ� �� ó��
                if (reconnectRequired) {
                    std::cout << "[" << config.name << "] �翬�� ��..." << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(2)); // �ణ�� ���� �� ��õ�
                    continue;
                }

                // �ٸ� ��Ȳ������ ���� ����
                break;
            }
            catch (const std::exception& e) {
                errorCount++;
                std::cerr << "[" << config.name << "] ���� �߻�: " << e.what() << std::endl;

                // �Է� ���ؽ�Ʈ�� ���� ������ �ݱ�
                if (inputFormatContext) {
                    avformat_close_input(&inputFormatContext);
                }

                // ��� ��� �� ��õ�
                std::this_thread::sleep_for(std::chrono::seconds(5));

                if (errorCount > 20) {
                    std::cerr << "[" << config.name << "] ������ �ʹ� ���� �߻��Ͽ� ��ȭ�� �ߴ��մϴ�." << std::endl;
                    break;
                }
            }
        }

        std::cout << "[" << config.name << "] ��ȭ ������ ����" << std::endl;
    }
};

// ���� ī�޶� ���� Ŭ����
class MultiCameraRecorder {
private:
    std::vector<ONVIFCameraConfig> cameraConfigs;
    std::vector<std::unique_ptr<ONVIFCameraRecorder>> recorders;

    // ������ ��ȭ ���� ���� �Լ�
    void cleanupOldRecordings(const std::string& directory, int daysToKeep) {
        std::cout << "������ ��ȭ ���� ���� ��... (" << daysToKeep << "�� �̻� ����)" << std::endl;

        // ���� �ð� ���
        auto now = std::chrono::system_clock::now();
        auto cutoffTime = now - std::chrono::hours(24 * daysToKeep);
        auto cutoffTimeT = std::chrono::system_clock::to_time_t(cutoffTime);

#ifdef _WIN32
        // Windows���� ���丮 Ž��
        WIN32_FIND_DATAA findData;
        std::string searchPath = directory + "\\*.mp4";
        HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                std::string filename = findData.cFileName;
                std::string fullPath = directory + "\\" + filename;

                // ���� �ð� Ȯ��
                FILETIME ftCreate, ftAccess, ftWrite;
                HANDLE hFile = CreateFileA(fullPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, 0, NULL);
                if (hFile != INVALID_HANDLE_VALUE) {
                    if (GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite)) {
                        ULARGE_INTEGER uli;
                        uli.LowPart = ftWrite.dwLowDateTime;
                        uli.HighPart = ftWrite.dwHighDateTime;

                        // ���� �ð��� time_t�� ��ȯ (Windows FILETIME�� 1601����� ����, time_t�� 1970����� ����)
                        const int64_t WINDOWS_TICK = 10000000;
                        const int64_t SEC_TO_UNIX_EPOCH = 11644473600LL;
                        time_t fileTime = (uli.QuadPart / WINDOWS_TICK - SEC_TO_UNIX_EPOCH);

                        // ������ ���� ����
                        if (fileTime < cutoffTimeT) {
                            CloseHandle(hFile);
                            if (DeleteFileA(fullPath.c_str())) {
                                std::cout << "������: " << filename << std::endl;
                            }
                            else {
                                std::cerr << "���� ����: " << filename << std::endl;
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
        // Linux/Unix �ý��ۿ��� ���丮 Ž�� (dirent.h ���)
        DIR* dir = opendir(directory.c_str());
        if (dir != NULL) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != NULL) {
                std::string filename = entry->d_name;

                // mp4 ���ϸ� ó��
                if (filename.length() < 4 || filename.substr(filename.length() - 4) != ".mp4") {
                    continue;
                }

                std::string fullPath = directory + "/" + filename;

                // ���� ���� Ȯ��
                struct stat fileStat;
                if (stat(fullPath.c_str(), &fileStat) == 0) {
                    // ���� �ð� Ȯ��
                    if (fileStat.st_mtime < cutoffTimeT) {
                        if (remove(fullPath.c_str()) == 0) {
                            std::cout << "������: " << filename << std::endl;
                        }
                        else {
                            std::cerr << "���� ����: " << filename << std::endl;
                        }
                    }
                }
            }
            closedir(dir);
        }
#endif
    }

public:
    // �������� ���� ���� �۾� ���� (��ũ ���� ����)
    void performMaintenance() {
        std::cout << "��ũ ���� ���� �۾� ���� ��..." << std::endl;

        // �� ī�޶� ���丮 ����
        for (const auto& config : cameraConfigs) {
            cleanupOldRecordings(config.outputDir, 7);
        }
    }

    // ī�޶� ���� �߰�
    void addCamera(const ONVIFCameraConfig& config) {
        cameraConfigs.push_back(config);
    }

    // ��� ī�޶� ��ȭ ����
    void startAll() {
        for (const auto& config : cameraConfigs) {
            auto recorder = std::make_unique<ONVIFCameraRecorder>(config);
            if (recorder->start()) {
                recorders.push_back(std::move(recorder));
            }
        }

        // ������ ���� ���� (7�� �̻� �� ����)
        for (const auto& config : cameraConfigs) {
            cleanupOldRecordings(config.outputDir, 7);
        }
    }

    // ��� ī�޶� ��ȭ ����
    void stopAll() {
        for (auto& recorder : recorders) {
            recorder->stop();
        }
        recorders.clear();
    }

    // ��� ī�޶� ���� ���
    void printAllStatus() {
        std::cout << "===== ī�޶� ���� =====" << std::endl;
        for (auto& recorder : recorders) {
            auto status = recorder->getStatus();
            std::cout << "ī�޶�: " << status["name"] << std::endl;
            std::cout << "  ����: " << status["state"] << std::endl;
            std::cout << "  ���� ����: " << status["current_file"] << std::endl;
            std::cout << "  ������ ��: " << status["frames_recorded"] << std::endl;
            std::cout << "  ���� �ð�: " << status["running_time"] << std::endl;
            std::cout << "  ���� ��Ʈ����Ʈ: " << status["current_bitrate"] << std::endl;
            std::cout << "  ���� FPS: " << status["current_fps"] << std::endl;
            std::cout << "  ���� ��: " << status["error_count"] << std::endl;
            std::cout << std::endl;
        }
    }

    // ��ũ ��뷮 ���
    void printDiskUsage() {
        std::cout << "===== ��ũ ��뷮 =====" << std::endl;

        for (const auto& config : cameraConfigs) {
            std::string directory = config.outputDir;
            uint64_t totalSize = 0;
            int fileCount = 0;

#ifdef _WIN32
            // Windows���� ���丮 �� ���� ũ�� ���
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
            // Linux/Unix �ý��ۿ��� ���丮 �� ���� ũ�� ���
            DIR* dir = opendir(directory.c_str());
            if (dir != NULL) {
                struct dirent* entry;
                while ((entry = readdir(dir)) != NULL) {
                    std::string filename = entry->d_name;

                    // mp4 ���ϸ� ó��
                    if (filename.length() < 4 || filename.substr(filename.length() - 4) != ".mp4") {
                        continue;
                    }

                    std::string fullPath = directory + "/" + filename;

                    // ���� ũ�� Ȯ��
                    struct stat fileStat;
                    if (stat(fullPath.c_str(), &fileStat) == 0) {
                        totalSize += fileStat.st_size;
                        fileCount++;
                    }
                }
                closedir(dir);
            }
#endif

            // MB �� GB ������ ��ȯ�Ͽ� ���
            double sizeInMB = totalSize / (1024.0 * 1024.0);
            double sizeInGB = sizeInMB / 1024.0;

            std::cout << "ī�޶� " << config.name << " (" << directory << "):" << std::endl;
            std::cout << "  ���� ��: " << fileCount << std::endl;
            std::cout << "  �� �뷮: " << sizeInMB << " MB (" << sizeInGB << " GB)" << std::endl;
            std::cout << std::endl;
        }
    }
};

// ���� �Լ�
int main() {
    // ���� ī�޶� ���ڴ� ����
    MultiCameraRecorder multiRecorder;

    // �׽�Ʈ ī�޶� ���� �߰� - H.265 IP ī�޶�� ����
    multiRecorder.addCamera(ONVIFCameraConfig(
        "IPCamera",                           // ī�޶� �̸�
        "192.168.219.181",                    // IP �ּ�
        554,                                  // RTSP ǥ�� ��Ʈ
        "admin",                              // ����� �̸� (ī�޶� ������ �°� ����)
        "Windo4101!",                           // ��й�ȣ (ī�޶� ������ �°� ����)
        "rtsp://admin:Windo4101!@192.168.219.181:554/stream1",  // RTSP URL (ī�޶��� ���� ��Ʈ�� ��η� ����)
        "C:/Download/savedfiles"              // ���� ���
    ));

    // ��� ī�޶� ��ȭ ����
    std::cout << "��ȭ�� �����մϴ�..." << std::endl;
    multiRecorder.startAll();

    // ���� ����
    bool running = true;
    while (running) {
        std::cout << "\n��ɾ�: status(����), disk(��ũ ��뷮), clean(���� ����), stop(����), quit(����)" << std::endl;
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
            std::cout << "��ȭ�� �����մϴ�..." << std::endl;
            multiRecorder.stopAll();
            running = false;
        }
        else if (command == "quit") {
            std::cout << "���α׷��� �����մϴ�..." << std::endl;
            multiRecorder.stopAll();
            running = false;
        }
        else {
            std::cout << "�� �� ���� ��ɾ��Դϴ�. �ٽ� �õ��ϼ���." << std::endl;
        }
    }

    return 0;
}