#pragma once

#include <memory>

#include "ffmpegcpp.h"

using namespace ffmpegcpp;

class GeneratedVideoSource : public InputSource
{
public:

	GeneratedVideoSource(int width, int height, VideoFrameSink* frameSink);
	~GeneratedVideoSource() override;

	void PreparePipeline() override;
	bool IsDone() override;
	void Step() override;

private:

	std::unique_ptr<RawVideoDataSource> output;

	int frameNumber = 0;

	uint8_t *rgb = nullptr;
};

