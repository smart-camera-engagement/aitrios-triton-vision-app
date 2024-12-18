// =============================================================================
//
//  Copyright (c) 2023, Lucid Vision Labs, Inc.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.
//
// =============================================================================
#ifndef ARENA_EXAMPLE_IMX501_UTILS_H
#define ARENA_EXAMPLE_IMX501_UTILS_H

// Includes --------------------------------------------------------------------
#include <vector>
#include <iostream>
#include <string>
#include <exception>
#include <stdexcept>
#include "Arena/ArenaApi.h"
#include "apParams.flatbuffers_generated.h"

// Namespace -------------------------------------------------------------------
namespace ArenaExample {

// ><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
//  ArenaExample::IMX501Utils class
// ><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
class IMX501Utils
{
public:
  // typedefs ------------------------------------------------------------------
  /**
  * This structure represents the header of Input/Output Tensor.
  * The structure has exactly same structure as the output of IMX501
  * through the MIPI connection.
  * For detail, please refer "IMX500 Software Reference Manual (Recognition)"
  */
  typedef struct
  {
    uint8_t           valid_flag;             /*!< Valid flag (0: Data is invalid, 1-255: Data is valid) */
    uint8_t           frame_count;            /*! Frame counter value. (0-254: streaming, 255: sw standby) */
    uint16_t          max_length_of_line;     /*! Maximum length of DNN valid data in the line of received data */
    uint16_t          size_of_ap_parameter;   /*! Size of AP Paramter on L2SRAM (bytes). Maximum size is 996 bytes */
    uint16_t          network_id;             /*! Network ID (0-0xFFFE) ID of inference Network selected for inference*/
    uint8_t           indicator;              /*! Indicates Input tensor or Output tensor (0: Input Tensor, 1: Output Tensor) */

    uint8_t           reserved[3];            /*! Reserved (The total size of the structure is 12bytes) */
  } tensor_header;

  /**
  * This structure represents information of Input Tensor.
  * The structure will be generated from the AP Parameter using flatbuffers. 
  * For detail, please refer "IMX500 Software Reference Manual (Recognition)"
  */
  typedef struct
  {
    uint8_t           id;                 /*!< Unique ID. Starts from zero so can be used as index */
    uint8_t           numOfDimensions;    /*!< Number of dimensions */
    uint16_t          shift;              /*!< (unused?) */
    float             scale;              /*!< (unused?) */
    uint8_t           format;             /*!< (unused?) */
  } input_tensor_info;

  /**
  * This structure represents information of Output Tensor.
  * The structure will be generated from the AP Parameter using flatbuffers. 
  * For detail, please refer "IMX500 Software Reference Manual (Recognition)"
  */
  typedef struct
  {
    uint8_t           id;                 /*!< Unique ID. Starts from zero so can be used as index */
    uint8_t           numOfDimensions;    /*!< Number of dimensions */
    uint8_t           bitsPerElement;     /*!< e.g. 8/16/32 */
    uint16_t          shift;              /*!< Const bias to be added to all tensor elements */
    float             scale;              /*!< Multiplication factor */
    uint8_t           format;             /*!< Indicates singned/unsigned?*/
  } output_tensor_info;

  /**
  * This structure represents dimension information of Input and Output Tensor.
  * The structure will be generated from the AP Parameter using flatbuffers. 
  * For detail, please refer "IMX500 Software Reference Manual (Recognition)"
  */
  typedef struct
  {
    uint8_t           id;                 /*!< Dimension ID */
    uint16_t          size;               /*!< Size of the N'th dimension*/
    uint8_t           serializationIndex; /*!< MIPI output order of the N'th dimension of the the tensor on L2SRAM (1-N) */
    uint8_t           padding;            /*!< Padding size at the end of N'th dimension. (Unit: element of the dimension e.g. planes)*/
  } dimension_info;

