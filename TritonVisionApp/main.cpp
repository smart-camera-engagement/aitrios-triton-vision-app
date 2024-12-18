// Copyright © 2024 Sony Semiconductor Solutions Corporation. All rights reserved.

/**
* @file main.cpp
* @brief メイン関数
* @author Shogo Takanashi
* @date 2024/9
*/

#include<windows.h>
#include <direct.h>
#include <chrono>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include "./imgui.h"
#include "./imgui_impl_glfw.h"
#include "./imgui_impl_opengl3.h"

#include <opencv2/opencv.hpp>

#include "Arena/ArenaApi.h"
#include "BrainBuilderDetectorUtils.h"
#include "ImageWindow.hpp"

#include "./load_texture.h"
#include "./common.h"

#include "./json_utils.h"
#include "./device_handler.h"


void ApplyROI(std::vector<ROI>& target_roi, std::vector<ROI>& source_roi ) {

    target_roi.clear();
    for (int i = 0; i < kNumOfRoi; i++) {
        target_roi.push_back(source_roi[i]);
    }
}

bool is_on_canvas(const int x, const int y, const int w, const int h) {
    return (x >= 0) 
        && (x < w) 
        && (y >= 0) 
        && (y < h);
}

int get_existing_roi(const int x, const int y, const int width, const int height,const std::vector<ROI>& roi) {

    int pointed_roi = -1;
    for (int i = 0; i < kNumOfRoi; i++) {
        int rx = (int)(roi[i].offset_x * width / kSensorWidth);
        int ry = (int)(roi[i].offset_y * height / kSensorHeight);
        int rw = (int)(roi[i].width * width / kSensorWidth);
        int rh = (int)(roi[i].height * height / kSensorHeight);
        if (
            (x >= rx)
            && (x < rx + rw)
            && (y >= ry)
            && (y < ry + rh)
            ) 
        {
            pointed_roi = i;
        }
    }

    return pointed_roi;
}

void StyleSpacing(const float spacing) {
    ImGuiStyle& style = ImGui::GetStyle();
    style.FramePadding.x = spacing;
    style.FramePadding.y = spacing;
    style.ItemSpacing.x = spacing;
    style.ItemSpacing.y = spacing;
    style.ItemInnerSpacing.x = spacing;
    style.ItemInnerSpacing.y = spacing;
}


void StyleCornerRounding(const float rounding) {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = rounding;
    style.FrameRounding = rounding;
    style.GrabRounding = rounding;
    style.ScrollbarRounding = rounding;
    style.PopupRounding = rounding;
}

std::string GetDatetimeStr() {
    time_t t = time(nullptr);
    const tm* localTime = localtime(&t);
    std::stringstream s;
    s << "20" << localTime->tm_year - 100 << "-";
    s << std::setw(2) << std::setfill('0') << localTime->tm_mon + 1 << "-";
    s << std::setw(2) << std::setfill('0') << localTime->tm_mday << "-";
    s << std::setw(2) << std::setfill('0') << localTime->tm_hour << "-";
    s << std::setw(2) << std::setfill('0') << localTime->tm_min << "-";
    s << std::setw(2) << std::setfill('0') << localTime->tm_sec;
    return s.str();
}

static int frameCount = 0;
double fps = 0.0;

