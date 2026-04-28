/// FicTrac http://rjdmoore.net/fictrac/
/// \file       PGRSource.cpp
/// \brief      PGR USB2/3 sources (FlyCapture/Spinnaker SDK).
/// \author     Richard Moore
/// \copyright  CC BY-NC-SA 3.0

#if defined(FLYCAPTURE) || defined(SPINNAKER)

#include "PGRSource.h"

#include "Logger.h"
#include "timing.h"

#if defined(SPINNAKER)
#include "SpinGenApi/SpinnakerGenApi.h"
using namespace Spinnaker;
#elif defined(FLYCAPTURE)
using namespace FlyCapture2;
#endif // FLYCAPTURE/SPINNAKER

using cv::Mat;

PGRSource::PGRSource(int index)
{
    try {
#if defined(SPINNAKER)
        // Retrieve singleton reference to system object
        _system = System::GetInstance();

        // Print out current library version
        const LibraryVersion spinnakerLibraryVersion = _system->GetLibraryVersion();
        LOG("Opening PGR camera using Spinnaker SDK (version %d.%d.%d.%d)",
            spinnakerLibraryVersion.major, spinnakerLibraryVersion.minor,
            spinnakerLibraryVersion.type, spinnakerLibraryVersion.build);

        // Retrieve list of cameras from the system
        _camList = _system->GetCameras();

        unsigned int numCameras = _camList.GetSize();

        if (numCameras == 0) {
            LOG_ERR("Error! Could not find any connected PGR cameras!");
            return;
        }
        else {
            LOG_DBG("Found %d PGR cameras. Connecting to camera %d..", numCameras, index);
        }

        // Select camera
        _cam = _camList.GetByIndex(index);

        // Initialize camera
        _cam->Init();

        // set acquisition mode - needed?
        {
            // Retrieve GenICam nodemap
            Spinnaker::GenApi::INodeMap& nodeMap = _cam->GetNodeMap();

            // Retrieve enumeration node from nodemap
            Spinnaker::GenApi::CEnumerationPtr ptrAcquisitionMode = nodeMap.GetNode("AcquisitionMode");
            if (!IsAvailable(ptrAcquisitionMode) || !IsWritable(ptrAcquisitionMode)) {
                LOG_ERR("Unable to set acquisition mode to continuous (enum retrieval)!");
                return;
            }

            // Retrieve entry node from enumeration node
            Spinnaker::GenApi::CEnumEntryPtr ptrAcquisitionModeContinuous = ptrAcquisitionMode->GetEntryByName("Continuous");
            if (!IsAvailable(ptrAcquisitionModeContinuous) || !IsReadable(ptrAcquisitionModeContinuous)) {
                LOG_ERR("Unable to set acquisition mode to continuous (entry retrieval)!");
                return;
            }

            // Retrieve integer value from entry node
            int64_t acquisitionModeContinuous = ptrAcquisitionModeContinuous->GetValue();

            // Set integer value from entry node as new value of enumeration node
            ptrAcquisitionMode->SetIntValue(acquisitionModeContinuous);

            LOG_DBG("Acquisition mode set to continuous.");
        }

        // Configure image processor (Spinnaker 4.x: IImage::Convert moved to ImageProcessor)
        _imageProcessor.SetColorProcessing(SPINNAKER_COLOR_PROCESSING_ALGORITHM_NEAREST_NEIGHBOR);

        // Begin acquiring images
        _cam->BeginAcquisition();

        // Get some params
        _width = _cam->Width();
        _height = _cam->Height();
        _fps = getFPS();
#elif defined(FLYCAPTURE)
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
#endif // FLYCAPTURE/SPINNAKER

        LOG("PGR camera initialised (%dx%d @ %.3f fps)!", _width, _height, _fps);

        _open = true;
        _live = true;
    }
#if defined(SPINNAKER)
    catch (Spinnaker::Exception& e) {
        LOG_ERR("Error opening capture device! Error was: %s", e.what());
    }
#endif // SPINNAKER
    catch (...) {
        LOG_ERR("Error opening capture device!");
    }
}

