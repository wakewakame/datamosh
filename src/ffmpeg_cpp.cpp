#include "ffmpeg_cpp.h"

namespace ff {
	void avformat_close_input_cpp::operator()(AVFormatContext *ptr) const {
		// メモ
		// avformat_close_input()は内部で
		// avformat_free_context()を呼び出している
		avformat_close_input(&ptr);
	}

	AVFormatContextCppInput avformat_open_input_cpp(
		const std::string& filename,
		ff_const59 AVInputFormat *fmt,
		AVDictionary **options,
		int* ret
	) {
		// メモ
		// avformat_open_input()が失敗するとctxは必ずNULLとなるため
		// 呼び出しが失敗したときにctxを開放する必要はない
		AVFormatContext *ctx = nullptr;
		int ret_ = avformat_open_input(&ctx, filename.c_str(), fmt, options);
		if (ret) { *ret = ret_; }
		return AVFormatContextCppInput(ctx);
	}

	void avformat_free_context_cpp::operator()(AVFormatContext *ptr) const {
		avformat_free_context(ptr);
	}

	AVFormatContextCppOutput avformat_alloc_output_context2_cpp(
		ff_const59 AVOutputFormat *oformat,
		const char *format_name,
		const char *filename,
		int* ret
	) {
		AVFormatContext *ctx = nullptr;
		int ret_ = avformat_alloc_output_context2(&ctx, oformat, format_name, filename);
		if (ret) { *ret = ret_; }
		return AVFormatContextCppOutput(ctx);
	}

	void avcodec_free_context_cpp::operator()(AVCodecContext *ptr) const {
		avcodec_free_context(&ptr);
	}

	AVCodecContextCpp avcodec_alloc_context3_cpp(const AVCodec* av) {
		AVCodecContext *ctx = avcodec_alloc_context3(av);
		return AVCodecContextCpp(ctx);
	}

	void avio_closep_cpp::operator()(AVIOContext *ptr) const {
		avio_closep(&ptr);
	}

	AVIOContextCpp avio_open_cpp(const char *url, int flags, int* ret) {
		AVIOContext *ctx = nullptr;
		int ret_ = avio_open(&ctx, url, flags);
		if (ret) { *ret = ret_; }
		return AVIOContextCpp(ctx);
	}

	void av_frame_free_cpp::operator()(AVFrame *ptr) const {
		// メモ
		// AVFrameは参照カウンタのような仕組みだと思っていたが、
		// AVFrame自体は参照カウントを持っていない。
		// AVFrameの内部にあるAVBufferRefが参照カウンタになっている。
		av_frame_free(&ptr);
	}

	AVFrameCpp av_frame_alloc_cpp() {
		return AVFrameCpp(av_frame_alloc());
	}

	void av_packet_free_cpp::operator()(AVPacket *ptr) const {
		av_packet_free(&ptr);
	}

	AVPacketCpp av_packet_alloc_cpp() {
		return AVPacketCpp(av_packet_alloc());
	}

	void sws_freeContext_cpp::operator()(SwsContext *ptr) const {
		sws_freeContext(ptr);
	}

	SwsContextCpp sws_getContext_cpp(
		int srcW, int srcH, enum AVPixelFormat srcFormat,
		int dstW, int dstH, enum AVPixelFormat dstFormat,
		int flags, SwsFilter *srcFilter,
		SwsFilter *dstFilter, const double *param
	) {
		return SwsContextCpp(sws_getContext(
			srcW, srcH, srcFormat, dstW, dstH, dstFormat,
			flags, srcFilter, dstFilter, param
		));
	}

	void VideoReader::reset() {
		decoded_frame_queue.clear();
		packet.reset();
		codec_context.reset();
		format_context.reset();
		stream_index = AVMEDIA_TYPE_UNKNOWN;
		pts_indices.clear();
		next_frame_number = 0;
	}

