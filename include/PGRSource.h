/// FicTrac http://rjdmoore.net/fictrac/
/// \file       PGRSource.h
/// \brief      PGR USB2/3 sources (FlyCapture/Spinnaker SDK).
/// \author     Richard Moore
/// \copyright  CC BY-NC-SA 3.0

#if defined(PGR_USB2) || defined(PGR_USB3)

#pragma once

#include "FrameSource.h"

#if defined(PGR_USB3)
#include <Spinnaker.h>
#elif defined(PGR_USB2)
#include <FlyCapture2.h>
#include <memory>
#endif // PGR_USB2/3

#include <opencv2/opencv.hpp>

#if defined(PGR_USB3)
// Include System.h to get version macros
#include <System.h>

// Compile-time version detection macros
#define SPINNAKER_VERSION_AT_LEAST(major, minor) \
    ((FLIR_SPINNAKER_VERSION_MAJOR > (major)) || \
     (FLIR_SPINNAKER_VERSION_MAJOR == (major) && FLIR_SPINNAKER_VERSION_MINOR >= (minor)))

// Forward declaration for version-specific helper functions
namespace {
    Spinnaker::ImagePtr convertImageCompat(const Spinnaker::ImagePtr& srcImage, Spinnaker::PixelFormatEnums destFormat);
    void setDefaultColorProcessingCompat();
}
#endif // PGR_USB3

class PGRSource : public FrameSource {
public:
	PGRSource(int index=0);
	virtual ~PGRSource();

    virtual double getFPS();
	virtual bool setFPS(double fps);
    virtual bool rewind() { return false; };
	virtual bool grab(cv::Mat& frame);

private:
#if defined(PGR_USB3)
    Spinnaker::SystemPtr _system;
    Spinnaker::CameraList _camList;
    Spinnaker::CameraPtr _cam;
#elif defined(PGR_USB2)
    std::shared_ptr<FlyCapture2::Camera> _cam;
#endif // PGR_USB2/3
};

#endif // PGR_USB2/3
