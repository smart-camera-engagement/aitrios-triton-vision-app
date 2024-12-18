// Copyright © 2024 Sony Semiconductor Solutions Corporation. All rights reserved.

/*
* @file device_handler.cpp
* @brief Class for device control
* @author Shogo Takanashi
* @date 2024/11
*/

#include "./device_handler.h"

inline int ceil_roi(int x) {
    return x - (x % 4);
}

ArenaDeviceHandler::ArenaDeviceHandler() {
    pSystem_ = Arena::OpenSystem();
    pSystem_->UpdateDevices(100);
    std::vector<Arena::DeviceInfo> deviceInfos = pSystem_->GetDevices();
    if (deviceInfos.size() == 0)
        throw std::runtime_error("deviceInfos.size() == 0");
    pDevice_ = pSystem_->CreateDevice(deviceInfos[0]);
    util_.SetValue(pDevice_, false);

    util_.InitCameraToOutputDNN(); // To get 30fps, we need to set inEnableNoRawOutput to true
    pNodeMap = pDevice_->GetNodeMap();

    SetNodeParam_(kNodeNameRAWRoiOffsetX_, 0);
    SetNodeParam_(kNodeNameRAWRoiOffsetY_, 0);
    SetNodeParam_(kNodeNameRAWRoiWidth_, kInitRoiWidth_);
    SetNodeParam_(kNodeNameRAWRoiHeight_, kInitRoiHeight_);
    SetNodeParam_(kNodeNameRAWRoiOffsetX_, kInitRoiOffestX_);
    SetNodeParam_(kNodeNameRAWRoiOffsetY_, kInitRoiOffsetY_);

    SetNodeParam_(kNodeNameRoiOffsetX_, 0);
    SetNodeParam_(kNodeNameRoiOffsetY_, 0);
    SetNodeParam_(kNodeNameRoiWidth_, kInitRoiWidth_);
    SetNodeParam_(kNodeNameRoiHeight_, kInitRoiHeight_);
    SetNodeParam_(kNodeNameRoiOffsetX_, kInitRoiOffestX_);
    SetNodeParam_(kNodeNameRoiOffsetY_, kInitRoiOffsetY_);

}
ArenaDeviceHandler::~ArenaDeviceHandler() {
    pSystem_->DestroyDevice(pDevice_);
    Arena::CloseSystem(pSystem_);
}
int ArenaDeviceHandler::GetOperationMode() {
    return op_mode_;
}
void ArenaDeviceHandler::StartStream(const int op_mode) {
    if (is_stream_ != true) {
        op_mode_ = op_mode;
        util_.InitCameraToOutputDNN();
        pDevice_->StartStream();
        is_stream_ = true;
    }
}
void ArenaDeviceHandler::StopStream() {
    if (is_stream_ != false) {
        pDevice_->StopStream();
        is_stream_ = false;
    }
}
bool ArenaDeviceHandler::GetStreamStatus() {
    return is_stream_;
}

void ArenaDeviceHandler::SetNodeParam_(const GENICAM_NAMESPACE::gcstring& node_name, const int node_value) {
    pNode = pNodeMap->GetNode(node_name);
    if (pNode == NULL)
        throw std::runtime_error("Couldn't find the node");
    pNode->SetValue(node_value);
}

void ArenaDeviceHandler::SetDnnRoi(const ROI& roi) {

    if ((roi.offset_x + roi.width <= kSensorWidth) && (roi.offset_y + roi.height <= kSensorHeight)) {
        StopStream();
        SetNodeParam_(kNodeNameRoiOffsetX_, ceil_roi(kInitRoiOffestX_));
        SetNodeParam_(kNodeNameRoiOffsetY_, ceil_roi(kInitRoiOffsetY_));
        SetNodeParam_(kNodeNameRoiWidth_, ceil_roi(roi.width));
        SetNodeParam_(kNodeNameRoiHeight_, ceil_roi(roi.height));
        SetNodeParam_(kNodeNameRoiOffsetX_, ceil_roi(roi.offset_x));
        SetNodeParam_(kNodeNameRoiOffsetY_, ceil_roi(roi.offset_y));
        //StartStream(GetOperationMode());
        current_roi_ = roi; // Store the current ROI
    }
}

void ArenaDeviceHandler::SetRawRoi(const ROI& roi) {
	if ((roi.offset_x + roi.width <= kSensorWidth) && (roi.offset_y + roi.height <= kSensorHeight)) {
		StopStream();
		SetNodeParam_(kNodeNameRAWRoiOffsetX_, ceil_roi(kInitRoiOffestX_));
		SetNodeParam_(kNodeNameRAWRoiOffsetY_, ceil_roi(kInitRoiOffsetY_));
		SetNodeParam_(kNodeNameRAWRoiWidth_, ceil_roi(roi.width));
		SetNodeParam_(kNodeNameRAWRoiHeight_, ceil_roi(roi.height));
		SetNodeParam_(kNodeNameRAWRoiOffsetX_, ceil_roi(roi.offset_x));
		SetNodeParam_(kNodeNameRAWRoiOffsetY_, ceil_roi(roi.offset_y));
		
	}
}

