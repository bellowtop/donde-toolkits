#pragma once

#include <chrono>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <stdlib.h>
#include <strings.h>
#include <vector>

struct Yolov8DetBox {
    float centerX;
    float centerY;
    float width;
    float height;
    float confidence;
};

float computeIOU(const Yolov8DetBox& boxA, const Yolov8DetBox& boxB) {
    float xA = std::max(boxA.centerX - boxA.width / 2, boxB.centerX - boxB.width / 2);
    float yA = std::max(boxA.centerY - boxA.height / 2, boxB.centerY - boxB.height / 2);
    float xB = std::min(boxA.centerX + boxA.width / 2, boxB.centerX + boxB.width / 2);
    float yB = std::min(boxA.centerY + boxA.height / 2, boxB.centerY + boxB.height / 2);

    float interArea = std::max(0.0f, xB - xA) * std::max(0.0f, yB - yA);
    float boxAArea = boxA.width * boxA.height;
    float boxBArea = boxB.width * boxB.height;
    float iou = interArea / (boxAArea + boxBArea - interArea);

    return iou;
}

std::vector<int> nonMaxSuppression(const std::vector<Yolov8DetBox>& boxes, float iouThreshold) {
    std::vector<int> indices(boxes.size());
    for (int i = 0; i < boxes.size(); i++) {
        indices[i] = i;
    }

    std::vector<int> keep;
    while (!indices.empty()) {
        int i = indices.front();
        keep.push_back(i);
        indices.erase(indices.begin());

        for (auto it = indices.begin(); it != indices.end();) {
            if (computeIOU(boxes[i], boxes[*it]) > iouThreshold) {
                indices.erase(it);
            } else {
                it++;
            }
        }
    }

    return keep;
}
