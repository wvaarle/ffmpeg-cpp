#pragma once

#include "ffmpeg.h"

#include "FrameSinks/VideoFrameSink.h"
#include "FFmpegResource.h"

namespace ffmpegcpp
{
	class VideoFilter : public VideoFrameSink
	{

	public:

		VideoFilter(const char* filterString, VideoFrameSink* target);
		virtual ~VideoFilter();

		void WriteFrame(AVFrame* frame, AVRational* timeBase) override;
		void Close() override;

		bool IsPrimed() override;

	private:

		void InitDelayed(AVFrame* frame, AVRational* timeBase);
		void PollFilterGraphForFrames();

		VideoFrameSink* target;

		const char* filterString;
		AVPixelFormat outputFormat;

		AVFilterGraph *filter_graph = nullptr;
		AVFilterContext *buffersink_ctx = nullptr;
		AVFilterContext *buffersrc_ctx = nullptr;
		FFmpegResource<AVFrame> filt_frame;

		bool initialized = false;

		AVRational* timeBase;

		void CleanUp();
	};


}
