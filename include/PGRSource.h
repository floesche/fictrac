/// FicTrac http://rjdmoore.net/fictrac/
/// \file       PGRSource.h
/// \brief      PGR USB2/3 sources (FlyCapture/Spinnaker SDK).
/// \author     Richard Moore
/// \copyright  CC BY-NC-SA 3.0

#if defined(FLYCAPTURE) || defined(SPINNAKER)

#pragma once

#include "FrameSource.h"

#if defined(SPINNAKER)
#include <Spinnaker.h>
#elif defined(FLYCAPTURE)
#include <FlyCapture2.h>
#include <memory>
#endif // FLYCAPTURE/SPINNAKER

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
#if defined(SPINNAKER)
    Spinnaker::SystemPtr _system;
    Spinnaker::CameraList _camList;
    Spinnaker::CameraPtr _cam;
    Spinnaker::ImageProcessor _imageProcessor;
#elif defined(FLYCAPTURE)
    std::shared_ptr<FlyCapture2::Camera> _cam;
#endif // FLYCAPTURE/SPINNAKER
};

#endif // FLYCAPTURE/SPINNAKER
