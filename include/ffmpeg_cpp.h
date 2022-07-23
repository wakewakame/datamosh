#ifndef FFMPEG_CPP
#define FFMPEG_CPP

#include <functional>
#include <memory>
#include <list>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

namespace ff {
	/* FFmpegの構造体をunique_ptrでラップ */
	// AVFrameContext
	struct avformat_close_input_cpp {
		void operator()(AVFormatContext *ptr) const;
	};
	using AVFormatContextCppInput = std::unique_ptr<AVFormatContext, avformat_close_input_cpp>;
	AVFormatContextCppInput avformat_open_input_cpp(
		const std::string& filename,
		ff_const59 AVInputFormat *fmt = nullptr,
		AVDictionary **options = nullptr,
		int* ret = nullptr
	);

	// AVFormatContext
	struct avformat_free_context_cpp {
		void operator()(AVFormatContext *ptr) const;
	};
	using AVFormatContextCppOutput = std::unique_ptr<AVFormatContext, avformat_free_context_cpp>;
	AVFormatContextCppOutput avformat_alloc_output_context2_cpp(
		ff_const59 AVOutputFormat *oformat,
		const char *format_name,
		const char *filename,
		int* ret = nullptr
	);

	// AVCodecContext
	struct avcodec_free_context_cpp {
		void operator()(AVCodecContext *ptr) const;
	};
	using AVCodecContextCpp = std::unique_ptr<AVCodecContext, avcodec_free_context_cpp>;
	AVCodecContextCpp avcodec_alloc_context3_cpp(const AVCodec* av);

	// AVIOContext
	struct avio_closep_cpp {
		void operator()(AVIOContext *ptr) const;
	};
	using AVIOContextCpp = std::unique_ptr<AVIOContext, avio_closep_cpp>;
	AVIOContextCpp avio_open_cpp(const char *url, int flags, int* ret = nullptr);

	// AVFrame
	struct av_frame_free_cpp {
		void operator()(AVFrame *ptr) const;
	};
	using AVFrameCpp = std::unique_ptr<AVFrame, av_frame_free_cpp>;
	AVFrameCpp av_frame_alloc_cpp();

	// AVPacket
	struct av_packet_free_cpp {
		void operator()(AVPacket *ptr) const;
	};
	using AVPacketCpp = std::unique_ptr<AVPacket, av_packet_free_cpp>;
	AVPacketCpp av_packet_alloc_cpp();

	// SwsContext
	struct sws_freeContext_cpp {
		void operator()(SwsContext *ptr) const;
	};
	using SwsContextCpp = std::unique_ptr<SwsContext, sws_freeContext_cpp>;
	SwsContextCpp sws_getContext_cpp(
		int srcW, int srcH, enum AVPixelFormat srcFormat,
		int dstW, int dstH, enum AVPixelFormat dstFormat,
		int flags, SwsFilter *srcFilter,
		SwsFilter *dstFilter, const double *param
	);

	/* 動画のデコーダ */
	class VideoReader {
	private:
		// ファイルをAVPacket単位で読み込んでくれたり、コーデックの種類を判別してくれたりするやつ
		AVFormatContextCppInput format_context;

		// AVPacketをAVFrameに変換してくれるやつ
		AVCodecContextCpp codec_context;

		// デコードされる前の1フレーム分のデータ
		AVPacketCpp packet;

		// ファイルには動画や音声などAVStreamがまとまって入っているので
		// その中でデコードする対象のAVStreamの番号
		int stream_index;

		// デコードされたフレームが取り出されるまで一時的に保管しておくためのキュー
		std::list<AVFrameCpp> decoded_frame_queue;

		struct PTSIndex {
			int64_t pts;
			bool key_frame;
		};

		// すべてのタイムスタンプ
		std::vector<PTSIndex> pts_indices;

		// 次にデコードされるフレーム番号
		int64_t next_frame_number;
	
		void reset();

	public:
		VideoReader(const std::string& path);

		virtual ~VideoReader();

		/**
		 * @fn AVFrameCpp nextFrame
		 * @brief 次のフレームを取得する
		 * @return 求めたフレーム
		 */
		AVFrameCpp nextFrame();

		enum class SeekMode { BeforeKeyframe, Exact };

		/**
		 * @fn int64_t keyframeSeek
		 * @brief キーフレーム単位でのシークを行う
		 * @param size_t frame_number[in] シーク先のフレーム番号
		 * @param SeekMode mode[in] シークのモード
		 * @param size_t after_frame_number[out] シーク後のフレーム番号
		 * @return 0: 成功, -1: 失敗
		 */
		int seek(int64_t frame_number, SeekMode mode = SeekMode::BeforeKeyframe);

		bool isOpened() const;

		int64_t getNextFrameNumber() const;

		AVRational getTimeBase() const;

		static AVFrameCpp convert(
			const AVFrameCpp& src,
			AVPixelFormat format = AVPixelFormat::AV_PIX_FMT_RGB24
		) {
			if (!src) return nullptr;

			AVFrameCpp dst = av_frame_alloc_cpp();
			av_frame_copy_props(dst.get(), src.get());
			dst->width  = src->width;
			dst->height = src->height;
			dst->format = format;
			if (av_frame_get_buffer(dst.get(), 0) != 0) {
				return nullptr;
			}

			SwsContextCpp sws_context = sws_getContext_cpp(
				src->width, src->height, (AVPixelFormat)(src->format),
				dst->width, dst->height, (AVPixelFormat)(dst->format),
				0, 0, 0, 0
			);
			if (!sws_context) return nullptr;
			int ret = sws_scale(
				sws_context.get(),
				src->data, src->linesize, 0, src->height,
				dst->data, dst->linesize
			);
			if (ret < 0) return nullptr;

			return dst;
		}
	};

	/* 動画のエンコーダ */
	class VideoWriter {
	private:
		AVIOContextCpp io_context;
		AVFormatContextCppOutput format_context;

		// AVFrameをAVPacketに変換してくれるやつ
		AVCodecContextCpp codec_context;

		// エンコードされる前の1フレーム分のデータ
		AVPacketCpp packet;

		AVCodec *p_codec = nullptr;  // このポインタの解放はffmpeg側が責任を持つ
		AVStream *p_stream = nullptr;  // このポインタはformat_contextのデストラクタで解放される

		// 書き込みを欠落させるフレームのPTS
		std::list<int64_t> datamosh_pts;

		void reset();

	public:
		VideoWriter(const std::string& path, const AVRational& time_base);

		virtual ~VideoWriter();

		int nextFrame(AVFrameCpp&& frame, bool datamosh=false);

		int flush();

		bool isOpened() const;
	};
}

#endif  // FFMPEG_CPP