  // Enum ----------------------------------------------------------------------
  /**
  * This enum represents the image type of the input tensor.
  * For detail, please refer "IMX500 Software Reference Manual (Recognition)"
  * Same as the DNN0_INPUT_FORMAT(0xD750) register setting.
  */
  enum class InputImageType
  {
    IMAGE_RGB  = 0,                       /*!< 0:RGB */
    IMAGE_Y,                              /*!< 1:Y */
    IMAGE_YUV444,                         /*!< 2:YUV444 */
    IMAGE_YUV420,                         /*!< 3:YUV420 */
    IMAGE_BGR,                            /*!< 4:BGR */
    IMAGE_BAYER_RGB,                      /*!< 5:Bayer RGB */
    IMAGE_UNKNOWN = 0xFFFF                /*!< 0xFFFF: Unknown Image Format*/
  };

  /**
  * This enum represents the type of a file uploaded to a camera.
  */
  enum class CameraFileType
  {
    FILE_DEEP_NEURAL_NETWORK_FIRMWARE = 0,    /*!< 0:Deep Neural Network Firmware       = firmware.fpk */
    FILE_DEEP_NEURAL_NETWORK_LOADER,          /*!< 1:Deep Neural Network Loader         = loader.fpk */
    FILE_DEEP_NEURAL_NETWORK_NETWORK,         /*!< 2:Deep Neural Network Network        = network.fpk */
    FILE_DEEP_NEURAL_NETWORK_INFO,            /*!< 3:Deep Neural Network info           = fpk_info.dat */
    FILE_DEEP_NEURAL_NETWORK_CLASSIFICATION,  /*!< 4:Deep Neural Network Classfication  = label.txt */
    CAMERA_FILE_TYPE_UNKNOWN = 0xFFFF         /*!< 0xFFFF: Unknown Camera File Type*/
  };

  // Constructors and Destructor -----------------------------------------------
  /**
  * @fn IMX501Utils(Arena::IDevice *inDevice, bool inVerboseMode = false)
  *
  * @param inDevice
  *   - Type: Arena::IDevice*
  *   - Pointer to the IDevice object of ArenaSDK
  * @param inVerboseMode
  *   - Type: bool
  *   - Default: false
  *   - Set true to enable a verbose mode. In the verbose mode, the created
  *     object will output additional information to a console.
  *
  * The <B> constructor </B> builds a utility object for the specified camera.
  * The verbose mode can be enable by specifying true to inVerboseMode.
  */
  IMX501Utils(/*Arena::IDevice *inDevice, bool inVerboseMode = false*/);

  void SetValue(Arena::IDevice* inDevice, bool inVerboseMod)
  {
      mVerboseMode = inVerboseMod;
      mDevice = inDevice;
  };

  /**
  * @fn virtual ~IMX501Utils()
  *
  * A destructor
  */
  virtual ~IMX501Utils();

  // Member functions ----------------------------------------------------------
  /**
  * @fn void  InitCameraToOutputDNN(bool inEnableNoRawOutput, bool inEnableSensorISPManualMode)
  *
  * @param inEnableNoRawOutput
  *   - Type: bool
  *   - Enable the no raw output streaming mode by specifying true
  *
  * @param inEnableSensorISPManualMode
  *   - Type: bool
  *   - Enable the sensor ISP manual mode by specifying true
  *
  * @return
  *   - none
  *
  * <B> InitCameraToOutputDNN </B> initializes the specified camera
  * with necessary settings for running DNN inference on IMX501,
  * downloads a label text file and a fpk_info.dat from the camera
  * and prepares internal buffers.
  * If the network is not loaded into the IMX501, this function
  * instructs the camera to uplaod the network into the sensor
  * and enable it. This process requires about 30 sec to
  * finish uploading and happens only once after powering
  * up the camera.
  * The function needs to be called once after creating the IMX501Utils
  * object for the initialization.
  * Wether to enable the no raw output streaming mode or the sensor
  * ISP manual mode or not can be specified as arguments.
  * These input parameters are also optional.
  */
  void  InitCameraToOutputDNN(bool inEnableNoRawOutput = false, bool inEnableSensorISPManualMode = false);
 
