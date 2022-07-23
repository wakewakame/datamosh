#include "ffmpeg_cpp.h"

int main(int argc, char **argv)
{
	ff::VideoWriter writer("output.mp4", AVRational{ 1, 30 });

	int ret = 0;
	for (int i = 0; i < 25; i++) {
		ff::AVFrameCpp frame = ff::av_frame_alloc_cpp();
		if (!frame) { return -1; }
		frame->format = AV_PIX_FMT_YUV420P;
		frame->width  = 352;
		frame->height = 288;
		frame->pts = i;

		ret = av_frame_get_buffer(frame.get(), 0);
		if (ret < 0) { return -1; }

		/* Y */
		for (int y = 0; y < frame->height; y++) {
			for (int x = 0; x < frame->width; x++) {
				frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
			}
		}

		/* Cb and Cr */
		for (int y = 0; y < frame->height/2; y++) {
			for (int x = 0; x < frame->width/2; x++) {
				frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
				frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
			}
		}

		if (writer.nextFrame(move(frame)) < 0) { return -1; }
	}

	if (writer.flush() < 0) { return -1; };

	return 0;
}

