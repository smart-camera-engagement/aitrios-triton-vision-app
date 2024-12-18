
// Copyright © 2021 Sony Semiconductor Solutions Corporation. All rights reserved.

/**
* @file load_texture.cpp
* @brief 画像読み込み関数
* @author Shogo Takanashi
* @date 2024/9
*/

#include "./load_texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"


void SetWindowIcon(GLFWwindow* window) {
    GLFWimage images[1];
    images[0].pixels = stbi_load(kWindowIconFile, &images[0].width, &images[0].height, 0, 4);
    glfwSetWindowIcon(window, 1, images);

    stbi_image_free(images[0].pixels);
}
//
//// Simple helper function to load an image into a OpenGL texture with common settings
//bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height) {
//    // Load from file
//    int image_width = 0;
//    int image_height = 0;
//    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
//    if (image_data == NULL)
//        return false;
//
//    // Create a OpenGL texture identifier
//    GLuint parts_img_texture;
//    glGenTextures(1, &parts_img_texture);
//    glBindTexture(GL_TEXTURE_2D, parts_img_texture);
//
//    // Setup filtering parameters for display
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//
//    // Upload pixels into texture
//#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
//    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
//#endif
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
//    stbi_image_free(image_data);
//
//    *out_texture = parts_img_texture;
//    *out_width = image_width;
//    *out_height = image_height;
//
//    return true;
//    }

bool LoadTextureFromCvMat(cv::Mat* image, GLuint* out_texture) {
    // Create a OpenGL texture identifier
    GLuint parts_img_texture;
    glGenTextures(1, &parts_img_texture);
    glBindTexture(GL_TEXTURE_2D, parts_img_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image->cols, image->rows, 0, GL_BGR, GL_UNSIGNED_BYTE, image->data);

    *out_texture = parts_img_texture;

    return true;
}

//bool LoadPartsIcons(std::vector<GLuint>* parts_img_texture, std::vector<std::pair<int, int>>* parts_img_size) {
//    bool ret = true;
//    for (int i = 0; i < kPartsIconFile.size(); i++) {
//        ret |= LoadTextureFromFile(kPartsIconFile[i], &parts_img_texture->at(i), &parts_img_size->at(i).first, &parts_img_size->at(i).second);
//    }
//    return ret;
//}
//
//bool LoadTrashIcon(GLuint* trash_img_texture, std::pair<int, int>* trash_img_size) {
//    bool ret = LoadTextureFromFile(kTrashIconFile, trash_img_texture, &trash_img_size->first, &trash_img_size->second);
//    return ret;
//}