  /**
  * @fn void  SetChunkData(Arena::IChunkData *inChunkData)
  *
  * @param inChunkData
  *   - Type: Arena::IChunkData*
  *   - Pointer to the IChunkData object of ArenaSDK
  *
  * @return
  *   - none
  *
  * <B> SetChunkData </B> sets a received chunk data from the camera to
  * the IMX501Utils object.
  * The object will extract an Input tensor and Output tensors from
  * the specified chunk data. The extract process in the function includes
  * the denormalization of the input tensor also.
  */
  void  SetChunkData(Arena::IChunkData *inChunkData);

  /**
  * @fn bool  ProcessChunkData(Arena::IChunkData *inChunkData)
  *
  * @param inChunkData
  *   - Type: Arena::IChunkData*
  *   - Pointer to the IChunkData object of ArenaSDK
  *
  * @return
  *   - True if the function ends with success
  *   - Otherwise, false
  *
  * <B> ProcessChunkData </B> calls SetChunkData with
  * catching exceptions from SetChunkData internally.
  * Unlike SetChunkData, <B> ProcessChunkData </B> does not throw
  * exception. Instead, the function just returns false
  * when the expectation happens from SetChunkData.
  * The function also prints out error/debug message when the object
  * is in the verbose mode.
  */
  bool  ProcessChunkData(Arena::IChunkData *inChunkData);

  // ---------------------------------------------------------------------------
  /**
  * @fn uint8_t*  GetInputImagePtr()
  *
  * @return
  *   - Type: uint8_t*
  *   - Pointer to an image buffer
  *
  * <B> GetInputImagePtr </B> returns a buffer pointer for an image that
  * is extracted from the input tensor by applying the input tensor
  * denormalization process.
  * Please note that the pixel format of the returned buffer is BGR
  * to maintain a simplicity on the windows platform, when the image
  * format is RGB.
  */
  const uint8_t *GetInputImagePtr();

  /**
  * @fn size_t  GetInputImageSize()
  *
  * @return
  *   - Type: size_t
  *   - Size of the image buffer
  *
  * <B> GetInputImageSize </B> returns the size of the input image buffer
  * that can be retrieved by the GetInputImagePtr() function.
  */
  size_t  GetInputImageSize();

  /**
  * @fn int32_t  GetInputImageWidth()
  *
  * @return
  *   - Type: int32_t
  *   - Width of the image
  *
  * <B> GetInputImageWidth </B> returns the width of the input image
  * that can be retrieved by the GetInputImagePtr() function.
  * The same value as "inputTensorWidth" in network_info.txt.
  */
  int32_t GetInputImageWidth();

  /**
  * @fn int32_t  GetInputImageHeight()
  *
  * @return
  *   - Type: int32_t
  *   - Height of the image
  *
  * <B> GetInputImageHeight </B> returns the height of the input image
  * that can be retrieved by the GetInputImagePtr() function.
  * The same value as "inputTensorHeight" in network_info.txt.
  */
  int32_t GetInputImageHeight();

  /**
  * @fn InputImageType  GetInputImageType()
  *
  * @return
  *   - Type: InputImageType
  *   - Type of the image in the input tensor
  *
  * <B> GetInputImageType </B> returns an InputImageType of the image
  * in the input tensor.
  * The same value as "inputTensorFormat" in network_info.txt.
  */
  InputImageType GetInputImageType();

  // ---------------------------------------------------------------------------
  /**
  * @fn uint32_t GetOutputTensorNum()
  *
  * @return
  *   - Type: uint32_t
  *   - Number of the output tensor
  *
  * <B> GetOutputTensorNum </B> returns the number of the output tensors
  */
  uint32_t GetOutputTensorNum();

  /**
  * @fn bool  GetOutputTensorInfo(uint32_t inIndex, output_tensor_info *outInfo)
  *
  * @param inIndex
  *   - Type: uint32_t
  *   - Index of the output tensor
  * @param outInfo
  *   - Type: output_tensor_info*
  *   - Pointer to the output_tensor_info structure of the result
  *
  * @return
  *   - True if the function ends with success
  *   - Otherwise, false
  *
  * <B> GetOutputTensorInfo </B> sets the output_tensor_info of the output
  * tensor specified in inIndex.
  */
  bool GetOutputTensorInfo(uint32_t inIndex, output_tensor_info *outInfo);