void ArenaDeviceHandler::ResetDnnRoi() {
    StopStream();
    SetNodeParam_(kNodeNameRoiOffsetX_, 0);
    SetNodeParam_(kNodeNameRoiOffsetY_, 0);
    SetNodeParam_(kNodeNameRoiWidth_, kInitRoiWidth_);
    SetNodeParam_(kNodeNameRoiHeight_, kInitRoiHeight_);
    SetNodeParam_(kNodeNameRoiOffsetX_, kInitRoiOffestX_);
    SetNodeParam_(kNodeNameRoiOffsetY_, kInitRoiOffsetY_);
}

void ArenaDeviceHandler::ResetRawRoi() {
	StopStream();
	SetNodeParam_(kNodeNameRAWRoiOffsetX_, 0);
	SetNodeParam_(kNodeNameRAWRoiOffsetY_, 0);
	SetNodeParam_(kNodeNameRAWRoiWidth_, kInitRoiWidth_);
	SetNodeParam_(kNodeNameRAWRoiHeight_, kInitRoiHeight_);
	SetNodeParam_(kNodeNameRAWRoiOffsetX_, kInitRoiOffestX_);
	SetNodeParam_(kNodeNameRAWRoiOffsetY_, kInitRoiOffsetY_);
}

ROI  ArenaDeviceHandler::GetDnnRoi() const {
    return current_roi_;
}

std::string ArenaDeviceHandler::GetBarcode() {
	return barcode_;
}

void ArenaDeviceHandler::SetBarcode(std::string& barcode) {
	barcode_ = barcode;
}

void ArenaDeviceHandler::SetDetectionThreshold(const double threshold) {
	detection_threshold_ = threshold;
}


int ArenaDeviceHandler::GetPointerRoi() {
    return pointer_roi_;
}

int& ArenaDeviceHandler::GetPointerRoiAddr() {
    return pointer_roi_;
}

void ArenaDeviceHandler::SetPointerRoi(const int roi_id) {
    pointer_roi_ = roi_id;
}

// Function to extract image data within the bounding box
std::vector<uint8_t> ArenaDeviceHandler::ExtractBoundingBoxData(Arena::IImage* pImage, const ArenaExample::ObjectDetectionUtils::rect_uint32& rect)
{
    std::vector<uint8_t> buffer;
    int width = pImage->GetWidth();
    int height = pImage->GetHeight();
    int bytesPerPixel = pImage->GetBitsPerPixel() / 8;
    const uint8_t* imageData = static_cast<const uint8_t*>(pImage->GetData());

    for (uint32_t y = rect.top; y < rect.bottom; ++y)
    {
        for (uint32_t x = rect.left; x < rect.right; ++x)
        {
            int index = (y * width + x) * bytesPerPixel;
            for (int b = 0; b < bytesPerPixel; ++b)
            {
                buffer.push_back(imageData[index + b]);
            }
        }
    }

    return buffer;
}


