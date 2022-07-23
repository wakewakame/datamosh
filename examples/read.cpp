#include <iostream>
#include "ffmpeg_cpp.h"
#include <opencv2/opencv.hpp>

int main(int argc, char **argv) {
	if (2 != argc) {
		std::cout << "Usage: read <video path>" << std::endl;
		return 0;
	}

	const std::string path = argv[1];
	ff::VideoReader reader(path);
	if (!reader.isOpened()) {
		std::cout << "Error: failed to open " << path << std::endl;
		return 0;
	}

	while(true) {
		int64_t frame_number = reader.getNextFrameNumber();
		ff::AVFrameCpp frame = reader.nextFrame();
		if (!frame) { break; }

		frame = ff::VideoReader::convert(frame, AVPixelFormat::AV_PIX_FMT_BGR24);
		if (!frame) { break; }

		cv::Mat image(cv::Size(frame->width, frame->height), CV_8UC3, frame->data[0], frame->linesize[0]);
		cv::Mat image_small;
		cv::resize(image, image_small, cv::Size(image.cols / 2, image.rows / 2));

		std::cout << "frame: " << frame_number << std::endl;
		cv::imshow("preview", image);

		int key = cv::waitKey(0);
		if ('q' == key || 0x1b == key) break;
	}

	std::cout << "completed" << std::endl;

	return 0;
}