  /**
  * @fn double  GetOutputTensorScale()
  *
  * @return
  *   - Type: double
  *
  * <B> GetOutputTensorScale </B> returns the scale parameter of the output
  * tensor specified in inIndex.
  */
  double GetOutputTensorScale(uint32_t inIndex);

  /**
  * @fn bool  GetOutputTensorDimensionInfo()
  *
  * @param inIndex
  *   - Type: uint32_t
  *   - Index of the output tensor
  * @param inDim
  *   - Type: uint32_t
  *   - Index of the dimension in the specified output tensor
  * @param outInfo
  *   - Type: dimension_info*
  *   - Pointer to the dimension_info structure of the result
  *
  * @return
  *   - True if the function ends with success
  *   - Otherwise, false
  *
  * <B> GetOutputTensorDimensionInfo </B> sets the dimension_info of the
  * specified dimension in the specified output tensor.
  */
  bool GetOutputTensorDimensionInfo(uint32_t inIndex, uint32_t inDim, dimension_info *outInfo);

  /**
  * @fn void  GetOutputTensorPtr()
  *
  * @param inIndex
  *   - Type: uint32_t
  *   - Index of the output tensor
  *
  * @return
  *   - Type: void *
  *   - Pointer to the beginning of the specified output tensor
  *
  * <B> GetOutputTensorPtr </B> returns the pointer to the beginning of the
  * specified output tensor. Please note that output tensor denormalization
  * is not applied to data pointed by the returned pointer.
  */
  const void *GetOutputTensorPtr(uint32_t inIndex);

  /**
  * @fn size_t  GetOutputTensorSize(uint32_t inIndex)
  *
  * @param inIndex
  *   - Type: uint32_t
  *   - Index of the output tensor
  *
  * @return
  *   - Type: size_t
  *   - Size of the specified output tensor
  *
  * <B> GetOutputTensorSize </B> returns the size of the output tensor
  * specified by inIndex.
  */
  size_t  GetOutputTensorSize(uint32_t inIndex);

  /**
  * @fn size_t  GetLabelNum()
  *
  * @return
  *   - Type: size_t
  *   - The number of the labels
  *
  * <B> GetLabelNum </B> returns the number of the labels
  */
  size_t GetLabelNum();

  /**
  * @fn std::string  GetLabelStr(size_t inIndex)
  *
  * @param inIndex
  *   - Type: size_t
  *   - Index of the label text
  *
  * @return
  *   - Type: std::string
  *   - String object of the specified label text
  *
  * <B> GetLabelStr </B> returns the label text string object of
  * the specified index.
  */
  std::string GetLabelStr(size_t inIndex);

  /**
  * @fn std::string  GetLabelAndScoreStr(size_t inIndex, double inScore)
  *
  * @param inIndex
  *   - Type: size_t
  *   - Index of the label text
  * @param inScore
  *   - Type: double
  *   - Score of the label. (value range: 0..1)
  *
  * @return
  *   - Type: std::string
  *
  * <B> GetLabelAndScoreStr </B> returns the formatted text of the specified
  * label text with the specified score value.
  * The score value needs to be between 0 and 1.
  */
  std::string GetLabelAndScoreStr(size_t inIndex, double inScore);

  // ---------------------------------------------------------------------------
  /**
  * @fn const tensor_header*  GetInputTensorHeader()
  *
  * @return
  *   - Type: tensor_header*
  *   - Pointer to the tensor_header structure of the input tensor
  *
  * <B> GetInputTensorHeader </B> returns the pointer to the tensor_header
  * structure of the input tensor.
  */
  const tensor_header *GetInputTensorHeader();

  /**
  * @fn const void*  GetInputTensorAPParameterBufPtr()
  *
  * @return
  *   - Type: void*
  *   - Pointer to the ap paramter of the input tensor
  *
  * <B> GetInputTensorAPParameterBufPtr </B> returns the pointer to
  * the ap parameter of the input tensor.
  * Using this pointer, deserialization by flatbuffer-s can be performed.
  */
  const void *GetInputTensorAPParameterBufPtr();