	VideoReader::VideoReader(const std::string& path) {
		reset();

		format_context = avformat_open_input_cpp(path);
		if (!format_context) {
			reset();
			return;
		}

		if (avformat_find_stream_info(format_context.get(), NULL) < 0) {
			reset();
			return;
		}

		AVCodec *p_codec = nullptr;  // このポインタの解放はffmpeg側が責任を持つ
		AVCodecParameters *p_codec_parameters = nullptr;  // このポインタの解放はffmpeg側が責任を持つ
		stream_index = AVMEDIA_TYPE_UNKNOWN;
		for (unsigned int i = 0; i < format_context->nb_streams; i++) {
			AVCodecParameters *p_codec_parameters_tmp = nullptr;
			p_codec_parameters_tmp = format_context->streams[i]->codecpar;
			AVCodec *p_codec_tmp = nullptr;
			p_codec_tmp = avcodec_find_decoder(p_codec_parameters_tmp->codec_id);
			if (!p_codec_tmp) {
				reset();
				return;
			}
			if (p_codec_parameters_tmp->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO) {
				stream_index = i;
				p_codec = p_codec_tmp;
				p_codec_parameters = p_codec_parameters_tmp;
			}
		}
		if (stream_index == AVMEDIA_TYPE_UNKNOWN) {
			reset();
			return;
		}

		codec_context = avcodec_alloc_context3_cpp(p_codec);
		if (!codec_context) {
			reset();
			return;
		}
		if (avcodec_parameters_to_context(codec_context.get(), p_codec_parameters) < 0) {
			reset();
			return;
		}
		if (avcodec_open2(codec_context.get(), p_codec, NULL) < 0) {
			reset();
			return;
		}
		packet = av_packet_alloc_cpp();
		if (!packet) {
			reset();
			return;
		}

		// すべてのタイムスタンプを記録する
		while(av_read_frame(format_context.get(), packet.get()) >= 0) {
			if (packet->stream_index == stream_index) {
				pts_indices.push_back(PTSIndex{
					packet->pts,
					static_cast<bool>(packet->flags & AV_PKT_FLAG_KEY)
				});
			}
			av_packet_unref(packet.get());
		}
		if (!pts_indices.size()) return;
		std::sort(
			pts_indices.begin(), pts_indices.end(),
			[](const PTSIndex& a, const PTSIndex& b) { return a.pts < b.pts; }
		);
		if (av_seek_frame(format_context.get(), stream_index, pts_indices.front().pts, AVSEEK_FLAG_BACKWARD) < 0) return;
	}

	VideoReader::~VideoReader() {
		reset();
	}

	AVFrameCpp VideoReader::nextFrame() {
		if (!isOpened()) return nullptr;

		// すでにフレームキューにフレームがある場合はそれを優先して返す
		if (!decoded_frame_queue.empty()) {
			AVFrameCpp frame = std::move(decoded_frame_queue.front());
			decoded_frame_queue.pop_front();
			next_frame_number += 1;
			return frame;
		}

		// フレームがキューに追加されるまでパケットを読み続ける
		while(decoded_frame_queue.empty()) {
			// メモ
			// av_read_frame()の中でpacketが上書きされるとき
			// av_read_frame()は上書き前の参照カウンタを減らさないので
			// その前にav_packet_unref()で参照カウントを減らす必要がある
			if (av_read_frame(format_context.get(), packet.get()) < 0) {
				return nullptr;
			}

			if (packet->stream_index != stream_index) {
				av_packet_unref(packet.get());
				continue;
			}

			int ret = avcodec_send_packet(codec_context.get(), packet.get());
			if (ret < 0) { return nullptr; }

			while (ret >= 0) {
				AVFrameCpp frame = av_frame_alloc_cpp();
				// メモ
				// avcodec_receive_frame()の中でframeが上書きされるとき
				// avcodec_receive_frame()は上書き前の参照カウンタを減らすので
				// その前にav_frame_unref()で参照カウントを減らす必要はない
				ret = avcodec_receive_frame(codec_context.get(), frame.get());
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					break;
				}
				else if (ret < 0) {
					reset();
					return nullptr;
				}

				decoded_frame_queue.push_back(std::move(frame));
			}

			av_packet_unref(packet.get());
		}

