
#include <iostream>
#include <memory>

#include "GeneratedVideoSource.h"
#include "GeneratedAudioSource.h"

#include <ffmpegcpp.h>

// This example demonstrates different combinations and usages for ffmpeg-cpp.
// Configure it by changing the values below.
// The list of supported codecs is not limited to the ones mentioned below,
// this is just a list we support in this demo. You can use any codec that is
// supported by ffmpeg, but you might have to write your own Codec-class wrapper
// to make it work. Look at the examples in the Codecs-dir to see how it is done.
void PlayDemo(int argc, char** argv)
{

	// These are example video and audio sources used below.
	const char* rawVideoFile = "samples/carphone_qcif.y4m";
	int rawVideoWidth = 176; int rawVideoHeight = 162;
	const char* rawAudioFile = "samples/Vivaldi_s16le_2_channels_samplerate_11025.dat";
	const char* rawAudioFormat = "s16le"; int rawAudioSampleRate = 11025; int rawAudioChannels = 2;

	const char* encodedVideoFile = "samples/carphone.h264";
	const char* encodedAudioFile = "samples/Vivaldi_Sonata_eminor_.mp3";

	const char* containerWithVideoAndAudioFile = "samples/big_buck_bunny.mp4";
	const char* containerWithAudioFile = "samples/DesiJourney.wav";

	// hard-code the settings here, but let them be overridden by the arguments
	std::string inputAudioSource = "CONTAINER"; // options are RAW, ENCODED, CONTAINER, GENERATED
	std::string inputVideoSource = "ENCODED"; // options are RAW, ENCODED, CONTAINER, GENERATED
	std::string outputAudioCodec = "AAC"; // options are MP2, AAC, NONE
	std::string outputVideoCodec = "H264"; // options are H264, H265, VP9, NONE (H264 and H265 only work on Nvidia hardware)
	std::string outputContainerName = "out.mp4"; // container format is deduced from extension so use a known one

	// you can use any filter string that you can use in the ffmpeg command-line here
	// set the filter to NULL to disable filtering.
	// See https://trac.ffmpeg.org/wiki/FilteringGuide for more info
	// This example rotates the entire video and then puts a vignette on top of it.
	const char* videoFilterConfig = nullptr;//"transpose=cclock[middle];[middle]vignette"

	// if command line is specified, we overwrite our hard-coded settings
	if (argc >= 6)
	{
		inputAudioSource = std::string(argv[1]);
		inputVideoSource = std::string(argv[2]);
		outputAudioCodec = std::string(argv[3]);
		outputVideoCodec = std::string(argv[4]);
		videoFilterConfig = argv[5];
	}


	// create the different components that make this come together
	try
	{

		/**
			* CREATE THE OUTPUT CONTAINER
		*/

		// create the output muxer - we'll be adding encoders to it later
		auto muxer = std::make_unique<ffmpegcpp::Muxer>(outputContainerName.c_str());

		/**
			* CONFIGURE AUDIO OUTPUT
		*/

		// create the output encoder based on our setting above
		std::unique_ptr<ffmpegcpp::AudioCodec> audioCodec;
		if (outputAudioCodec == "MP2")
		{
			printf("Encoding audio as MP2...\n");
			audioCodec = std::make_unique<ffmpegcpp::AudioCodec>(AV_CODEC_ID_MP2);

		}
		else if (outputAudioCodec == "AAC")
		{
			printf("Encoding audio as AAC...\n");
			audioCodec = std::make_unique<ffmpegcpp::AudioCodec>(AV_CODEC_ID_AAC);

		}
		else if (outputAudioCodec == "NONE")
		{
			// no codec specified - don't output audio!
		}

		// create an encoder - this encoder will receive raw data from any source (filter, file, container, memory, etc),
		// encode it and send it to the muxer (the output container).
		std::unique_ptr<ffmpegcpp::AudioEncoder> audioEncoder;
		if (audioCodec != nullptr)
		{
			audioEncoder = std::make_unique<ffmpegcpp::AudioEncoder>(audioCodec.get(), muxer.get());
		}

		/**
			* CONFIGURE VIDEO OUTPUT
		*/

		// create the output encoder based on our setting above
		std::unique_ptr<ffmpegcpp::VideoCodec> videoCodec;
		if (outputVideoCodec == "H264")
		{
			printf("Encoding video as H264 on Nvidia GPU...\n");
			auto h264Codec = std::make_unique<ffmpegcpp::H264NVEncCodec>();
			h264Codec->SetPreset("hq");
			videoCodec = std::move(h264Codec);
		}
		else if (outputVideoCodec == "H265")
		{
			printf("Encoding video as H265 on Nvidia GPU...\n");
			auto h265Codec = std::make_unique<ffmpegcpp::H265NVEncCodec>();
			h265Codec->SetPreset("hq");
			videoCodec = std::move(h265Codec);
		}
		else if (outputVideoCodec == "VP9")
		{
			printf("Encoding video as VP9...\n");
			auto vp9Codec = std::make_unique<ffmpegcpp::VP9Codec>();
			vp9Codec->SetLossless(true);
			videoCodec = std::move(vp9Codec);
		}
		else if (outputVideoCodec == "NONE")
		{
			// no codec specified - don't output audio!
		}

		// create an encoder for the codec and tie it to the muxer
		// this encoder will receive data from an input source (file, raw, filter, etc), encode it and send it to the output container (muxer)
		std::unique_ptr<ffmpegcpp::VideoEncoder> videoEncoder;
		if (videoCodec != nullptr)
		{
			videoEncoder = std::make_unique<ffmpegcpp::VideoEncoder>(videoCodec.get(), muxer.get());
		}

		/**
			* CONFIGURE AUDIO INPUT
		*/

		// only do this when there is an output - otherwise there is no point in reading audio
		std::unique_ptr<ffmpegcpp::InputSource> audioInputSource;
		if (audioEncoder != nullptr)
		{
			if (inputAudioSource == "RAW")
			{
				printf("Pulling audio from %s...\n", rawAudioFile);
				audioInputSource = std::make_unique<ffmpegcpp::RawAudioFileSource>(rawAudioFile, rawAudioFormat, rawAudioSampleRate, rawAudioChannels, audioEncoder.get());
			}
			else if (inputAudioSource == "ENCODED")
			{
				printf("Pulling audio from %s...\n", encodedAudioFile);
				audioInputSource = std::make_unique<ffmpegcpp::EncodedFileSource>(encodedAudioFile, AV_CODEC_ID_MP3, audioEncoder.get());
			}
			else if (inputAudioSource == "CONTAINER")
			{
				// if the input comes from a container, we use the demuxer class - it is just an input source like any other
				printf("Pulling audio from %s...\n", containerWithAudioFile);
				auto demuxer = std::make_unique<ffmpegcpp::Demuxer>(containerWithAudioFile);
				demuxer->DecodeBestAudioStream(audioEncoder.get());
				audioInputSource = std::move(demuxer);
			}
			else if (inputAudioSource == "GENERATED")
			{
				printf("Generating 440Hz audio tone...\n");
				audioInputSource = std::make_unique<GeneratedAudioSource>(audioEncoder.get());
			}
		}

		/**
			* CONFIGURE VIDEO FILTER IF IT IS USED
		*/

		// If a video filter was specified, we inject it into the pipeline here.
		// Instead of feeding the video source directly to the encoder, we feed it to
		// the video filter instead, which will pass it on to the encoder.
		std::unique_ptr<ffmpegcpp::VideoFilter> videoFilter;
		if (videoFilterConfig != nullptr && videoEncoder != nullptr)
		{
			printf("Applying filter %s to video...\n", videoFilterConfig);
			videoFilter = std::make_unique<ffmpegcpp::VideoFilter>(videoFilterConfig, videoEncoder.get());
		}

		/**
			* CONFIGURE VIDEO INPUT
		*/

		// only do this when there is video output
		std::unique_ptr<ffmpegcpp::InputSource> videoInputSource;
		if (videoEncoder != nullptr)
		{
			if (inputVideoSource == "RAW")
			{
				printf("Pulling video from %s...\n", rawVideoFile);
				videoInputSource = std::make_unique<ffmpegcpp::RawVideoFileSource>(rawVideoFile, videoFilter.get());
			}
			else if (inputVideoSource == "ENCODED")
			{
				printf("Pulling video from %s...\n", encodedVideoFile);
				videoInputSource = std::make_unique<ffmpegcpp::RawVideoFileSource>(encodedVideoFile, videoFilter.get());
			}
			else if (inputVideoSource == "CONTAINER")
			{
				printf("Pulling video from %s...\n", containerWithVideoAndAudioFile);
				auto demuxer = std::make_unique<ffmpegcpp::Demuxer>(containerWithVideoAndAudioFile);
				demuxer->DecodeBestVideoStream(videoFilter.get());
				videoInputSource = std::move(demuxer);
			}
			else if (inputVideoSource == "GENERATED")
			{
				printf("Generating checkerboard video pattern...\n");
				videoInputSource = std::make_unique<GeneratedVideoSource>(640, 480, videoFilter.get());
			}
		}


		/**
		* PROCESS THE DATA
		*/


		// As an awkward but necessary first step, we need to prime each input source.
		// This step decodes a part of the input data and from this, generates info
		// about the stream and propagates it all the way to the output container,
		// which needs to write this data to its header.
		if (videoInputSource != nullptr) videoInputSource->PreparePipeline();
		if (audioInputSource != nullptr) audioInputSource->PreparePipeline();



		// finally, we can start writing data to our pipelines. Open the floodgates
		// to start reading frames from the input, decoding them, optionally filtering them,
		// encoding them and writing them to the final container.
		// This can be interweaved if you want to.
		if (audioInputSource != nullptr)
		{
			while (!audioInputSource->IsDone()) audioInputSource->Step();
		}
		if (videoInputSource != nullptr)
		{
			while (!videoInputSource->IsDone()) videoInputSource->Step();
		}

		// close the muxer and save the file to disk
		muxer->Close();

	}
	catch (ffmpegcpp::FFmpegException e)
	{
		std::cerr << e.what() << '\n';
		throw e;
	}
}

int main(int argc, char **argv)
{
	PlayDemo(argc, argv);

	std::cout << "Encoding complete!" << '\n';
	std::cout << "Press any key to continue..." << '\n';

	getchar();

	return 0;
}