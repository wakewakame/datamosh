#include <iostream>
#include <cstring>
#include "ffmpeg_cpp.h"

ff::AVFrameCpp copy_frame_data(const ff::AVFrameCpp& frame, uint64_t pts, bool key_frame) {
	ff::AVFrameCpp converted_frame = ff::VideoReader::convert(frame, AVPixelFormat::AV_PIX_FMT_YUV420P);
	if (!converted_frame) { return nullptr; }

	ff::AVFrameCpp new_frame = ff::av_frame_alloc_cpp();
	if (!new_frame) { return nullptr; }

	new_frame->format = converted_frame->format;
	new_frame->width  = converted_frame->width;
	new_frame->height = converted_frame->height;
	new_frame->pts = pts;
	new_frame->key_frame = key_frame;
	new_frame->pict_type = key_frame ? AV_PICTURE_TYPE_I : AV_PICTURE_TYPE_NONE;
	if (av_frame_get_buffer(new_frame.get(), 0) < 0) { return nullptr; }

	if (new_frame->linesize[0] != converted_frame->linesize[0]) { return nullptr; }
	if (new_frame->linesize[1] != converted_frame->linesize[1]) { return nullptr; }
	if (new_frame->linesize[2] != converted_frame->linesize[2]) { return nullptr; }
	std::memcpy(new_frame->data[0], converted_frame->data[0], new_frame->linesize[0] * new_frame->height);
	std::memcpy(new_frame->data[1], converted_frame->data[1], new_frame->linesize[1] * new_frame->height / 2);
	std::memcpy(new_frame->data[2], converted_frame->data[2], new_frame->linesize[2] * new_frame->height / 2);

	return new_frame;
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		std::cout << "Usage: datamosh <video1 path> <video2 path> ..." << std::endl;
		return 0;
	}

	ff::VideoWriter writer("output.mp4", AVRational{ 1, 60 });
	uint64_t global_frame_number = 0;
	for (uint64_t i = 1; i < argc; i++) {
		ff::VideoReader reader(argv[i]);
		if (!reader.isOpened()) {
			std::cout << "Error: failed to open " << argv[i] << std::endl;
			return 1;
		}

		for(uint64_t local_frame_number = 0;; local_frame_number++, global_frame_number++) {
			ff::AVFrameCpp frame = reader.nextFrame();
			if (!frame) { break; }

			if (global_frame_number == 0) {
				writer.nextFrame(copy_frame_data(frame, global_frame_number, true), false);
			}
			else {
				if (local_frame_number == 0) {
					writer.nextFrame(copy_frame_data(frame, global_frame_number, true), true);
					global_frame_number++;
				}
				writer.nextFrame(copy_frame_data(frame, global_frame_number, false), false);
			}

			std::cout << "frame: " << global_frame_number << std::endl;
		}
	}
	writer.flush();

	std::cout << "completed" << std::endl;

	return 0;
}