		if (!decoded_frame_queue.empty()) {
			AVFrameCpp frame = std::move(decoded_frame_queue.front());
			decoded_frame_queue.pop_front();
			next_frame_number += 1;
			return frame;
		}
		else {
			return nullptr;
		}
	}

	int VideoReader::seek(int64_t frame_number, SeekMode mode) {
		if (!isOpened()) return -1;
		if (!pts_indices.size()) return -1;
		if (frame_number < 0 || pts_indices.size() <= frame_number) return -1;

		for(int i = frame_number; 0 <= i; i--) {
			PTSIndex pts_index = pts_indices[i];
			if (pts_index.key_frame) {
				if (av_seek_frame(format_context.get(), stream_index, pts_index.pts, AVSEEK_FLAG_BACKWARD) < 0) return -1;
				avcodec_flush_buffers(codec_context.get());
				decoded_frame_queue.clear();
				next_frame_number = i;
				if (SeekMode::Exact == mode) {
					for(int64_t j = i; j < frame_number; j++) {
						nextFrame();
					}
				}
				return 0;
			}
		}

		return -1;
	}

	bool VideoReader::isOpened() const {
		return static_cast<bool>(format_context);
	}

	int64_t VideoReader::getNextFrameNumber() const {
		return next_frame_number;
	}

	AVRational VideoReader::getTimeBase() const {
		return format_context->streams[stream_index]->time_base;
	}

	void VideoWriter::reset() {
		p_codec = nullptr;
		p_stream = nullptr;
		packet.reset();
		codec_context.reset();
		format_context.reset();
		io_context.reset();
		datamosh_pts.clear();
	}

	VideoWriter::VideoWriter(const std::string& path, const AVRational& time_base) {
		reset();

		io_context = ff::avio_open_cpp(path.c_str(), AVIO_FLAG_WRITE);
		if (!io_context) {
			reset();
			return;
		}

		format_context = ff::avformat_alloc_output_context2_cpp(nullptr, "mp4", nullptr);
		if (!format_context) {
			reset();
			return;
		}

		format_context->pb = io_context.get();
		p_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!p_codec) {
			reset();
			return;
		}

		codec_context = ff::avcodec_alloc_context3_cpp(p_codec);
		if (!codec_context) {
			reset();
			return;
		}
		codec_context->time_base = time_base;
	}

	VideoWriter::~VideoWriter() {
		reset();
	}

	int VideoWriter::nextFrame(AVFrameCpp&& frame, bool datamosh) {
		if (!isOpened()) return -1;

		if (frame) {
			if (!avcodec_is_open(codec_context.get())) {
				codec_context->pix_fmt = static_cast<AVPixelFormat>(frame->format);
				codec_context->width = frame->width;
				codec_context->height = frame->height;
				codec_context->field_order = AV_FIELD_PROGRESSIVE;
				codec_context->color_range = frame->color_range;
				codec_context->color_primaries = frame->color_primaries;
				codec_context->color_trc = frame->color_trc;
				codec_context->colorspace = frame->colorspace;
				codec_context->chroma_sample_location = frame->chroma_location;
				codec_context->sample_aspect_ratio = frame->sample_aspect_ratio;
				if (format_context->oformat->flags & AVFMT_GLOBALHEADER) {
					codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
				}

				AVDictionary* codec_options = nullptr;
				av_dict_set(&codec_options, "preset", "medium", 0);
				av_dict_set(&codec_options, "crf", "22", 0);
				av_dict_set(&codec_options, "profile", "high", 0);
				av_dict_set(&codec_options, "level", "4.0", 0);
				if (avcodec_open2(codec_context.get(), codec_context->codec, &codec_options) != 0) {
					reset();
					return -1;
				}

				p_stream = avformat_new_stream(format_context.get(), p_codec);
				if (!p_stream) {
					reset();
					return -1;
				}

				p_stream->sample_aspect_ratio = codec_context->sample_aspect_ratio;
				p_stream->time_base = codec_context->time_base;

				if (avcodec_parameters_from_context(p_stream->codecpar, codec_context.get()) < 0) {
					reset();
					return -1;
				}

				if (avformat_write_header(format_context.get(), nullptr) < 0) {
					reset();
					return -1;
				}
			}
			if (datamosh) { datamosh_pts.push_back(frame->pts); }
		}

		if (avcodec_send_frame(codec_context.get(), frame.get()) != 0) {
			reset();
			return -1;
		}

		AVPacketCpp packet = av_packet_alloc_cpp();
		while (avcodec_receive_packet(codec_context.get(), packet.get()) == 0) {
			bool ignore = std::find(datamosh_pts.begin(), datamosh_pts.end(), packet->pts) != datamosh_pts.end();
			if (ignore) {
				datamosh_pts.remove(packet->pts);
				continue;
			}
			packet->stream_index = 0;
			av_packet_rescale_ts(packet.get(), codec_context->time_base, p_stream->time_base);
			if (av_interleaved_write_frame(format_context.get(), packet.get()) != 0) {
				reset();
				return -1;
			}
		}

		return 0;
	}

	int VideoWriter::flush() {
		if (nextFrame(nullptr)) {
			reset();
			return -1;
		}

		if (av_write_trailer(format_context.get()) != 0) {
			reset();
			return -1;
		}

		return 0;
	}

	bool VideoWriter::isOpened() const {
		return static_cast<bool>(codec_context);
	}
}