PGRSource::~PGRSource()
{
    if (_open) {
        try {
#if defined(SPINNAKER)
            _cam->EndAcquisition();
#elif defined(FLYCAPTURE)
            _cam->StopCapture();
#endif // FLYCAPTURE/SPINNAKER
        }
#if defined(SPINNAKER)
        catch (Spinnaker::Exception& e) {
            LOG_ERR("Error ending acquisition! Error was: %s", e.what());
        }
#endif // SPINNAKER
        catch (...) {
            LOG_ERR("Error ending acquisition!");
        }
        _open = false;
    }

#if defined(FLYCAPTURE)
    _cam->Disconnect();
#endif // FLYCAPTURE

    _cam = NULL;

#if defined(SPINNAKER)
    // Clear camera list before releasing system
    _camList.Clear();

    // Release system
    _system->ReleaseInstance();
#endif // SPINNAKER
    
}

double PGRSource::getFPS()
{
    double fps = _fps;
    if (_open) {
#if defined(SPINNAKER)
        try {
            fps = _cam->AcquisitionResultingFrameRate();
        }
        catch (Spinnaker::Exception& e) {
            LOG_ERR("Error retrieving camera frame rate! Error was: %s", e.what());
        }
        catch (...) {
            LOG_ERR("Error retrieving camera frame rate!");
        }
#endif // SPINNAKER
    }
    return fps;
}

bool PGRSource::setFPS(double fps)
{
    bool ret = false;
    if (_open && (fps > 0)) {
#if defined(SPINNAKER)
        try {
            _cam->AcquisitionFrameRateEnable.SetValue(true);
            _cam->AcquisitionFrameRate.SetValue(fps);
        }
        catch (Spinnaker::Exception& e) {
            LOG_ERR("Error setting frame rate! Error was: %s", e.what());
        }
        catch (...) {
            LOG_ERR("Error setting frame rate!");
        }
#endif // SPINNAKER
        _fps = getFPS();
        LOG("Device frame rate is now %.2f", _fps);
        ret = true;
    }
    return ret;
}

bool PGRSource::grab(cv::Mat& frame)
{
	if( !_open ) { return false; }

#if defined(SPINNAKER)
    ImagePtr pgr_image = NULL;

    try {
        // Retrieve next received image
        long int timeout = _fps > 0 ? std::max(static_cast<long int>(1000), static_cast<long int>(1000. / _fps)) : 1000; // set capture timeout to at least 1000 ms
        pgr_image = _cam->GetNextImage(timeout);
        double ts = ts_ms();    // backup, in case the device timestamp is junk
        _ms_since_midnight = ms_since_midnight();
        _timestamp = pgr_image->GetTimeStamp();
        LOG_DBG("Frame captured %dx%d%d @ %f (t_sys: %f ms, t_day: %f ms)", pgr_image->GetWidth(), pgr_image->GetHeight(), pgr_image->GetNumChannels(), _timestamp, ts, _ms_since_midnight);
        if (_timestamp <= 0) {
            _timestamp = ts;
        }

        // Ensure image completion
        if (pgr_image->IsIncomplete()) {
            // Retreive and print the image status description
            LOG_ERR("Error! Image capture incomplete (%s).", Image::GetImageStatusDescription(pgr_image->GetImageStatus()));
            pgr_image->Release();
            return false;
        }
    }
    catch (Spinnaker::Exception& e) {
        LOG_ERR("Error grabbing frame! Error was: %s", e.what());
        pgr_image->Release();
        return false;
    }
    catch (...) {
        LOG_ERR("Error grabbing frame!");
        pgr_image->Release();
        return false;
    }

    try {
        // Convert image
        ImagePtr bgr_image = _imageProcessor.Convert(pgr_image, PixelFormat_BGR8);

        Mat tmp(_height, _width, CV_8UC3, bgr_image->GetData(), bgr_image->GetStride());
        tmp.copyTo(frame);

        // We have to release our original image to clear space on the buffer
        pgr_image->Release();

        return true;
    }
    catch (Spinnaker::Exception& e) {
        LOG_ERR("Error converting frame! Error was: %s", e.what());
        pgr_image->Release();
        return false;
    }
    catch (...) {
        LOG_ERR("Error converting frame!");
        pgr_image->Release();
        return false;
    }
#elif defined(FLYCAPTURE)
    Image frame_raw;
    Error error = _cam->RetrieveBuffer(&frame_raw);
    double ts = ts_ms();    // backup, in case the device timestamp is junk
    //LOG_DBG("Frame captured %dx%d%d @ %f (%f)", pgr_image->GetWidth(), pgr_image->GetHeight(), pgr_image->GetNumChannels(), _timestamp, ts);
    if (error != PGRERROR_OK) {
        LOG_ERR("Error grabbing image frame!");
        return false;
    }
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
#endif // FLYCAPTURE/SPINNAKER
}

#endif // FLYCAPTURE/SPINNAKER