  /**
  * @fn const void*  GetInputTensorsBufPtr()
  *
  * @return
  *   - Type: void*
  *   - Pointer to the buffer of the input tensor
  *
  * <B> GetInputTensorsBufPtr </B> returns the pointer to the buffer of
  * the input tensor.
  */
  const void *GetInputTensorsBufPtr();

  /**
  * @fn size_t  GetInputTensorsBufSize()
  *
  * @return
  *   - Type: size_t
  *   - Size of the input tensor buffer
  *
  * <B> GetInputTensorsBufSize </B> returns the size of the buffer, which
  * stores the input tensor.
  */
  size_t  GetInputTensorsBufSize();

  /**
  * @fn uint32_t GetInputTensorNum()
  *
  * @return
  *   - Type: uint32_t
  *   - Number of the input tensor
  *
  * <B> GetInputTensorNum </B> returns the number of the input tensors
  */
  uint32_t GetInputTensorNum();

  /**
  * @fn bool  GetInputTensorInfo()
  *
  * @param inIndex
  *   - Type: uint32_t
  *   - Index of the input tensor
  * @param outInfo
  *   - Type: input_tensor_info*
  *   - Pointer to the input_tensor_info structure of the result
  *
  * @return
  *   - True if the function ends with success
  *   - Otherwise, false
  *
  * <B> GetInputTensorInfo </B> sets the input_tensor_info of the input
  * tensor specified in inIndex.
  */
  bool GetInputTensorInfo(uint32_t inIndex, input_tensor_info *outInfo);

  /**
  * @fn bool  GetInputTensorDimensionInfo()
  *
  * @param inIndex
  *   - Type: uint32_t
  *   - Index of the input tensor
  * @param inDim
  *   - Type: uint32_t
  *   - Index of the dimension in the specified input tensor
  * @param outInfo
  *   - Type: dimension_info*
  *   - Pointer to the dimension_info structure of the result
  *
  * @return
  *   - True if the function ends with success
  *   - Otherwise, false
  *
  * <B> GetInputTensorDimensionInfo </B> sets the dimension_info of the
  * specified dimension in the specified input tensor.
  */
  bool GetInputTensorDimensionInfo(uint32_t inIndex, uint32_t inDim, dimension_info *outInfo);

  // ---------------------------------------------------------------------------
  /**
  * @fn const tensor_header*  GetOutputTensorHeader()
  *
  * @return
  *   - Pointer to the tensor_header structure of the input tensor
  *
  * <B> GetOutputTensorHeader </B> returns the pointer to the tensor_header
  * structure of the output tensor.
  */
  const tensor_header *GetOutputTensorHeader();

/**
  * @fn const void*  GetOutputTensorAPParameterBufPtr()
  *
  * @return
  *   - Type: void*
  *   - Pointer to the ap paramter of the input tensor
  *
  * <B> GetOutputTensorAPParameterBufPtr </B> returns the pointer to
  * the ap parameter of the output tensor.
  * Using this pointer, deserialization by flatbuffers can be performed.
  */
  const void *GetOutputTensorAPParameterBufPtr();

/**
  * @fn void*  GetOutputTensorsBufPtr()
  *
  * @return
  *   - Type: void*
  *   - Pointer to the buffer of the output tensor
  *
  * <B> GetInputTensorsBufPtr </B> returns the pointer to the buffer of
  * the output tensor.
  */
  const void *GetOutputTensorsBufPtr();

/**
  * @fn size_t  GetOutputTensorsBufSize()
  *
  * @return
  *   - Type: size_t
  *   - Size of the output tensor buffer
  *
  * <B> GetOutputTensorsBufSize </B> returns the size of the buffer, which
  * stores the input tensor.
  */
  size_t  GetOutputTensorsBufSize();

/**
  * @fn bool  IsVerboseMode()
  *
  * @return
  *   - True if the object is in the verbose mode
  *   - Otherwise, false
  *
  * <B> IsVerboseMode </B> returns the status of the verbose mode.
  * If true, the object is in the verbose mode.
  */
  bool  IsVerboseMode();

/**
  * @fn size_t  IsDataExtracted()
  *
  * @return
  *   - True if the object has the input tensor and the output tensors
  *   - Otherwise, false
  *
  * <B> IsDataExtracted </B> returns the input tensor and the output tensors
  * in the object. If true, the object holds the input tensor and the output
  * tensor which were extracted from the specified chunk data and processed
  * without an error.
  */
  bool  IsDataExtracted();