// Function to calculate and display FPS
double ShowFPS() {
    static auto lastTime = std::chrono::high_resolution_clock::now();
    frameCount++;
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = currentTime - lastTime;
    if (elapsed.count() >= 1.0) {
        fps = frameCount / elapsed.count();
        frameCount = 0;
        lastTime = currentTime;
    }

    return fps;
}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
) {

    // [GLFW] Initialization
    if (!glfwInit()) {
        return -1;
    }

    // [GLFW] Setup context version of OpenGL
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // [GLFW] Create main window
    GLFWwindow* window = glfwCreateWindow(kWindow_Width, kWindow_Height, kWindowTitle, NULL, NULL);

    if (!window) {
        glfwTerminate();
        return -1;
    }

    // [GLFW] Set Icon
    SetWindowIcon(window);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    gl3wInit();

    // [ImGui] Initialization
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // [ImGui] Font Settings
    io.Fonts->AddFontFromFileTTF(kFontFile, kFontSize, NULL);

    // [ImGui] Style Settings
    ImGui::StyleColorsClassic();    
    StyleSpacing(kStyleSpacing8);

    // [GLFW][ImGui] Initialization for combining OpenGl with ImGui
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Set inital image view size
    int main_window_width = kResizedWidth960;
    int main_window_height = kResizedHeight720;

    // [OpenCV] RAW image matrix
    cv::Mat original(kSensorHeight, kSensorWidth, CV_8UC3, cv::Scalar(0, 0, 0));

    cv::Mat raw_cropped(kSensorHeight, kSensorWidth, CV_8UC3, cv::Scalar(0, 0, 0));

    // [OpenCV] Input tensor matrix
    cv::Mat input_tensor[kNumOfRoi] = {
        cv::Mat(kInputtensorHeight, kInputtensorWidth, CV_8UC3, cv::Scalar(0, 0, 0)),
    };

    // [OpenCV] detections
    cv::Mat detections[kNumOfRoi] = {
        cv::Mat(kInputtensorHeight, kInputtensorWidth, CV_8UC3, cv::Scalar(0, 0, 0)),
    };

    // Initial score for ROIs
    std::vector<double> score = { 1.0f};

    // ROI coordinate on GUI, reserved coordinate to be applied when clicked "Save" on GUI 
    std::vector<ROI> roi;

    // Configuration parameters on json file, including saved ROI coordinate
    Data result;
    ReadConfigJson(result);
    
    // Create a device instance
    ArenaDeviceHandler triton;

    // Selected Connector pattern
    int pattern_id = 0;

    // Apply initial ROI
    ApplyROI(roi, result.pattern_list[pattern_id].roi_list);

    // state for demo mode
    int demo_state = -1;

    // state for window mode
    int window_mode = 1;


    // setup canvas parameters
    int s_pos_x = 0;
    int s_pos_y = 0;
    int e_pos_x = 0;
    int e_pos_y = 0;
    int in_delta_w = 0;
    int in_delta_h = 0;

    bool is_show_inputtensor = true;
    bool is_show_bbox = true;
    bool is_show_position_control = false;
    bool is_show_inference_control = true;

    while (!glfwWindowShouldClose(window)) {

        // [GL] Process for getting events
        glfwPollEvents();

        // [ImGui] Start for current frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // [GL] Get window size
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        //cv::Mat original(kSensorHeight, kSensorWidth, CV_8UC3, cv::Scalar(0, 0, 0));
        cv::Mat resized(main_window_height, main_window_width, CV_8UC3, cv::Scalar(0, 0, 0));

        cv::Mat resized_crop(main_window_height, main_window_width, CV_8UC3, cv::Scalar(0, 0, 0));

        // Change state for demo mode
        if (demo_state < 0) { // Demo is not started
            int p_roi = triton.GetPointerRoi();
            if (triton.GetStreamStatus()) {
                triton.Process(original, raw_cropped, input_tensor[p_roi], detections[p_roi]);
                cv::resize(original, resized, cv::Size(main_window_width, main_window_height));
                cv::resize(raw_cropped, resized_crop, cv::Size(main_window_width, main_window_height));
            }
        }
        else { // Demo started
            triton.SetDnnRoi(roi[demo_state]); // Set new ROI
            triton.SetPointerRoi(demo_state); // Set operation pointer
            triton.StopStream();
            triton.StartStream(1);
            triton.Process(original, raw_cropped, input_tensor[demo_state], detections[demo_state]); // Process
            cv::resize(original, resized, cv::Size(main_window_width, main_window_height));
            if (demo_state >= kNumOfRoi - 1) {
                demo_state = -1;
            }
            else {
                demo_state += 1;
            }
        }

        // Draw Menu Bar
        StyleCornerRounding(kStyleRounding0);
        ImGui::SetNextWindowPos(ImVec2(kWidget_Menu_OffsetX, kWidget_Menu_OffsetY));
        ImGui::SetNextWindowSize(ImVec2((float)width, kWidget_Menu_Height));
        ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
        flags |= ImGuiWindowFlags_NoTitleBar;
        flags |= ImGuiWindowFlags_NoResize;
        flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
        ImGui::Begin("menu_window", nullptr, flags);

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Stream")) {
                if (triton.GetStreamStatus()) {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
                    ImGui::MenuItem(kStreamStatus[triton.GetOperationMode()], NULL, false, false);
                    ImGui::PopStyleColor();
                    if (ImGui::MenuItem("Stop")) {
                        triton.ResetRawRoi();
                        triton.ResetDnnRoi();
                    }
                    ImGui::EndMenu();
                }
                else {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
                    ImGui::MenuItem(kStreamStatus[3], NULL, false, false);
                    ImGui::PopStyleColor();
                    if (ImGui::MenuItem("Start")) {
                        triton.StartStream(2);
                    }
                    ImGui::EndMenu();
                }
            }
            if (ImGui::BeginMenu("Settings")) {
                if (ImGui::BeginMenu("Window Size")) {
                    if (ImGui::MenuItem("Small", NULL, main_window_width == kResizedWidth640)) {
                        main_window_width = kResizedWidth640;
                        main_window_height = kResizedHeight480;
                    }
                    if (ImGui::MenuItem("Medium", NULL, main_window_width == kResizedWidth960)) {
                        main_window_width = kResizedWidth960;
                        main_window_height = kResizedHeight720;
                    }
                    if (ImGui::MenuItem("Large", NULL, main_window_width == kResizedWidth1280)) {
                        main_window_width = kResizedWidth1280;
                        main_window_height = kResizedHeight960;
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::End();

        GLuint main_texture;
        GLuint crop_texture;
        GLuint inputtensor_texture[kNumOfRoi];
        GLuint detection_texture[kNumOfRoi];

        ImGui::SetNextWindowPos(ImVec2(kWidget_Tab_OffsetX, kWidget_Tab_OffsetY));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width), static_cast<float>(height - kWidget_Tab_OffsetY)));
        ImGui::Begin("tab_window", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyScroll;
        if (ImGui::BeginTabBar("tab_bar", tab_bar_flags)) {
            
            if (ImGui::BeginTabItem("Detector")) {
                window_mode = 1;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Setup")) {
                window_mode = 0;
                if (triton.GetStreamStatus() && (triton.GetOperationMode() == 1)) {
                    triton.ResetDnnRoi();
                    triton.ResetRawRoi();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
        StyleCornerRounding(kStyleRounding8);

        if (window_mode == 1)
        {
            // Draw Control Panel
            ImGui::Begin("Control Panel", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::SeparatorText("Image Stream");
            //ImGui::Text("Status:  ");
            if (triton.GetStreamStatus()) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
                ImGui::Text(kStreamStatus[triton.GetOperationMode()]);
                ImGui::PopStyleColor();
            }
            else {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
                ImGui::Text(kStreamStatus[3]);
                ImGui::PopStyleColor();

            }
            if (ImGui::Button("Start")) {
                triton.StartStream(2);
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop")) {
                triton.ResetRawRoi();
            }
            ImGui::Text("Threshold");
            ImGui::PushID(1);
            ImGui::SliderFloat(" ", &result.detection_threshold, 0.0f, 1.0f, "%.2f");
            ImGui::PopID();
            ImGui::SameLine();
            ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
            if (ImGui::ArrowButton("##Left", ImGuiDir_Left)) {
                result.detection_threshold -= 0.01f;
                result.detection_threshold = std::max(result.detection_threshold, 0.0f);
            }
            ImGui::SameLine();
            if (ImGui::ArrowButton("##Right", ImGuiDir_Right)) {
                result.detection_threshold += 0.01f;
                result.detection_threshold = std::min(result.detection_threshold, 1.0f);
            }
            ImGui::PopItemFlag();
            if (ImGui::Button("Run")) {
                if (demo_state < 0) {
                    demo_state = 0;
                    triton.ResetDnnRoi();
					triton.SetDetectionThreshold(result.detection_threshold);
                }
            }
            ImGui::Spacing();
            ImGui::SeparatorText("Advanced View Settings");
            ImGui::Spacing();
            ImGui::Checkbox("Show Input tensor", &is_show_inputtensor);
            ImGui::Checkbox("Show Bounding Box", &is_show_bbox);
            ImGui::Checkbox("Show Position Control", &is_show_position_control);
            ImGui::Checkbox("Show Inference Control", &is_show_inference_control);
            ImGui::End();

            /*
            // Draw RAW Image
            ImGui::Begin("Image View (Setup Mode)", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text(result.pattern_list[pattern_id].pattern_name.c_str());
            cv::Mat main = resized.clone();
            
            for (int i = 0; i < kNumOfRoi; i++) {
                int x = (int)(roi[i].offset_x * main_window_width / kSensorWidth);
                int y = (int)(roi[i].offset_y * main_window_height / kSensorHeight);
                int w = (int)(roi[i].width * main_window_width / kSensorWidth);
                int h = (int)(roi[i].height * main_window_height / kSensorHeight);
                cv::Scalar color,tcolor;
                int thickness = 2;
                color = cv::Scalar(0, 0xff, 0);
                tcolor = cv::Scalar(0xff, 0xff, 0xff, 0);
                cv::rectangle(main, cv::Rect(x, y, w, h), color, thickness);

                char buf[64];
                sprintf(buf, "ROI", i);
                cv::Size text_size = cv::getTextSize(buf, cv::FONT_HERSHEY_DUPLEX, 1.0, 1, nullptr);
                cv::rectangle(main, cv::Rect(x, y - (text_size.height + 2* thickness), text_size.width, text_size.height + 2 * thickness), color, thickness);
                cv::rectangle(main, cv::Rect(x, y - (text_size.height + 2 * thickness), text_size.width, text_size.height + 2 * thickness), color, -1);
                cv::putText(main, buf, cv::Point(x, y - thickness), cv::FONT_HERSHEY_DUPLEX, 1.0, tcolor, 1);
            }
            
            LoadTextureFromCvMat(&main, &main_texture);
            ImVec2 demo_size = ImVec2(static_cast<float>(main.cols), static_cast<float>(main.rows));
            ImGui::Image(main_texture, demo_size);
            ImGui::End();
            */

            ImGui::Begin("Image View (Inference Mode)", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Text(result.pattern_list[pattern_id].pattern_name.c_str());
            cv::Mat main2 = resized_crop.clone();

            LoadTextureFromCvMat(&main2, &main_texture);
            ImVec2 demo_size2 = ImVec2(static_cast<float>(main2.cols), static_cast<float>(main2.rows));
            ImGui::Image(main_texture, demo_size2);
            ImGui::End();

            // Draw the Barcode Information window below the image view
            ImGui::Separator();
            ImGui::Begin("Barcode Information", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::SeparatorText("Last Seen");
            ImGui::Spacing();
            ImGui::Text("Barcode: ");
            ImGui::SameLine();
            ImGui::Text(triton.GetBarcode().c_str());
            ImGui::End();

            double output_fps = ShowFPS();

			// Convert FPS to string
			char ShowFPS[32];
			sprintf(ShowFPS, "%.2f", output_fps);

            // Draw a window to show the fps below the Barcode Information window
            ImGui::Begin("Frame Rate Information");
            ImGui::Spacing();
            ImGui::Text("FPS: ");
            ImGui::SameLine();
            ImGui::Text(ShowFPS);
            ImGui::End();

            // Draw Input tensor and Bounding Box of each ROIs
            for (int i = 0; i < kNumOfRoi; i++) {
                char label[64];
                sprintf(label, "ROI", i);
                ImGui::PushID(i);
                ImGui::SetNextWindowSizeConstraints(ImVec2(kInputtensorWidth + ImGui::GetStyle().FramePadding.x * 2.0f, 200), ImVec2(static_cast<float>(width), static_cast<float>(height)));
                ImGui::Begin(label, nullptr, ImGuiWindowFlags_AlwaysAutoResize);

                if (is_show_inputtensor || is_show_bbox) {
                    ImGui::Spacing();
                    ImGui::SeparatorText("Result Image");
                    ImGui::Spacing();
                }

                if (is_show_inputtensor) {
                    ImGui::BeginGroup();
                    ImGui::Text("Input tensor");
                    LoadTextureFromCvMat(&input_tensor[i], &inputtensor_texture[i]);
                    demo_size2 = ImVec2(static_cast<float>(input_tensor[i].cols), static_cast<float>(input_tensor[i].rows));
                    ImGui::Image(inputtensor_texture[i], demo_size2);
                    ImGui::EndGroup();
                }

                if (is_show_bbox) {
                    ImGui::BeginGroup();
                    ImGui::Text("Detections");
                    LoadTextureFromCvMat(&detections[i], &detection_texture[i]);
                    demo_size2 = ImVec2(static_cast<float>(detections[i].cols), static_cast<float>(detections[i].rows));
                    ImGui::Image(detection_texture[i], demo_size2);
                    ImGui::EndGroup();
                }

                if (is_show_inputtensor && is_show_bbox) {
                    ImGui::SameLine();
                }

                if (is_show_position_control) {
                    ImGui::Spacing();
                    ImGui::SeparatorText("Position Control");
                    ImGui::Spacing();

                    ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
                    ImGui::PushID(100);
                    if (ImGui::SliderInt(" ", &roi[i].offset_x, 0, kSensorWidth - 1, "Left = %d")) {
                        if (roi[i].offset_x + roi[i].width >= kSensorWidth) {
                            roi[i].offset_x = kSensorWidth - roi[i].width - 1;
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::ArrowButton("##Left", ImGuiDir_Left)) {
                        roi[i].offset_x = (roi[i].offset_x < 4) ? roi[i].offset_x : roi[i].offset_x - 4;
                    }
                    ImGui::SameLine();
                    if (ImGui::ArrowButton("##Right", ImGuiDir_Right)) {
                        roi[i].offset_x = (roi[i].offset_x >= kSensorWidth - 4) ? kSensorWidth - 1 : roi[i].offset_x + 4;
                        if (roi[i].offset_x + roi[i].width >= kSensorWidth) {
                            roi[i].offset_x = kSensorWidth - roi[i].width - 1;
                        }
                    }
                    ImGui::PopID();

                    ImGui::PushID(101);
                    if (ImGui::SliderInt(" ", &roi[i].offset_y, 0, kSensorHeight - 1, "Top = %d")) {
                        if (roi[i].offset_y + roi[i].height >= kSensorHeight) {
                            roi[i].offset_y = kSensorHeight - roi[i].height - 1;
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::ArrowButton("##Up", ImGuiDir_Up)) {
                        roi[i].offset_y = (roi[i].offset_y < 4) ? roi[i].offset_y : roi[i].offset_y - 4;
                    }
                    ImGui::SameLine();
                    if (ImGui::ArrowButton("##Down", ImGuiDir_Down)) {
                        roi[i].offset_y = (roi[i].offset_y >= kSensorHeight - 4) ? kSensorHeight - 1 : roi[i].offset_y + 4;
                        if (roi[i].offset_y + roi[i].height >= kSensorHeight) {
                            roi[i].offset_y = kSensorHeight - roi[i].height - 1;
                        }
                    }
                    ImGui::PopID();

                    ImGui::PushID(102);
                    if (ImGui::SliderInt(" ", &roi[i].width, 4, kSensorWidth - 1, "Width = %d")) {
                        if (roi[i].offset_x + roi[i].width >= kSensorWidth) {
                            roi[i].width = kSensorWidth - roi[i].offset_x - 1;
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::ArrowButton("##Left", ImGuiDir_Left)) {
                        roi[i].width = (roi[i].width < 8) ? 4 : roi[i].width - 4;
                    }
                    ImGui::SameLine();
                    if (ImGui::ArrowButton("##Right", ImGuiDir_Right)) {
                        roi[i].width = (roi[i].width >= kSensorWidth - 4) ? kSensorWidth - 1 : roi[i].width + 4;
                        if (roi[i].offset_x + roi[i].width >= kSensorWidth) {
                            roi[i].width = kSensorWidth - roi[i].offset_x - 1;
                        }
                    }
                    ImGui::PopID();

                    ImGui::PushID(103);
                    if (ImGui::SliderInt(" ", &roi[i].height, 4, kSensorHeight - 1, "Height = %d")) {
                        if (roi[i].offset_y + roi[i].height >= kSensorHeight) {
                            roi[i].height = kSensorHeight - roi[i].offset_y - 1;
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::ArrowButton("##Up", ImGuiDir_Up)) {
                        roi[i].height = (roi[i].height < 8) ? 4 : roi[i].height - 4;
                    }
                    ImGui::SameLine();
                    if (ImGui::ArrowButton("##Down", ImGuiDir_Down)) {
                        roi[i].height = (roi[i].height >= kSensorHeight - 4) ? kSensorHeight - 1 : roi[i].height + 4;
                        if (roi[i].offset_y + roi[i].height >= kSensorHeight) {
                            roi[i].height = kSensorHeight - roi[i].offset_y - 1;
                        }
                    }
                    ImGui::PopID();
                    ImGui::PopItemFlag();
                    if (ImGui::Button("Reset")) {
                        ApplyROI(roi, result.pattern_list[pattern_id].roi_list);
                    }
                }
                if(is_show_inference_control) {
                    ImGui::Spacing();
                    ImGui::SeparatorText("Inference Control");
                    ImGui::Spacing();
                    if (ImGui::Button("Start")) {
                        triton.SetRawRoi(roi[i]);
                        triton.SetDnnRoi(roi[i]);
                        triton.SetPointerRoi(i);
                        triton.StartStream(1);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Stop")) {
                        triton.ResetDnnRoi();
                        triton.ResetRawRoi();
                    }
                }
                ImGui::End();
                ImGui::PopID();
            }

            if (demo_state >= 0) ImGui::OpenPopup("Now Processing..");
            ImGui::SetNextWindowPos(ImVec2(static_cast<float>(width / 2) - 200.0f, static_cast<float>(height / 2) - 100.0f), ImGuiCond_Always);
            if (ImGui::BeginPopupModal("Now Processing..", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
                char buf[32];
                float v;
                if (demo_state == 0) {
                    v = 0.0f;
                    sprintf(buf, "%d/%d", demo_state, kNumOfRoi);
                }
                else if (demo_state < 0) {
                    v = 1.0f;
                    sprintf(buf, "%d/%d", kNumOfRoi, kNumOfRoi);
                }
                else {
                    v = (float)demo_state / (float)kNumOfRoi;
                    sprintf(buf, "%d/%d", demo_state, kNumOfRoi);
                }
                ImGui::ProgressBar(v, ImVec2(0.f, 0.f), buf);

                if (demo_state < 0) {
                    triton.StopStream();
                    triton.StartStream(2);
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

        }

        // Setup Mode
        else {
            ImVec4 text_color_gray = { 1.0f, 1.0f, 1.0f, 0.5f };
            
            ImGui::SetNextWindowPos(ImVec2(kWidget_Canvas_OffsetX, kWidget_Canvas_OffsetY));
            ImGui::SetNextWindowSize(ImVec2(main_window_width+ ImGui::GetStyle().FramePadding.x * 2.0f, static_cast<float>(height - kWidget_Canvas_OffsetY)));
            ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
            flags |= ImGuiWindowFlags_NoTitleBar;
            ImGui::Begin("Main", nullptr, flags);

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, text_color_gray);
            ImGui::SeparatorText("Draw Target Areas for Package Detector");
            ImGui::PopStyleColor();
            ImGui::Spacing();

            float window_width = ImGui::GetWindowSize().x;
            float reset_button_width = ImGui::CalcTextSize("Reset ").x + ImGui::GetStyle().FramePadding.x * 2.0f;
            float save_button_width = ImGui::CalcTextSize("Save ").x + ImGui::GetStyle().FramePadding.x * 2.0f;
            ImGui::SetCursorPosX(window_width - reset_button_width - save_button_width - ImGui::GetStyle().ItemSpacing.x * 2.0f);
            if (ImGui::Button("Reset")) {
                ApplyROI(roi, result.pattern_list[pattern_id].roi_list);
            }
            ImGui::SameLine();
            if (ImGui::Button("Save")) {
                for (int i = 0; i < kNumOfRoi; i++) {
                    result.pattern_list[pattern_id].roi_list[i].offset_x = roi[i].offset_x;
                    result.pattern_list[pattern_id].roi_list[i].offset_y = roi[i].offset_y;
                    result.pattern_list[pattern_id].roi_list[i].width = roi[i].width;
                    result.pattern_list[pattern_id].roi_list[i].height = roi[i].height;
                }
                WriteConfigJson(result);
            }

            ImVec2 image_pos = ImGui::GetCursorScreenPos();

            // Get Mouse Position
            int c_pos_x = static_cast<int>(io.MousePos.x - image_pos.x);
            int c_pos_y = static_cast<int>(io.MousePos.y - image_pos.y);

            // Get RoI
            int target_roi = triton.GetPointerRoi();

            // Roi interaction flag
            static bool exsisting_roi = false;
            static bool same_roi = false;

            // Get interacted idx of roi
            int ret = get_existing_roi(c_pos_x, c_pos_y, main_window_width, main_window_height, roi);

            // Store start pos when Mouse clicked
            if (io.MouseClicked[0] && is_on_canvas(c_pos_x, c_pos_y, main_window_width, main_window_height)) {

                // Clicked on existig ROI
                if (ret >= 0) {
                    exsisting_roi = true;
                    if (triton.GetPointerRoiAddr() == ret) { // Clicked on same ROI -> keep delta between roi offset and current position
                        in_delta_w = (c_pos_x * kSensorWidth / main_window_width) - roi[ret].offset_x;
                        in_delta_h = (c_pos_y * kSensorHeight / main_window_height) - roi[ret].offset_y;
                        same_roi = true;
                    }
                    else // Clicked on different ROI -> change focus
                    {
                        triton.GetPointerRoiAddr() = ret;
                    }

                }
                // Clicked on outside of Existig ROI
                else {
                    s_pos_x = c_pos_x;
                    s_pos_y = c_pos_y;
                }
            }
            // reset flags when Mouse released
            else if (io.MouseReleased[0]) {
                exsisting_roi = false;
                same_roi = false;
            }

            cv::Mat main = resized.clone();
            // Mouse down & clicked outside of ROI
            if (ImGui::IsMouseDown(0) && ImGui::IsWindowFocused() && is_on_canvas(c_pos_x, c_pos_y, main_window_width, main_window_height) && !exsisting_roi)
            {
                e_pos_x = c_pos_x;
                e_pos_y = c_pos_y;

                e_pos_x = std::max(std::min(e_pos_x, kSensorWidth - 1), 0);
                e_pos_y = std::max(std::min(e_pos_y, kSensorHeight - 1), 0);

                roi[target_roi].offset_x = std::min(s_pos_x, e_pos_x) * kSensorWidth / main_window_width;
                roi[target_roi].offset_y = std::min(s_pos_y, e_pos_y) * kSensorHeight / main_window_height;
                roi[target_roi].width = (abs(s_pos_x - e_pos_x)) * kSensorWidth / main_window_width;
                roi[target_roi].height = (abs(s_pos_y - e_pos_y)) * kSensorHeight / main_window_height;

            }
            // Mouse down & clicked inside of same ROI
            else if (ImGui::IsMouseDown(0) && ImGui::IsWindowFocused() && is_on_canvas(c_pos_x, c_pos_y, main_window_width, main_window_height) && same_roi) {
                roi[target_roi].offset_x = (c_pos_x * kSensorWidth / main_window_width) - in_delta_w;
                roi[target_roi].offset_y = (c_pos_y * kSensorHeight / main_window_height) - in_delta_h;

                roi[target_roi].offset_x = std::max(roi[target_roi].offset_x, 0);
                roi[target_roi].offset_y = std::max(roi[target_roi].offset_y, 0);
                if (roi[target_roi].offset_x + roi[target_roi].width >= kSensorWidth) {
                    roi[target_roi].offset_x = kSensorWidth - roi[target_roi].width;
                }
                if (roi[target_roi].offset_y + roi[target_roi].height >= kSensorHeight) {
                    roi[target_roi].offset_y = kSensorHeight - roi[target_roi].height;
                }

            }
            for (int i = 0; i < kNumOfRoi; i++) {
                int x = (int)(roi[i].offset_x * main_window_width / kSensorWidth);
                int y = (int)(roi[i].offset_y * main_window_height / kSensorHeight);
                int w = (int)(roi[i].width * main_window_width / kSensorWidth);
                int h = (int)(roi[i].height * main_window_height / kSensorHeight);

                cv::Scalar color;
                int thickness = 2;
                if (i == target_roi) {
                    color = cv::Scalar(0x00, 0xff, 0x00, 0);
                }
                else {
                    color = cv::Scalar(0xff, 0xff, 0xff, 0);
                }
                if (w <= 0 || h <= 0) {
                    cv::circle(main, cv::Point(x, y), 5, color, -1);
                }
                else {
                    cv::rectangle(main, cv::Rect(x, y, w, h), color, thickness);
                }
                char buf[64];
                sprintf(buf, "ROI", i);
                cv::Size text_size = cv::getTextSize(buf, cv::FONT_HERSHEY_DUPLEX, 1.0, 1, nullptr);
                cv::rectangle(main, cv::Rect(x, y - (text_size.height + 2 * thickness), text_size.width, text_size.height + 2 * thickness), color, thickness);
                cv::rectangle(main, cv::Rect(x, y - (text_size.height + 2 * thickness), text_size.width, text_size.height + 2 * thickness), color, -1);
                cv::putText(main, buf, cv::Point(x, y - thickness), cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar(0, 0, 0, 0), 1);

            }

            LoadTextureFromCvMat(&main, &main_texture);
            ImVec2 demo_size = ImVec2(static_cast<float>(main.cols), static_cast<float>(main.rows));
            ImGui::Image(main_texture, demo_size);
            
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, text_color_gray);
            ImGui::SeparatorText("Adjust for the Position of the Target Area");
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
            ImGui::PushID(100);
            if (ImGui::SliderInt(" ", &roi[target_roi].offset_x, 0, kSensorWidth - 1, "Left = %d")) {
                if (roi[target_roi].offset_x + roi[target_roi].width >= kSensorWidth) {
                    roi[target_roi].offset_x = kSensorWidth - roi[target_roi].width - 1;
                }
            }
            ImGui::SameLine();
            if (ImGui::ArrowButton("##Left", ImGuiDir_Left)) {
                roi[target_roi].offset_x = (roi[target_roi].offset_x < 4) ? roi[target_roi].offset_x : roi[target_roi].offset_x - 4;
            }
            ImGui::SameLine();
            if (ImGui::ArrowButton("##Right", ImGuiDir_Right)) {
                roi[target_roi].offset_x = (roi[target_roi].offset_x >= kSensorWidth - 4) ? kSensorWidth - 1 : roi[target_roi].offset_x + 4;
                if (roi[target_roi].offset_x + roi[target_roi].width >= kSensorWidth) {
                    roi[target_roi].offset_x = kSensorWidth - roi[target_roi].width - 1;
                }
            }
            ImGui::PopID();

            ImGui::PushID(101);
            if (ImGui::SliderInt(" ", &roi[target_roi].offset_y, 0, kSensorHeight - 1, "Top = %d")) {
                if (roi[target_roi].offset_y + roi[target_roi].height >= kSensorHeight) {
                    roi[target_roi].offset_y = kSensorHeight - roi[target_roi].height - 1;
                }
            }
            ImGui::SameLine();
            if (ImGui::ArrowButton("##Up", ImGuiDir_Up)) {
                roi[target_roi].offset_y = (roi[target_roi].offset_y < 4) ? roi[target_roi].offset_y : roi[target_roi].offset_y - 4;
            }
            ImGui::SameLine();
            if (ImGui::ArrowButton("##Down", ImGuiDir_Down)) {
                roi[target_roi].offset_y = (roi[target_roi].offset_y >= kSensorHeight - 4) ? kSensorHeight - 1 : roi[target_roi].offset_y + 4;
                if (roi[target_roi].offset_y + roi[target_roi].height >= kSensorHeight) {
                    roi[target_roi].offset_y = kSensorHeight - roi[target_roi].height - 1;
                }
            }
            ImGui::PopID();

            ImGui::PushID(102);
            if (ImGui::SliderInt(" ", &roi[target_roi].width, 4, kSensorWidth - 1, "Width = %d")) {
                if (roi[target_roi].offset_x + roi[target_roi].width >= kSensorWidth) {
                    roi[target_roi].width = kSensorWidth - roi[target_roi].offset_x - 1;
                }
            }
            ImGui::SameLine();
            if (ImGui::ArrowButton("##Left", ImGuiDir_Left)) {
                roi[target_roi].width = (roi[target_roi].width < 8) ? 4 : roi[target_roi].width - 4;
            }
            ImGui::SameLine();
            if (ImGui::ArrowButton("##Right", ImGuiDir_Right)) {
                roi[target_roi].width = (roi[target_roi].width >= kSensorWidth - 4) ? kSensorWidth - 1 : roi[target_roi].width + 4;
                if (roi[target_roi].offset_x + roi[target_roi].width >= kSensorWidth) {
                    roi[target_roi].width = kSensorWidth - roi[target_roi].offset_x - 1;
                }
            }
            ImGui::PopID();

            ImGui::PushID(103);
            if (ImGui::SliderInt(" ", &roi[target_roi].height, 4, kSensorHeight - 1, "Height = %d")) {
                if (roi[target_roi].offset_y + roi[target_roi].height >= kSensorHeight) {
                    roi[target_roi].height = kSensorHeight - roi[target_roi].offset_y - 1;
                }
            }
            ImGui::SameLine();
            if (ImGui::ArrowButton("##Up", ImGuiDir_Up)) {
                roi[target_roi].height = (roi[target_roi].height < 8) ? 4 : roi[target_roi].height - 4;
            }
            ImGui::SameLine();
            if (ImGui::ArrowButton("##Down", ImGuiDir_Down)) {
                roi[target_roi].height = (roi[target_roi].height >= kSensorHeight - 4) ? kSensorHeight - 1 : roi[target_roi].height + 4;
                if (roi[target_roi].offset_y + roi[target_roi].height >= kSensorHeight) {
                    roi[target_roi].height = kSensorHeight - roi[target_roi].offset_y - 1;
                }
            }
            ImGui::PopID();
            ImGui::PopItemFlag();

            ImGui::End();

            ImGui::SetNextWindowPos(ImVec2(main_window_width+ ImGui::GetStyle().FramePadding.x * 2.0f, kWidget_Canvas_OffsetY));
            ImGui::SetNextWindowSize(ImVec2(width - (main_window_width+ ImGui::GetStyle().FramePadding.x * 2.0f), static_cast<float>(height- kWidget_Canvas_OffsetY)));
            flags |= ImGuiWindowFlags_HorizontalScrollbar;
            ImGui::Begin("Selection", nullptr, flags);

            ImGui::Spacing();
            cv::Mat cropped;
            if ((roi[target_roi].width > 0) && (roi[target_roi].height > 0) && (roi[target_roi].offset_x + roi[target_roi].width < kSensorWidth) && (roi[target_roi].offset_y + roi[target_roi].height < kSensorHeight)) {
                cropped = original(cv::Rect(roi[target_roi].offset_x, roi[target_roi].offset_y, roi[target_roi].width - (roi[target_roi].width % 4), roi[target_roi].height - (roi[target_roi].height % 4))).clone();
                LoadTextureFromCvMat(&cropped, &crop_texture);
                demo_size = ImVec2(static_cast<float>(cropped.cols), static_cast<float>(cropped.rows));
            }
            static std::string time;
            static int path_selection;
            ImGui::SameLine();

            if (triton.GetStreamStatus()) {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, text_color_gray);
                ImGui::SeparatorText("Capture Cropped image for AI training");
                ImGui::PopStyleColor();
                ImGui::Spacing();

                window_width = std::min(ImGui::GetWindowSize().x - 10.0f, demo_size.x);
                float date_button_width = ImGui::CalcTextSize(("Last Captured: " + time).c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0f;
                float capture_button_width = ImGui::CalcTextSize("Capture").x + ImGui::GetStyle().FramePadding.x * 2.0f;
                float cursol_pos_x = window_width - capture_button_width - date_button_width - ImGui::GetStyle().ItemSpacing.x * 2.0f;
                ImGui::SetCursorPosX(std::max(cursol_pos_x, 0.0f));

                ImGui::Text(("Last Captured: " + time + "  ").c_str());
                ImGui::SameLine();
                if (ImGui::Button("Capture")) {
                    int ret = _mkdir(kDataImagePath);
                    time = GetDatetimeStr();
                    cv::imwrite(std::string(kDataImagePath) + time + ".png", cropped);
                }
                ImGui::Image(crop_texture, demo_size);
            }
            ImGui::End();

        }

        // [GL][ImGui] rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glClearColor(0.8f, 0.8f, 0.8f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, display_w, display_h);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // [GL] Release memory allocation for OpenGL
        glDeleteTextures(1, &main_texture);
        glDeleteTextures(1, &crop_texture);
        for (int i = 0; i < kNumOfRoi; i++) {
            glDeleteTextures(1, &inputtensor_texture[i]);
            glDeleteTextures(1, &detection_texture[i]);
        }

        // [GL] Swap window
        glfwSwapBuffers(window);
    }

    // Save json file for threshold
    WriteConfigJson(result);

    // [ImGui] Closing Process
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // [GL] Destroy window
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
