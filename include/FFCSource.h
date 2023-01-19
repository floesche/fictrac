/// FicTrac http://rjdmoore.net/fictrac/
/// \file       FFCSource
/// \brief      FLIR FlyCapture sources (FlyCapture SDK).
/// \author     Richard Moore
/// \author     Frank Loesche
/// \copyright  CC BY-NC-SA 4.0

#pragma once

#include "FrameSource.h"
#include <FlyCapture2.h>
#include <memory>
#include <opencv2/opencv.hpp>

class PGRSource : public FrameSource {
public:
	PGRSource(int index=0);
	virtual ~PGRSource();

    virtual double getFPS();
	virtual bool setFPS(double fps);
    virtual bool rewind() { return false; };
	virtual bool grab(cv::Mat& frame);

private:
    std::shared_ptr<FlyCapture2::Camera> _cam;
};
