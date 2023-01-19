/// FicTrac http://rjdmoore.net/fictrac/
/// \file       BaslerSource.cpp
/// \brief      Basler USB3 sources (Pylon SDK).
/// \author     Wenbin Yang, Richard Moore
/// \copyright  CC BY-NC-SA 3.0

#if defined(BASLER_USB3)

#include "BaslerSource.h"

#include "Logger.h"
#include "timing.h"

#include <algorithm>

using namespace std;
using namespace cv;
using namespace Pylon;

BaslerSource::BaslerSource(int index, int src_width, int src_height)
{
    PylonInitialize();
    LOG("Pylon initialized");

    

    // create an instant camera object
    if (!_cam.IsPylonDeviceAttached()){
        LOG("Attach camera");
        _cam.Attach(CTlFactory::GetInstance().CreateFirstDevice());
    }

    _cam.Open();

    const GenApi::CIntegerPtr roiWidth = _cam.GetNodeMap().GetNode("Width");
    const GenApi::CIntegerPtr roiHeight = _cam.GetNodeMap().GetNode("Height");
    if (IsWritable(roiWidth) && IsWritable(roiHeight)) {
        roiWidth->SetValue(src_width);
        roiHeight->SetValue(src_height);
    } else {
        LOG_ERR("Error setting source width and height in constructor");
    }

    LOG("Opening Basler camera device: %s %s version %s, serial number %s", 
        _cam.GetDeviceInfo().GetVendorName().c_str(), 
        _cam.GetDeviceInfo().GetModelName().c_str(), 
        _cam.GetDeviceInfo().GetDeviceVersion().c_str(),
        _cam.GetDeviceInfo().GetSerialNumber().c_str());
    
    // Start acquisition
    if (!_cam.IsGrabbing()) {
        LOG("Start grabbing");
        _cam.StartGrabbing();
    }
    // Get some params
    GenApi::INodeMap &control = _cam.GetNodeMap();
    const GenApi::CIntegerPtr camWidth = control.GetNode("Width");
    const GenApi::CIntegerPtr camHeight = control.GetNode("Height");
    _width = camWidth->GetValue();
    _height = camHeight->GetValue();
    _fps = getFPS();

    LOG("Basler camera initialised (%dx%d @ %.3f fps)!", _width, _height, _fps);

    _open = true;
    _live = true;
}

BaslerSource::~BaslerSource()
{
    if (_open) {
        if (_cam.IsGrabbing())
            _cam.StopGrabbing();
        try {
            LOG("Closing and Detaching Camera");
            _cam.Close();
	        _cam.DetachDevice();
        }
        catch (const GenericException &e) {
            LOG_ERR("Error closing capture device! Error was: %s", e.GetDescription());
        }
        _open = false;
    }
    PylonTerminate();
    LOG("Pylon Terminated");
}

double BaslerSource::getFPS()
{
    const GenApi::CFloatPtr camFrameRate = _cam.GetNodeMap().GetNode("ResultingFrameRate");
    return camFrameRate->GetValue();
}

bool BaslerSource::setFPS(double fps)
{
    using namespace GenApi;

    bool ret = false;
    if (_open && (fps > 0)) {
        // Get the camera control object.
        const GenApi::CFloatPtr camFrameRate = _cam.GetNodeMap().GetNode("ResultingFrameRateAbs");// TODO: test this line
        if (IsWritable(camFrameRate))
        {
            camFrameRate->SetValue(fps);
            ret = true;
        }
        else {
            LOG_ERR("Error setting frame rate!");
        }
        _fps = getFPS();
        LOG("Device frame rate is now %.2f", _fps);
    }
    return ret;
}

bool BaslerSource::setOffset(int src_offset_x, int src_offset_y){
    using namespace GenApi;
    bool ret = false;
    if (_open){
        const GenApi::CIntegerPtr roiOffsetX = _cam.GetNodeMap().GetNode("OffsetX");
        const GenApi::CIntegerPtr roiOffsetY = _cam.GetNodeMap().GetNode("OffsetY");
        if (IsWritable(roiOffsetX) && IsWritable(roiOffsetY)){
            roiOffsetX->SetValue(src_offset_x);
            roiOffsetY->SetValue(src_offset_y);
            LOG("Set offset to %d and %d", src_offset_x, src_offset_y);
            ret = true;
        } else {
            LOG_ERR("Error setting source offset");
        }
    }
    return ret;
}

