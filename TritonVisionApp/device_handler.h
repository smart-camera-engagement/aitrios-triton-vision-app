// Copyright © 2024 Sony Semiconductor Solutions Corporation. All rights reserved.
// Author: Shogo.Takanashi@sony.com

/**
* @file device_handler.h
* @brief Class for device control
* @author Shogo Takanashi
* @date 2024/11
*/

#ifndef DEVICE_HANDLER_H_
#define DEVICE_HANDLER_H_

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>

#include "Arena/ArenaApi.h"
#include "BrainBuilderDetectorUtils.h"
#include "ean13_reader.h"
#include "./common.h"
#include <string>

class ArenaDeviceHandler {
public:
    ArenaDeviceHandler();
    ~ArenaDeviceHandler();
    //void SetOperationMode(const int op_mode);
    int GetOperationMode();
    bool GetStreamStatus();
    ROI current_roi_;
    void Process(cv::Mat& image_12m, cv::Mat& raw_cropped, cv::Mat& input_tensor, cv::Mat& detections);
    std::vector<uint8_t> ExtractBoundingBoxData(Arena::IImage* pImage, const ArenaExample::ObjectDetectionUtils::rect_uint32& rect);
    void StartStream(const int op_mode);
    void StopStream();
    void SetDnnRoi(const ROI& roi);
    void SetRawRoi(const ROI& roi);
    void ResetDnnRoi();
    void ResetRawRoi();
	void SetDetectionThreshold(const double threshold);
    int GetPointerRoi();
    void SetPointerRoi(const int roi_id);
    std::string GetBarcode();
    void SetBarcode(std::string& barcode);
    int& GetPointerRoiAddr();
    ROI GetDnnRoi() const;

private:
    void SetNodeParam_(const GENICAM_NAMESPACE::gcstring& node_name, const int node_value);
    Arena::ISystem* pSystem_;
    Arena::IDevice* pDevice_;
    ArenaExample::IMX501Utils util_;
    GenApi::INodeMap* pNodeMap;
    GenApi::CIntegerPtr pNode;
    int op_mode_ = 1; // 0-> get all, 1-> get inference results only
    bool is_stream_ = false;
    double detection_threshold_ = 0.6f;
    int pointer_roi_ = 0;

    std::string barcode_ = "0000000000000";

    const GENICAM_NAMESPACE::gcstring& kNodeNameRoiOffsetX_ = "DeepNeuralNetworkISPOffsetX";
    const GENICAM_NAMESPACE::gcstring& kNodeNameRoiOffsetY_ = "DeepNeuralNetworkISPOffsetY";
    const GENICAM_NAMESPACE::gcstring& kNodeNameRoiWidth_ = "DeepNeuralNetworkISPWidth";
    const GENICAM_NAMESPACE::gcstring& kNodeNameRoiHeight_ = "DeepNeuralNetworkISPHeight";

    const GENICAM_NAMESPACE::gcstring& kNodeNameRAWRoiOffsetX_ = "OffsetX";
    const GENICAM_NAMESPACE::gcstring& kNodeNameRAWRoiOffsetY_ = "OffsetY";
    const GENICAM_NAMESPACE::gcstring& kNodeNameRAWRoiWidth_ = "Width";
    const GENICAM_NAMESPACE::gcstring& kNodeNameRAWRoiHeight_ = "Height";

    const int kInitRoiOffestX_ = 0;
    const int kInitRoiOffsetY_ = 0;
    const int kInitRoiWidth_ = kSensorWidth;
    const int kInitRoiHeight_ = kSensorHeight;

    const int kTimeOut_ = 2000;


};

#endif 

