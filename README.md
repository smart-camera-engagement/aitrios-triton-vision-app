# aitrios-triton-vision-app
This repository contains an application and UI for Barcode detection (object detection on IMX500) and decoding on the post processing side (on PC) using AITRIOS triton devices. This application can be easily modified to support other use-cases as well as models (Classification, Segmentation, Anamoly etc) on IMX500.


# Operational Modes 

## Setup Mode

The Setup Mode is designed to support functionalities that would be needed to do "one-time" at the site of installation location. The Setup mode can support two main functions:

1. Specify Region of Interest (RoI) to Crop for Input Tensor
2. Capture Images from Triton camera for the purposes of AI model training/re-training.

## Inference Mode

The Inference Mode is designed to support functionalities that would be needed for final use-case and is the "operational mode" running the AI model and the post processing application. The Inference mode can support the following functions:

1. Start and Stop the Streaming of Triton Camera.
2. Specify a threshold value for the AI model on IMX500.
3. Run the AI model to obtain the input tensor and as well visualize the bounding box results on input tensor based on specified threshold.
4. Two View Modes (Setup & Inference) available. During setup mode and start of stream from triton, the RAW 12MP image will be displayed to the "Setup" Window. During inference the setup viewing is stopped to improve the frame rate and the post-processing application receives only the cropped RAW image from the sensor. The inference viewing mode, shows the cropped ROI region from 12MP, with overlayed bounding boxes based on output tensor and the decoded barcode from the post processing application.
5. Start and Stop the continuous streaming of inference data with the post processing application.
6. The current frame rate is displayed to the window. During Setup mode, because we stream the full 12MP image, the frame rate will be less than 10fps. Depending on the size of ROI, it is possible to achieve ~20fps in the inference mode.
7. The "Last Seen" decoded barcode is shown as a seperate window. If there are more than one barcode in the input tensor, the last seen value will randomly pick one of the decoded barcodes for the purposes of display.

# Barcode Detection on Triton

## AI Model Specifications

*Type: Object Detection using Neurala's Brain Builder
*Input Size: 256x256x3
*Classes supported: Barcode only
*Default Detection Threshold: 0.50

The AI model was trained on a small dataset comprising of small-sized cardboard boxes (brown and white). It is recommended to re-train the AI model with dataset representing the final use-case for the optimal performance.

## Post-Processing Application

*Where: Post processing is done on PC side.
*Type: Standalone C++ port of ZXing Library 
*Supported Barcode Type: EAN13/ UPC-A
*Supported Oritentation: Horizontal, Vertical directions (0, 90, 180, 270 degrees)

Note: Only EAN13/ UPC-A type barcodes is currently supported. It is easy to support other types of barcodes easily using this application. Please refer to the EAN13 decoder for reference. Only horizontal & vertical type barcode is supported. If further orientations are needed, please adjust the rotation angle parameter in the post processing script for improved performance.

# Steps to run Barcode Detection on Triton