bool BaslerSource::setFlip(bool flip_x, bool flip_y){
    using namespace GenApi;
    bool ret = false;
    if (_open){
        const GenApi::CBooleanPtr reverse_x = _cam.GetNodeMap().GetNode("ReverseX");
        const GenApi::CBooleanPtr reverse_y = _cam.GetNodeMap().GetNode("ReverseY");
        if (IsWritable(reverse_x)){
            reverse_x->SetValue(flip_x);
        } else {
            LOG_ERR("Reverse X does not work");
        }
        if (IsWritable(reverse_y)){
            reverse_y->SetValue(flip_y);
            LOG("Flip Y");
            ret = true;
        } else {
            LOG_ERR("Reverse Y does not work");
        }
    }
    return ret;
}

bool BaslerSource::setExposureTime(double exp_time){
    using namespace GenApi;
    bool ret = false;
    if (_open){
        const GenApi::CEnumerationPtr exposure_auto = _cam.GetNodeMap().GetNode("ExposureAuto");
        if(IsWritable(exposure_auto)){
            exposure_auto->SetIntValue(Basler_UsbCameraParams::ExposureAuto_Off);
            LOG("Auto Exposure off");
        } else {
            LOG_ERR("Cannot turn auto exposure off");
        }
        const GenApi::CEnumerationPtr throughputLimit = _cam.GetNodeMap().GetNode("DeviceLinkThroughputLimitMode");
        if(IsWritable(exposure_auto)){
            throughputLimit->SetIntValue(Basler_UsbCameraParams::DeviceLinkThroughputLimitMode_Off);
            LOG("ThroughputLimit Off");
        } else {
            LOG_ERR("Cannot disable throughput mode");
        }
        
        const GenApi::CFloatPtr exposure_time = _cam.GetNodeMap().GetNode("ExposureTime");
        if(IsWritable(exposure_time)){
            exposure_time->SetValue(exp_time);
            LOG("Setting exposure time");
            ret = true;
        } else {
            LOG_ERR("Cannot set exposure time");
        }
    }
    return ret;
}

bool BaslerSource::grab(cv::Mat& frame)
{
    if (!_open) { return false; }

    // Set grab timeout
    long int timeout = _fps > 0 ? max(static_cast<long int>(1000), static_cast<long int>(1000. / _fps)) : 1000; // set capture timeout to at least 1000 ms
    try {
        _cam.RetrieveResult(timeout, _ptrGrabResult, TimeoutHandling_ThrowException);
        double ts = ts_ms();    // backup, in case the device timestamp is junk
        //_timestamp = pgr_image->GetTimeStamp();   // TODO: extract timestamp
        if (!_ptrGrabResult->GrabSucceeded()) {
            _cam.RetrieveResult(timeout, _ptrGrabResult, TimeoutHandling_ThrowException);
        }
        if (!_ptrGrabResult->GrabSucceeded()) {
            LOG_ERR("Error! Image capture failed (%d: %s).", _ptrGrabResult->GetErrorCode(), _ptrGrabResult->GetErrorDescription().c_str());
            // release the original image pointer
            _ptrGrabResult.Release();
            return false;
        }
        else { // TODO: no getNumChannels method found, use GetPixelType() instead.
            LOG_DBG("Frame captured %dx%dx%d @ %f (%f)", _ptrGrabResult->GetWidth(), _ptrGrabResult->GetHeight(), _ptrGrabResult->GetPixelType(), _timestamp, ts);
        }
        if (_timestamp <= 0) {
            _timestamp = ts;
        }
    }
    catch (const GenericException &e) {
        LOG_ERR("Error grabbing frame! Error was: %s", e.GetDescription());
        // release the original image pointer
        _ptrGrabResult.Release();
        return false;
    }

    try {
        // Convert image
        Pylon::CImageFormatConverter formatConverter;
        formatConverter.OutputPixelFormat = PixelType_BGR8packed;
        formatConverter.Convert(_pylonImg, _ptrGrabResult);

        Mat tmp(_height, _width, CV_8UC3, (uint8_t*)_pylonImg.GetBuffer()); 
        tmp.copyTo(frame);

        // release the original image pointer
        _ptrGrabResult.Release();
    }
    catch (const GenericException &e) {
        LOG_ERR("Error converting frame! Error was: %s", e.GetDescription());
        // release the original image pointer
        _ptrGrabResult.Release();
        return false;
    }

    return true;
}

#endif // BASLER_USB3
