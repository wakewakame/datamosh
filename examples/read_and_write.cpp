#include <iostream>
#include "ffmpeg_cpp.h"

int main(int argc, char **argv)
{
	if (2 != argc) {
		std::cout << "Usage: read_and_write <video path>" << std::endl;
		return 0;
	}

	std::string path = argv[1];
	ff::VideoReader reader(path);
	if (!reader.isOpened()) {
		std::cout << "Error: failed to open " << path << std::endl;
		return 0;
	}

	ff::VideoWriter writer("output.mp4", reader.getTimeBase());

	while(true) {
		int64_t frame_number = reader.getNextFrameNumber();
		ff::AVFrameCpp frame = reader.nextFrame();
		if (!frame) { break; }

		writer.nextFrame(move(frame));

		std::cout << "frame: " << frame_number << std::endl;
	}

	writer.flush();

	std::cout << "completed" << std::endl;

	return 0;
}

