# aitrios-triton-vision-app
This repository contains an application and UI for Barcode detection (object detection on IMX500) and decoding on the post processing side (on PC) using AITRIOS triton devices. This application can be easily modified to support other use-cases as well as models (Classification, Segmentation, Anamoly etc) on IMX500.


# Operational Modes 

## Setup Mode

The Setup Mode is designed to support functionalities that would be needed to do "one-time" at the site of installation location. The Setup mode can support two main functions:

1. Specify Region of Interest (RoI) to Crop for Input Tensor
2. Capture Images from Triton camera for the purposes of AI model training/re-training.

![setup_mode](https://github.com/user-attachments/assets/d5f291fc-b9d4-42b5-a64b-e55decc30f54)

## Inference Mode

The Inference Mode is designed to support functionalities that would be needed for final use-case and is the "operational mode" running the AI model and the post processing application. The Inference mode can support the following functions:

1. Start and Stop the Streaming of Triton Camera.
2. Specify a threshold value for the AI model on IMX500.
3. Run the AI model to obtain the input tensor and as well visualize the bounding box results on input tensor based on specified threshold.
4. Two View Modes (Setup & Inference) available. During setup mode and start of stream from triton, the RAW 12MP image will be displayed to the "Setup" Window. During inference the setup viewing is stopped to improve the frame rate and the post-processing application receives only the cropped RAW image from the sensor. The inference viewing mode, shows the cropped ROI region from 12MP, with overlayed bounding boxes based on output tensor and the decoded barcode from the post processing application.
5. Start and Stop the continuous streaming of inference data with the post processing application.
6. The current frame rate is displayed to the window. During Setup mode, because we stream the full 12MP image, the frame rate will be less than 10fps. Depending on the size of ROI, it is possible to achieve ~20fps in the inference mode.
7. The "Last Seen" decoded barcode is shown as a seperate window. If there are more than one barcode in the input tensor, the last seen value will randomly pick one of the decoded barcodes for the purposes of display.

![inference_mode](https://github.com/user-attachments/assets/2d0803a5-1de1-410b-b449-5a8e20e5db9c)

# Barcode Detection on Triton

## AI Model Specifications

```
Type: Object Detection using Neurala's Brain Builder
Input Size: 256x256x3
Classes supported: Barcode only
Default Detection Threshold: 0.50
```

> [!NOTE]  
> The AI model was trained on a small dataset comprising of small-sized cardboard boxes (brown and white). 

> [!TIP]
> It is recommended to re-train the AI model with dataset representing the final use-case for the optimal performance.


## Post-Processing Application

```
Where: Post processing is done on PC side.
Type: Standalone C++ port of ZXing Library
Supported Barcode Type: EAN13/ UPC-A
Supported Oritentation: Horizontal, Vertical directions (0, 90, 180, 270 degrees)
```

> [!WARNING]  
> Only EAN13/ UPC-A type barcodes is currently supported. It is easy to support other types of barcodes easily using this application. Please refer to the EAN13 decoder for reference.
> Only horizontal & vertical type barcode is supported. If further orientations are needed, please adjust the rotation angle parameter in the post processing script for improved performance.

> [!TIP]
> It is recommended to crop region of interest for improved pixel density as well as resolution for barcode reading.

# Steps to run Barcode Detection on Triton

```
Step 1: Reorganize the windows in the UI based on your monitor resolution. This needs to be done only once and the positions of the windows are automatically saved when the application is closed for next time.

Step 2: Click the "Image Stream" start button to start Triton Camera.

Step 3: Click the "Setup" Tab button to see the Region of Interest.

Step 4: Once in the setup mode, you can specify the Region of Interest by dragging and adjusting the bounding box shown in the window using the mouse cursor. Alternatively, you can also use the cursors for position adjusting.

Step 5 [Optional]: You can capture images in the setup mode using the "Capture" Button. This will save the images in ".png" format and can later be used for the purposes of re-training the AI model.

Step 6: After finishing the ROI specification, you can go to the "Detector" Tab. Adjust the Threshold if needed for the object detection. Click "Run" to update the threshold parameter for the AI model and verify if you can obtain a sample input and output tensor as expected. 

Step 7: If you are satisfied with the "Detections" shown for a sample input tensor. You can click the "Start" button in the Inference Control panel to begin the application. The detected and decoded barcode will be displayed to the output Image view window.
 
```

> [!NOTE]  
> A red box in the "Image View" window indicates that a barcode is successfully detected (on IMX500) but not decoded by post processing application.
> A green box indicates successful detection and decoding.



