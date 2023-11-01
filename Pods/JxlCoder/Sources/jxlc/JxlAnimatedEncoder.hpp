//
//  JxlAnimatedEncoder.hpp
//  JxclCoder [https://github.com/awxkee/jxl-coder-swift]
//
//  Created by Radzivon Bartoshyk on 26/10/2023.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
//

#ifndef JxlAnimatedEncoder_hpp
#define JxlAnimatedEncoder_hpp

#ifdef __cplusplus

#include <stdio.h>
#include <jxl/encode.h>
#include <jxl/encode_cxx.h>
#include <jxl/thread_parallel_runner_cxx.h>
#include <string>
#include "JxlDefinitions.h"
#include <vector>
#include <thread>

class AnimatedEncoderError : public std::exception {
public:
    AnimatedEncoderError(const std::string& message) : errorMessage(message) {}

    const char* what() const noexcept override {
        return errorMessage.c_str();
    }

private:
    std::string errorMessage;
};

class JxlAnimatedEncoder {
public:
    JxlAnimatedEncoder(int width, int height, JxlPixelType pixelType, 
                       JxlEncodingPixelFormat encodingPixelFormat, 
                       JxlCompressionOption compressionOption, 
                       int numLoops, int quality, int effort): width(width), height(height),
    pixelType(pixelType), encodingPixelFormat(encodingPixelFormat),
    compressionOption(compressionOption), quality(quality), effort(effort) {
        if (!enc || !runner) {
            std::string str = "Cannot initialize encoder";
            throw AnimatedEncoderError(str);
        }
        if (JXL_ENC_SUCCESS != JxlEncoderSetParallelRunner(enc.get(),
                                                           JxlThreadParallelRunner,
                                                           runner.get())) {
            std::string str = "Cannot initialize parallel runner";
            throw AnimatedEncoderError(str);
        }

        pixelFormat = {3, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0};
        switch (pixelType) {
            case rgb:
                pixelFormat = {3, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0};
                break;
            case rgba:
                pixelFormat = {4, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0};
                break;
        }

        if (encodingPixelFormat == efloat16) {
            pixelFormat.data_type = JXL_TYPE_FLOAT16;
        } else {
            pixelFormat.data_type = JXL_TYPE_UINT8;
        }

        JxlEncoderInitBasicInfo(&basicInfo);
        basicInfo.xsize = width;
        basicInfo.ysize = height;
        basicInfo.bits_per_sample = 8;
        basicInfo.uses_original_profile = compressionOption == loosy ? JXL_FALSE : JXL_TRUE;
        basicInfo.num_color_channels = 3;

        basicInfo.animation.tps_numerator = 1000;
        basicInfo.animation.tps_denominator = 1;
        basicInfo.animation.num_loops = static_cast<uint32_t>(numLoops);
        basicInfo.animation.have_timecodes = false;
        basicInfo.have_animation = true;

        if (JXL_ENC_SUCCESS != JxlEncoderSetCodestreamLevel(enc.get(), 10)) {
            std::string str = "Cannot set codestream level";
            throw AnimatedEncoderError(str);
        }

        if (pixelType == rgba) {
            basicInfo.num_extra_channels = 1;
            basicInfo.alpha_bits = 8;
        }

        if (JXL_ENC_SUCCESS != JxlEncoderSetBasicInfo(enc.get(), &basicInfo)) {
            std::string str = "Cannot set basic info to encoder";
            throw AnimatedEncoderError(str);
        }

        switch (pixelType) {
            case rgb:
                basicInfo.num_color_channels = 3;
                break;
            case rgba:
                basicInfo.num_color_channels = 4;
                JxlExtraChannelInfo channelInfo;
                JxlEncoderInitExtraChannelInfo(JXL_CHANNEL_ALPHA, &channelInfo);
                channelInfo.bits_per_sample = 8;
                channelInfo.alpha_premultiplied = false;
                if (JXL_ENC_SUCCESS != JxlEncoderSetExtraChannelInfo(enc.get(), 0, &channelInfo)) {
                    std::string str = "Cannot set extra channel to encoder";
                    throw AnimatedEncoderError(str);
                }
                break;
        }

        JxlColorEncoding colorEncoding = {};
        JxlColorEncodingSetToSRGB(&colorEncoding, pixelFormat.num_channels < 3);
        if (JXL_ENC_SUCCESS !=
            JxlEncoderSetColorEncoding(enc.get(), &colorEncoding)) {
            std::string str = "Cannot set color encoding";
            throw AnimatedEncoderError(str);
        }

        frameSettings =
                JxlEncoderFrameSettingsCreate(enc.get(), nullptr);

        JxlBitDepth depth;
        if (encodingPixelFormat == efloat16) {
            depth.bits_per_sample = 16;
            depth.exponent_bits_per_sample = 5;
        } else {
            depth.bits_per_sample = 8;
            depth.exponent_bits_per_sample = 0;
        }
        depth.type = JXL_BIT_DEPTH_FROM_PIXEL_FORMAT;

        if (JXL_ENC_SUCCESS != JxlEncoderSetFrameBitDepth(frameSettings, &depth)) {
            std::string str = "Set bit depth to frame has failed";
            throw AnimatedEncoderError(str);
        }

        if (JXL_ENC_SUCCESS != JxlEncoderSetFrameLossless(frameSettings, compressionOption == loseless)) {
            std::string str = "Set frame to loseless has failed";
            throw AnimatedEncoderError(str);
        }

        if (JXL_ENC_SUCCESS !=
                   JxlEncoderSetFrameDistance(frameSettings, JXLGetDistance(quality))) {
            std::string str = "Set frame distance has failed";
            throw AnimatedEncoderError(str);
        }

        if (pixelType == rgba) {
            if (JXL_ENC_SUCCESS !=
                       JxlEncoderSetExtraChannelDistance(frameSettings, 0, JXLGetDistance(quality))) {
                std::string str = "Set extra channel distance has failed";
                throw AnimatedEncoderError(str);
            }
        }

        if (JxlEncoderFrameSettingsSetOption(frameSettings,
                                             JXL_ENC_FRAME_SETTING_EFFORT, effort) != JXL_ENC_SUCCESS) {
            std::string str = "Set effort has failed";
            throw AnimatedEncoderError(str);
        }


    }

