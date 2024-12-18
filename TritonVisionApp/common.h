// Copyright © 2024 Sony Semiconductor Solutions Corporation. All rights reserved.
// Author: Shogo.Takanashi@sony.com

/**
* @file common.h
* @brief defines common used
* @author Shogo Takanashi
* @date 2024/11
*/

#ifndef COMMON_H_
#define COMMON_H_

// Basic Parameters
const int kSensorWidth = 4052;
const int kSensorHeight = 3036;

const int kResizedWidth1280 = 1280;
const int kResizedHeight960 = 960;

const int kResizedWidth960 = 960;
const int kResizedHeight720 = 720;

const int kResizedWidth640 = 640;
const int kResizedHeight480 = 480;

const int kInputtensorWidth = 256;
const int kInputtensorHeight = 256;
const int kNumOfRoi = 1;

// Status
static const char* kStreamStatus[] = {
                "On Streaming RAW Image + Inference",
                "On Streaming Inference",
                "On Streaming RAW Image",
                "Stopped"
};

// [Style] Window title
static const char* kWindowTitle = "Barcode Detector for Triton Smart";

// [Style] Font file
static const char* kFontFile = "./resource/SSTJpPro-Regular.otf";
const float kFontSize = 20.0f;

// [Style] Corner Rounding
const float kStyleRounding0 = 0.0f;
const float kStyleRounding8 = 8.0f;
const float kStyleRounding12 = 12.0f;

// [Style] Spacing
const float kStyleSpacing8 = 8.0f;
const float kStyleSpacing12 = 12.0f;

// App Window Size
const int kWindow_Width = 1920;
const int kWindow_Height = 1080;

// Menu Widget Location and Size
const int kWidget_Menu_OffsetX = 0;
const int kWidget_Menu_OffsetY = 0;
//const int kWidget_Menu_Width = kWindow_Width;// To fit on window width
const int kWidget_Menu_Height = 40;

// Tab Widget Location
const int kWidget_Tab_OffsetX = 0;
const int kWidget_Tab_OffsetY = kWidget_Menu_OffsetY + kWidget_Menu_Height;


// Canvas Widget Location and Size
const int kWidget_Canvas_OffsetX = 0;
const int kWidget_Canvas_OffsetY = kWidget_Tab_OffsetY + 45;
//const int kWidget_Canvas_Width = kResizedWidth;
//const int kWidget_Canvas_Height = kResizedHeight;

// Icon file
static const char* kWindowIconFile = "./resource/icon.png";

// Config file
static const char* kConfigFile = "roi.json";

// Export normal image path
static const char* kDataImagePath = ".\\export-barcode\\";

struct  ROI {
    int id;
    int offset_x;
    int offset_y;
    int width;
    int height;
};

struct ConnectorPattern {
    std::string pattern_name;
    int num_of_roi;
    std::vector<ROI> roi_list;
};

struct Data {
    float detection_threshold = 0.6f;
    int num_of_pattern = 1;
    std::vector<ConnectorPattern> pattern_list;
};

#endif