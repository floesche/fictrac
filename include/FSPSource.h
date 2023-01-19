/// FicTrac http://rjdmoore.net/fictrac/
/// \file       FSPSource.h
/// \brief      FLIR Spinnaker sources (FlyCapture/Spinnaker SDK).
/// \author     Richard Moore
/// \author     Frank Loesche
/// \copyright  CC BY-NC-SA 4.0

#pragma once
#include "FrameSource.h"
#include <Spinnaker.h>

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
    Spinnaker::SystemPtr _system;
    Spinnaker::CameraList _camList;
    Spinnaker::CameraPtr _cam;
    #if FLIR_SPINNAKER_VERSION_MAJOR >= 3
    Spinnaker::ImageProcessor _processor;
    #endif

};


