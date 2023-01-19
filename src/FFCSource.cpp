/// FicTrac http://rjdmoore.net/fictrac/
/// \file       PFCSource.cpp
/// \brief      FLIR USB2 sources (FlyCapture SDK).
/// \author     Richard Moore
/// \author     Frank Loesche
/// \copyright  CC BY-NC-SA 4.0

#include "FFCSource.h"

#include "Logger.h"
#include "timing.h"
using namespace FlyCapture2;
using cv::Mat;

PGRSource::PGRSource(int index)
{
    try {
        LOG_DBG("Looking for camera at index %d...", index);

        BusManager busMgr;
        PGRGuid guid;
        Error error = busMgr.GetCameraFromIndex(index, &guid);
        if (error != PGRERROR_OK) {
            LOG_ERR("Error reading camera GUID!");
            return;
        }

        _cam = std::make_shared<Camera>();
        error = _cam->Connect(&guid);
        if (error != PGRERROR_OK) {
            LOG_ERR("Error connecting to camera!");
            return;
        }

        CameraInfo camInfo;
        error = _cam->GetCameraInfo(&camInfo);
        if (error != PGRERROR_OK) {
            LOG_ERR("Error retrieving camera information!");
            return;
        }
        else {
            LOG_DBG("Connected to PGR camera (%s/%s max res: %s)", camInfo.modelName, camInfo.sensorInfo, camInfo.sensorResolution);
        }

        error = _cam->StartCapture();
        if (error != PGRERROR_OK) {
            LOG_ERR("Error starting video capture!");
            return;
        }

        Image::SetDefaultColorProcessing(ColorProcessingAlgorithm::NEAREST_NEIGHBOR);

        // capture test image
        Image testImg;
        error = _cam->RetrieveBuffer(&testImg);
        if (error != PGRERROR_OK) {
            LOG_ERR("Error capturing image!");
            return;
        }
        _width = testImg.GetCols();
        _height = testImg.GetRows();
        _fps = getFPS();

        LOG("PGR camera initialised (%dx%d @ %.3f fps)!", _width, _height, _fps);

        _open = true;
        _live = true;
    }
    catch (...) {
        LOG_ERR("Error opening capture device!");
    }
}

PGRSource::~PGRSource()
{
    if (_open) {
        try {
            _cam->StopCapture();
        }
        catch (...) {
            LOG_ERR("Error ending acquisition!");
        }
        _open = false;
    }
    _cam->Disconnect();
    _cam = NULL;
}

double PGRSource::getFPS()
{
    double fps = _fps;
    if (_open) {
    }
    return fps;
}

bool PGRSource::setFPS(double fps)
{
    bool ret = false;
    if (_open && (fps > 0)) {
        _fps = getFPS();
        LOG("Device frame rate is now %.2f", _fps);
        ret = true;
    }
    return ret;
}

bool PGRSource::grab(cv::Mat& frame)
{
	if( !_open ) { return false; }
    Image frame_raw;
    Error error = _cam->RetrieveBuffer(&frame_raw);
    double ts = ts_ms();    // backup, in case the device timestamp is junk
    //LOG_DBG("Frame captured %dx%d%d @ %f (%f)", 7479c6c5650ce7201b6a5583df6cf8962dd9cf4e
    auto timestamp = frame_raw.GetTimeStamp();
    _timestamp = timestamp.seconds * 1e3 + timestamp.microSeconds / (double)1e3;
    if (_timestamp <= 0) {
        _timestamp = ts;
    }

    Image frame_bgr;
    error = frame_raw.Convert(PIXEL_FORMAT_BGR, &frame_bgr);
    if (error != PGRERROR_OK) {
        LOG_ERR("Error converting image format!");
        return false;
    }
    Mat frame_cv(frame_bgr.GetRows(), frame_bgr.GetCols(), CV_8UC3, frame_bgr.GetData(), frame_bgr.GetStride());
    frame_cv.copyTo(frame);
    return true;
}