  /**
  * @fn bool  UploadFileToCamera(Arena::IDevice* inDevice, CameraFileType inFileType, void* inData, size_t inDataSize)
  *
  * @param inDevice
  *   - Type: Arena::IDevice*
  *   - Pointer to the IDevice object of ArenaSDK
  * @param inFileType
  *   - Type: CameraFileType
  *   - Specify the camera file type to upload to the camera
  * @param inData
  *   - Type: void*
  *   - Pointer to the data buffer for uploading
  * @param inDataSize
  *   - Type: size_t
  *   - size of data
  * @param inProgressFunc
  *   - Type: int (*inProgressFunc)(size_t inFileSize, size_t inFileTransferred)
  *   - Pointer to the optional callback function
  *
  * The <B> UploadFileToCamera </B> uploads data to the specified camera.
  * Uploaded data will be stored as a file in the camera.
  * Can specify a callback function to inform the progress of the file upload process to the caller (optional).
  * inFileSize is the size of the file (same as inDataSize)
  * inFileTransferred is the size of the data uploaded to the camera
    */
  static void UploadFileToCamera(Arena::IDevice* inDevice,
    CameraFileType inFileType, const void* inData, size_t inDataSize,
    int (*inProgressFunc)(size_t, size_t) = NULL);

protected:
  // typedefs ------------------------------------------------------------------
  typedef struct
  {
    uint16_t          input_tensor_norm_k[8];         // offset 0   (128) -- k00/k02/k03/k11/k13/k20/k22/k23
  
    uint16_t          dd_ch7_x;                       // offset 16  (144)
    uint16_t          dd_ch7_y;                       // offset 18  (146)
    uint16_t          dd_ch8_x;                       // offset 20  (148)
    uint16_t          dd_ch8_y;                       // offset 22  (150)
  
    uint8_t           input_tensor_format;            // offset 24  (152)
    uint8_t           reserved_0[3];                  // offset 25  (153)
  
    uint16_t          input_tensor_norm_ygain;        // offset 28  (156)
    uint16_t          input_tensor_norm_yadd;         // offset 30  (158)
    uint32_t          y_clip;                         // offset 32  (160)
    uint32_t          cb_clip;                        // offset 36  (164)
    uint32_t          cr_clip;                        // offset 40  (168)
  
    uint16_t          input_norm[4];                  // offset 44  (172)
    uint8_t           input_norm_shift[4];            // offset 52  (180)
    uint32_t          input_norm_clip[4];             // offset 56  (184)
  
    uint32_t          reg_wr_mask;                    // offset 72  (200)
    uint32_t          reserved_1[13];                 // offset 76  (204)
  } fpk_dnn_info;
  
  // fpk_info ver.2.0 (size = 256bytes)
  typedef struct
  {
    // Header part (Version 1 header)
    uint32_t          signature;                      // offset 0
    uint32_t          data_size;                      // offset 4
    uint16_t          version_major;                  // offset 8
    uint16_t          version_minor;                  // offset 10
    uint16_t          chip_id;                        // offset 12
    uint16_t          data_type;                      // offset 14  (section size 16bytes)
  
    //  DNN information
    int8_t            fpk_info_str[64];               // offset  16 (0)
    uint32_t          reserved_v20[11];               // offset  80 (64)
    uint32_t          network_num;                    // offset 124 (108)
  
    //  DNN0 settings
    fpk_dnn_info      dnn[1];                         // offset 128 (0)
  } fpk_info;