void ArenaDeviceHandler::Process(cv::Mat& original, cv::Mat& raw_cropped, cv::Mat& input_tensor, cv::Mat& detections) {
    Arena::IImage* pImage;
    ArenaExample::BrainBuilderDetectorUtils outputUtil(&util_);
    try
    {
        pImage = pDevice_->GetImage(kTimeOut_);
    }
    catch (GenICam::TimeoutException)
    {
        if (pDevice_->IsConnected() != false) {
            printf("pDevice_->IsConnected() != false\n");
            return;
        }
        else
        {
            printf("The camera was disconnected\n");
            return;
        }
    }
    if (pImage != NULL)
    {
        if (pImage->IsIncomplete() == false)
        {
            Arena::IChunkData* pChunkData = pImage->AsChunkData();
            if (pChunkData != NULL)
            {
                if (pChunkData->IsIncomplete() == false)
                {
                    if (op_mode_ == 0 || op_mode_ == 2) {
                        int width_12M = (int)pImage->GetWidth();
                        int height_12M = (int)pImage->GetHeight();

                        Arena::IImage* pImage_12M = Arena::ImageFactory::Convert(pImage, BGR8);
                        cv::Mat image_12m = cv::Mat((int)pImage_12M->GetHeight(), (int)pImage_12M->GetWidth(), CV_8UC3, (void*)pImage_12M->GetData());
                        image_12m.copyTo(original);
                        Arena::ImageFactory::Destroy(pImage_12M);
                    }
                  
					ROI set_roi = GetDnnRoi();

                    if (op_mode_ < 2) // Get input tensor and inference results
                    {
                        if (util_.ProcessChunkData(pChunkData))
                        {
                            int width_12M_crop = (int)pImage->GetWidth();
                            int height_12M_crop = (int)pImage->GetHeight();

                            Arena::IImage* pImage_12M_crop = Arena::ImageFactory::Convert(pImage, BGR8);
                            cv::Mat image_12m_crop = cv::Mat((int)pImage_12M_crop->GetHeight(), (int)pImage_12M_crop->GetWidth(), CV_8UC3, (void*)pImage_12M_crop->GetData());
                            cv::Mat detection_12m_copy = image_12m_crop.clone();

                            input_tensor = cv::Mat((int)util_.GetInputImageHeight(), (int)util_.GetInputImageWidth(), CV_8UC3, (void*)util_.GetInputImagePtr());
                            cv::Mat detection_copy = input_tensor.clone();

                            //std::cout << util_->GetOutputTensorSize(0);
                            if (outputUtil.ProcessOutputTensor())
                            {
                                outputUtil.DumpOutputTensor();

                                int num = outputUtil.GetObjectNum(detection_threshold_);
                                for (int i = 0; i < num; i++)
                                {
                                    ArenaExample::ObjectDetectionUtils::object_info info;
                                    ArenaExample::ObjectDetectionUtils::rect_uint32 rect;
                                    outputUtil.GetObjectInfo(i, &info);
                                    rect = outputUtil.ToInputImageRect(info.location);
                                    std::string label = util_.GetLabelStr(info.index);

                                    if (label.find("barcode") != std::string::npos)
                                    {
                                        cv::rectangle(detection_copy, cv::Rect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top), cv::Scalar(0, 0, 255), 2);
                                        cv::putText(detection_copy, label, cv::Point(rect.left, rect.top - 8), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);

                                        auto rect_12m = outputUtil.ToRect_uint32(info.location, width_12M_crop, height_12M_crop);

										//Make the rect_12m a bit bigger to make sure the barcode is fully included
										rect_12m.left = std::max(0, (int)rect_12m.left - 10);
										rect_12m.top = std::max(0, (int)rect_12m.top - 10);
										rect_12m.right = std::min(width_12M_crop, (int)rect_12m.right + 10);
										rect_12m.bottom = std::min(height_12M_crop, (int)rect_12m.bottom + 10);

                                        bool canDecode = false;

                                        std::vector<uint8_t> buffer = ExtractBoundingBoxData(pImage_12M_crop, rect_12m);

                                        // Save buffer as JPG for debugging
                                        cv::Mat buffer_image(rect_12m.bottom - rect_12m.top, rect_12m.right - rect_12m.left, CV_8UC3);
                                        std::memcpy(buffer_image.data, buffer.data(), buffer.size());
                                        //cv::imwrite("buffer_image.jpg", buffer_image);

                                        std::string result = DecodeWithRotation(buffer_image);
                                        std::cout << "Barcode detected: " << result << "\n";

                                        if (!result.empty())
                                        {
                                            canDecode = true;
                                            SetBarcode(result);
                                            cv::putText(detection_12m_copy, result, cv::Point(rect_12m.left, rect_12m.bottom + 60), cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(255, 0, 0), 2, cv::LINE_AA);
                                        }

                                        if (canDecode) {
                                            cv::rectangle(detection_12m_copy, cv::Rect(rect_12m.left, rect_12m.top, rect_12m.right - rect_12m.left, rect_12m.bottom - rect_12m.top), cv::Scalar(0, 255, 0), 8);
                                        }
                                        else {
                                            cv::rectangle(detection_12m_copy, cv::Rect(rect_12m.left, rect_12m.top, rect_12m.right - rect_12m.left, rect_12m.bottom - rect_12m.top), cv::Scalar(0, 0, 255), 8);
                                        }

                                        cv::putText(detection_12m_copy, label, cv::Point(rect_12m.left, rect_12m.top - 15), cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0, 0, 0), 2, cv::LINE_AA);

                                    }
                                }

                                detections = detection_copy;

                            }
                            else
                            {
                                printf("outputUtil_->ProcessOutputTensor() returned false\nNetwork mismatch?\n");
                            }

                            detection_12m_copy.copyTo(raw_cropped);
                            Arena::ImageFactory::Destroy(pImage_12M_crop);
                        }

                    }

                }
            }
            else
            {
                printf("no chunk data\n");
            }
        }
    }
    pDevice_->RequeueBuffer(pImage);
}