    void addFrame(std::vector<uint8_t>& data, int frameTime);
    void encode(std::vector<uint8_t>& dst);

    int getWidth() {
        return width;
    }

    int getHeight() {
        return height;
    }

    JxlPixelFormat getPixelFormat() {
        return pixelFormat;
    }

    JxlPixelType getJxlPixelType() {
        return pixelType;
    }

    ~JxlAnimatedEncoder();
private:
    const int width;
    const int height;
    const int quality;
    const int effort;
    const JxlPixelType pixelType;
    const JxlEncodingPixelFormat encodingPixelFormat;
    const JxlCompressionOption compressionOption;
    JxlPixelFormat pixelFormat;
    int addedFrames = 0;

    JxlEncoderPtr enc = JxlEncoderMake(nullptr);
    JxlThreadParallelRunnerPtr runner = JxlThreadParallelRunnerMake(nullptr,
                                                                    JxlThreadParallelRunnerDefaultNumWorkerThreads());

    JxlBasicInfo basicInfo;
    JxlFrameHeader header;
    JxlEncoderFrameSettings * frameSettings;

    float JXLGetDistance(const int quality)
    {
        if (quality == 0)
            return(1.0f);
        float distance = quality >= 100 ? 0.0
                           : quality >= 30
                               ? 0.1 + (100 - quality) * 0.09
                               : 53.0 / 3000.0 * quality * quality -
                                     23.0 / 20.0 * quality + 25.0;
        return distance;
    }

    std::mutex lock;
};

#endif

#endif /* JxlAnimatedEncoder_hpp */
