
// Copyright © 2021 Sony Semiconductor Solutions Corporation. All rights reserved.

/**
* @file load_texture.h
* @brief 画像読み込み関数ヘッダー
* @author Shogo Takanashi
* @date 2024/11
*/

#ifndef UXCONSOLE_LOAD_TEXTURE_H_ // NOLINT [build/header_guard]
#define UXCONSOLE_LOAD_TEXTURE_H_ // NOLINT [build/header_guard]

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <utility>
#include <vector>

#include <opencv2/opencv.hpp>

#include "./common.h"

void SetWindowIcon(GLFWwindow* window);
//bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height);
bool LoadTextureFromCvMat(cv::Mat* image, GLuint* out_texture);
//bool LoadPartsIcons(std::vector<GLuint>* parts_img_texture, std::vector<std::pair<int, int>>* parts_img_size);
//bool LoadTrashIcon(GLuint* trash_img_texture, std::pair<int, int>* trash_img_size);


#endif