  // Member variables ----------------------------------------------------------
  bool  mVerboseMode;
  Arena::IDevice  *mDevice;
  GenApi::INodeMap *mNodeMap;

  fpk_info  mFPKinfo;
  uint8_t   *mLabelData;
  size_t    mLabelDataSize;
  std::vector<std::string>  mLabelList;

  size_t mChunkWidth;
  size_t mChunkHeight;
  size_t  mReceiveBufSize;

  uint8_t *mChunkBuf;
  uint8_t *mTensorBuf;
  uint8_t *mInputImageBuf;

  uint8_t *mInputTensorsPtr;
  size_t  mInputTensorsSize;

  size_t  mInputImageSize;
  int32_t mInputImageWidth;
  int32_t mInputImageHeight;
  InputImageType mInputImageType;

  uint8_t *mOutputTensorsPtr;
  size_t  mOutputTensorsSize;

  bool  mDataExtracted;
  const apParams::fb::FBApParams  *mApParams;

  // Protected static member functions -----------------------------------------
  static void RetrieveFPKinfo(GenApi::INodeMap *inNodeMap, fpk_info *outInfo);
  static uint8_t *RetrieveLabelData(GenApi::INodeMap *inNodeMap, size_t *outDataSize);
  static void MakeLabelList(const uint8_t *inLabelData, size_t inDataSize,
                                  std::vector<std::string> *outList);
  static bool ValidateFPKinfo(const fpk_info *inInfo);
  static void DumpFPKinfo(const fpk_info *inInfo);
  static void GetChunkNeuralNetworkData(Arena::IChunkData *inChunkData,
                              uint8_t *outBuf, size_t  inBufSize,
                              int64_t *outDataSize);
  static void DumpTensorHeader(const tensor_header *inHeader);
  static void DumpAPparameter(const apParams::fb::FBApParams *inParameter);
  static void ExtractTensorData(const fpk_info *inInfo, const tensor_header *inHeader,
                                           const uint8_t *inChunkBuf, size_t inReceiveBufSize,
                                           uint8_t *inTensorBuf);
  static void ExtractTensors(const fpk_info *inInfo, const tensor_header *inHeader,
                                        uint8_t *inTensorBuf, size_t inReceiveBufSize,
                                        uint8_t **outInputTensorPtr, size_t *outInputTensorSize,
                                        uint8_t **outOutputTensorPtr, size_t *outOutputTensorSize);
  static void ExtractInputImage(const fpk_info* inInfo,
                        const apParams::fb::FBApParams *inParameter,
                        const uint8_t *inInputTensorPtr, size_t inInputTensorSize,
                        uint8_t *mInputImageBuf, size_t *outInputImageSize,
                        int32_t *outInputImageWidth, int32_t *outInputImageHeight);
  static void ReadFile(GenApi::INodeMap *inNodeMap,
                              const char *inFileName,
                              void *inFileBuf, size_t inFileSize,
                              int inOffset = 0);
  static int64_t GetFileSize(GenApi::INodeMap *inNodeMap,
                              const char *inFileName);
  static void WriteFile(GenApi::INodeMap* inNodeMap,
                              const char* inFileName,
                              const void* inFileBuf, size_t inFileSize,
                              int inOffset = 0,
                              int (*inProgressFunc)(size_t, size_t) = NULL);
  static void SetEnumNode(GenApi::INodeMap *inNodeMap,
                              const char *inNodeName, const char *inValue);
  static void SetFloatNode(GenApi::INodeMap* inNodeMap,
                    const char* inNodeName, double inValue);
  static void SetBooleanNode(GenApi::INodeMap* inNodeMap,
                    const char* inNodeName, bool inValue);
  static void SetIntNode(GenApi::INodeMap *inNodeMap,
                              const char *inNodeName, int64_t inValue);
  static int64_t GetIntNode(GenApi::INodeMap *inNodeMap,
                              const char *inNodeName);
  static void ExecuteNode(GenApi::INodeMap *inNodeMap,
                              const char *inNodeName);
};

// Namespace -------------------------------------------------------------------
}
#endif //ARENA_EXAMPLE_IMX501_UTILS_H
