// Copyright © 2024 Sony Semiconductor Solutions Corporation. All rights reserved.
// Author: Shogo.Takanashi@sony.com

/**
* @file json_utils.h
* @brief defines common used
* @author Shogo Takanashi
* @date 2024/11
*/

#ifndef JSON_UTILS_H_
#define JSON_UTILS_H_

#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

#include"common.h"

void ReadConfigJson(Data& data);
void WriteConfigJson(const Data& data);

#endif