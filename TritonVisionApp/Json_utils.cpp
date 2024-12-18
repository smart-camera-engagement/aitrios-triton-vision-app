// Copyright © 2024 Sony Semiconductor Solutions Corporation. All rights reserved.
// Author: Shogo.Takanashi@sony.com

/**
* @file json_utils.cpp
* @brief defines common used
* @author Shogo Takanashi
* @date 2024/11
*/

#include "./json_utils.h"

void from_json(const json& j, ROI& p) {
    p.id = j.at("ID").get<int>();
    p.offset_x = j.at("Offset_x").get<int>();
    p.offset_y = j.at("Offset_y").get<int>();
    p.width = j.at("Width").get<int>();
    p.height = j.at("Height").get<int>();
}

void to_json(ordered_json& j, const ROI& p) {
    j = ordered_json{
        { "ID", p.id},
        { "Offset_x", p.offset_x},
        { "Offset_y", p.offset_y},
        { "Width", p.width},
        { "Height", p.height}
    };
}

void from_json(const json& j, ConnectorPattern& p) {
    p.pattern_name = j.at("PatternName").get<std::string>();
    p.num_of_roi = j.at("NumOfRoi").get<int>();
    p.roi_list = j.at("RoiList").get<std::vector<ROI>>();
}

void to_json(ordered_json& j, const ConnectorPattern& p) {
    j = ordered_json{
        { "PatternName", p.pattern_name},
        { "NumOfRoi", p.num_of_roi},
        { "RoiList", p.roi_list},
    };
}

void from_json(const json& j, Data& p) {
    p.detection_threshold = j.at("DetectionThreshold").get<float>();
    p.num_of_pattern = j.at("NumOfPattern").get<int>();
    p.pattern_list = j.at("PatternList").get<std::vector<ConnectorPattern>>();
}

void to_json(ordered_json& j, const Data& p) {
    j = ordered_json{
        { "DetectionThreshold", p.detection_threshold},
        { "NumOfPattern", p.num_of_pattern},
        { "PatternList", p.pattern_list}
    };
}

void ReadConfigJson(Data& data) {

    std::ifstream stream(kConfigFile);

    if (!stream.is_open())
        throw new std::exception("Failed open file.");

    if (!json::accept(stream))
        throw new std::exception("Incorrect format");

    stream.seekg(0, std::ios::beg);
    json j = json::parse(stream);
    data = j.get<Data>();
}

void WriteConfigJson(const Data& data) {
    std::ofstream file(kConfigFile, std::ios::trunc);
    ordered_json dampj = data;
    file << dampj.dump(4);
}

