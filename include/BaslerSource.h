/// FicTrac http://rjdmoore.net/fictrac/
/// \file       BaslerSource.h
/// \brief      Basler USB3 sources (Pylon SDK).
/// \author     Wenbin Yang
/// \copyright  CC BY-NC-SA 3.0

#if defined(BASLER_USB3)

#pragma once

#include "FrameSource.h"

// Include Basler Pylon libraries
#include <pylon/PylonIncludes.h>
#include <pylon/ImageFormatConverter.h>
#include <pylon/usb/BaslerUsbInstantCameraArray.h>

class BaslerSource : public FrameSource {
public:
    BaslerSource(int index=0, int width=400, int height=400);
    ~BaslerSource();

    double getFPS();
    bool setFPS(double);

    bool setOffset(int, int);

    bool setFlip(bool, bool);
    bool setExposureTime(double);

    bool rewind() { return false; };
    bool grab(cv::Mat& frame);

    private:
    Pylon::CPylonImage _pylonImg;
    Pylon::CGrabResultPtr _ptrGrabResult;
    Pylon::CInstantCamera _cam;
};

#endif // BASLER_USB3
