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

// Includes --------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include "Arena/ArenaApi.h"
#include "IMX501Utils.h"

// Macros ----------------------------------------------------------------------
#define FPK_INFO_SIGNATURE      0x4443554C  // 'L' 'U' 'C' 'D'
#define FPK_INFO_CHIP_ID        501         // IMX501
#define FPK_INFO_DATA_TYPE      0x0001      // DNN Output setting

// Namespace -------------------------------------------------------------------
namespace ArenaExample {

// ><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
//  IMX501Utils class
// ><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
// Constructors and Destructor -------------------------------------------------
// -----------------------------------------------------------------------------
//  IMX501Utils
// -----------------------------------------------------------------------------
IMX501Utils::IMX501Utils(/*Arena::IDevice *inDevice, bool inVerboseMode*/)
{
  //mVerboseMode = inVerboseMode;

  //mDevice = inDevice;
  mNodeMap = NULL;

  memset(&mFPKinfo, 0, sizeof(mFPKinfo));
  mLabelData = NULL;
  mLabelDataSize = 0;

  mChunkWidth = 0;
  mChunkHeight = 0;
  mReceiveBufSize = 0;

  mChunkBuf = NULL;
  mTensorBuf = NULL;
  mInputImageBuf = NULL;

  mInputTensorsPtr = NULL;
  mInputTensorsSize = 0;
  mOutputTensorsPtr = NULL;
  mOutputTensorsSize = 0;

  mInputImageSize = 0;
  mInputImageWidth = 0;
  mInputImageHeight = 0;
  mInputImageType = InputImageType::IMAGE_UNKNOWN;

  mApParams = NULL;
  mDataExtracted = false;
}

// -----------------------------------------------------------------------------
//  ~IMX501Utils
// -----------------------------------------------------------------------------
IMX501Utils::~IMX501Utils()
{
  if (mLabelData != 0)
    delete[] mLabelData;
  if (mChunkBuf != 0)
    delete[] mChunkBuf;
  if (mTensorBuf != 0)
    delete[] mTensorBuf;
  if (mInputImageBuf != 0)
    delete[] mInputImageBuf;
}

// -----------------------------------------------------------------------------
// Public member functions -----------------------------------------------------
// -----------------------------------------------------------------------------
//  InitCameraToOutputDNN
// -----------------------------------------------------------------------------
void IMX501Utils::InitCameraToOutputDNN(bool inEnableNoRawOutput, bool inEnableSensorISPManualMode)
{
  mDataExtracted = false;

  if (mDevice == NULL)
    throw std::runtime_error("mDevice == NULL");

  mNodeMap = mDevice->GetNodeMap();
  if (mNodeMap == NULL)
    throw std::runtime_error("Couldn't get the NodeMap");

  GenApi::CBooleanPtr pNetworkEnable = mNodeMap->GetNode("DeepNeuralNetworkEnable");
  if (pNetworkEnable == NULL)
    throw std::runtime_error("Couldn't find node: DeepNeuralNetworkEnable");

  if (pNetworkEnable->GetAccessMode() == GenApi::EAccessMode::RO)
  {
    // Always output the log for now.
    // Since the execution of DeepNeuralNetworkLoad takes time 
    printf("Execute: DeepNeuralNetworkLoad\n");
    ExecuteNode(mNodeMap, "DeepNeuralNetworkLoad");
    printf("Execute: Done\n");
  }

  GenApi::CBooleanPtr pManualEnable = mNodeMap->GetNode("DeepNeuralNetworkISPAutoEnable");
  if (pManualEnable == NULL)
    throw std::runtime_error("Couldn't find node: DeepNeuralNetworkISPAutoEnable");
  if (pManualEnable->GetValue() != (!inEnableSensorISPManualMode))
  {
    if (pNetworkEnable->GetValue() != false)
      pNetworkEnable->SetValue(false);
    pManualEnable->SetValue((!inEnableSensorISPManualMode));
    if (mVerboseMode)
      printf("Changed: DeepNeuralNetworkISPAutoEnable\n");
  }

  if (pNetworkEnable->GetValue() == false)
  {
    if (mVerboseMode)
      printf("Enable: DeepNeuralNetworkEnable\n");
    pNetworkEnable->SetValue(true);
    if (mVerboseMode)
      printf("Enable: Done\n");
  }

  SetBooleanNode(mNodeMap, "GammaEnable", true);
  SetFloatNode(mNodeMap, "Gamma", 0.45);

  RetrieveFPKinfo(mNodeMap, &mFPKinfo);
  if (mVerboseMode)
    DumpFPKinfo(&mFPKinfo);
  if (ValidateFPKinfo(&mFPKinfo) == false)
    throw std::runtime_error("Received invalid fpk_info");

  mLabelData = RetrieveLabelData(mNodeMap, &mLabelDataSize);
  if (mLabelDataSize != 0)
    MakeLabelList(mLabelData, mLabelDataSize, &mLabelList);

  mChunkWidth = (size_t)mFPKinfo.dnn[0].dd_ch7_x;
  mChunkHeight = (size_t)mFPKinfo.dnn[0].dd_ch7_y + (size_t)mFPKinfo.dnn[0].dd_ch8_y;
  mReceiveBufSize = mChunkWidth * mChunkHeight;
  mChunkBuf = new uint8_t[mReceiveBufSize];
  if (mChunkBuf == NULL)
    throw std::runtime_error("mChunkBuf == NULL");

  mTensorBuf = new uint8_t[mReceiveBufSize];
  if (mTensorBuf == NULL)
  {
    delete[] mChunkBuf;
    mChunkBuf = NULL;
    throw std::runtime_error("mTensorBuf == NULL");
  }

  mInputImageBuf = new uint8_t[mReceiveBufSize];
  if (mInputImageBuf == NULL)
  {
    delete[] mChunkBuf;
    mChunkBuf = NULL;
    delete[] mTensorBuf;
    mTensorBuf = NULL;
    throw std::runtime_error("mInputImageBuf == NULL");
  }
  mInputImageType = (InputImageType )mFPKinfo.dnn[0].input_tensor_format;

  if (inEnableNoRawOutput != false)
  {
    SetIntNode(mNodeMap, "Width",  4);
    SetIntNode(mNodeMap, "Height", 4);
  }
}

// -----------------------------------------------------------------------------
//  SetChunkData
// -----------------------------------------------------------------------------
void IMX501Utils::SetChunkData(Arena::IChunkData *inChunkData)
{
  mDataExtracted = false;
  if (mDevice == NULL || inChunkData == NULL ||
      mChunkBuf == NULL || mTensorBuf == NULL || mInputImageBuf == NULL)
    throw std::runtime_error("Necessary object or buffer is NULL");

  if (inChunkData->IsIncomplete())
    throw std::runtime_error("inChunkData->IsIncomplete() returned true");

  memset(mChunkBuf, 0, mReceiveBufSize);
  memset(mTensorBuf, 0, mReceiveBufSize);
  memset(mInputImageBuf, 0, mReceiveBufSize);

  int64_t dataSize;
  GetChunkNeuralNetworkData(inChunkData, mChunkBuf, mReceiveBufSize, &dataSize);
  if (mVerboseMode)
  {
    printf("[InputTensor]\n");
    DumpTensorHeader(GetInputTensorHeader());
    printf("[OutputTensor]\n");
    DumpTensorHeader(GetOutputTensorHeader());
  }
  ExtractTensorData(&mFPKinfo, GetInputTensorHeader(),
                    mChunkBuf, mReceiveBufSize,
                    mTensorBuf);
  ExtractTensors(&mFPKinfo, GetInputTensorHeader(),
                mTensorBuf, mReceiveBufSize,
                 &mInputTensorsPtr, &mInputTensorsSize,
                 &mOutputTensorsPtr, &mOutputTensorsSize);

  mApParams = apParams::fb::GetFBApParams(GetInputTensorAPParameterBufPtr());
  if (mVerboseMode)
  {
    printf("[InputTensor]\n");
    DumpAPparameter(mApParams);
  }

  ExtractInputImage(&mFPKinfo,
                    mApParams, mInputTensorsPtr, mInputTensorsSize,
                    mInputImageBuf, &mInputImageSize,
                    &mInputImageWidth, &mInputImageHeight);

  if (mVerboseMode)
  {
    const apParams::fb::FBApParams  *params;
    params = apParams::fb::GetFBApParams(GetOutputTensorAPParameterBufPtr());
    printf("[OutputTensor]\n");
    DumpAPparameter(params);
  }
  mDataExtracted = true;
}

// -----------------------------------------------------------------------------
//  ProcessChunkData
// -----------------------------------------------------------------------------
bool IMX501Utils::ProcessChunkData(Arena::IChunkData *inChunkData)
{
  try
  {
    SetChunkData(inChunkData);
  }

  catch (GenICam::GenericException& ge)
  {
    if (IsVerboseMode())
      printf("Error: GenICam exception thrown: %s\n", ge.what());
    return false;
  }
  catch (std::exception& ex)
  {
    if (IsVerboseMode())
      printf("Error: Standard exception thrown: %s\n", ex.what());
    return false;
  }
  catch (...)
  {
    if (IsVerboseMode())
      printf("Error: Unexpected exception thrown\n");
    return false;
  }

  return true;
}

// -----------------------------------------------------------------------------
//  GetInputTensorHeader
// -----------------------------------------------------------------------------
const IMX501Utils::tensor_header *IMX501Utils::GetInputTensorHeader()
{
  return (tensor_header *)mChunkBuf;
}

// -----------------------------------------------------------------------------
//  GetInputTensorAPParameterBufPtr
// -----------------------------------------------------------------------------
const void *IMX501Utils::GetInputTensorAPParameterBufPtr()
{
  if (mChunkBuf == NULL)
    return NULL;
  return (((uint8_t *)GetInputTensorHeader()) + sizeof(tensor_header));
}

// -----------------------------------------------------------------------------
//  GetInputTensorsBufPtr
// -----------------------------------------------------------------------------
const void *IMX501Utils::GetInputTensorsBufPtr()
{
  return mInputTensorsPtr;
}

// -----------------------------------------------------------------------------
//  GetInputTensorsBufSize
// -----------------------------------------------------------------------------
size_t IMX501Utils::GetInputTensorsBufSize()
{
  return mInputTensorsSize;
}

// -----------------------------------------------------------------------------
//  GetInputTensorNum
// -----------------------------------------------------------------------------
uint32_t IMX501Utils::GetInputTensorNum()
{
  return mApParams->networks()->Get(0)->inputTensors()->size();
}

// -----------------------------------------------------------------------------
//  GetInputTensorInfo
// -----------------------------------------------------------------------------
bool IMX501Utils::GetInputTensorInfo(uint32_t inIndex, input_tensor_info *outInfo)
{
  if (inIndex >= mApParams->networks()->Get(0)->inputTensors()->size())
    return false;

  outInfo->id = mApParams->networks()->Get(0)->inputTensors()->Get(inIndex)->id();
  outInfo->numOfDimensions = mApParams->networks()->Get(0)->inputTensors()->Get(inIndex)->numOfDimensions();
  outInfo->shift = mApParams->networks()->Get(0)->inputTensors()->Get(inIndex)->shift();
  outInfo->scale = mApParams->networks()->Get(0)->inputTensors()->Get(inIndex)->scale();
  outInfo->format = mApParams->networks()->Get(0)->inputTensors()->Get(inIndex)->format();
  return true;
}

// -----------------------------------------------------------------------------
//  GetInputTensorDimensionInfo
// -----------------------------------------------------------------------------
bool IMX501Utils::GetInputTensorDimensionInfo(uint32_t inIndex, uint32_t inDim, dimension_info *outInfo)
{
  if (inIndex >= mApParams->networks()->Get(0)->inputTensors()->size())
    return false;
  if (inDim >= mApParams->networks()->Get(0)->inputTensors()->Get(inIndex)->numOfDimensions())
    return false;

  outInfo->id = mApParams->networks()->Get(0)->inputTensors()->Get(inIndex)->dimensions()->Get(inDim)->id();
  outInfo->size = mApParams->networks()->Get(0)->inputTensors()->Get(inIndex)->dimensions()->Get(inDim)->size();
  outInfo->serializationIndex = mApParams->networks()->Get(0)->inputTensors()->Get(inIndex)->dimensions()->Get(inDim)->serializationIndex();
  outInfo->padding = mApParams->networks()->Get(0)->inputTensors()->Get(inIndex)->dimensions()->Get(inDim)->padding();
  return true;
}

// -----------------------------------------------------------------------------
//  GetOutputTensorHeader
// -----------------------------------------------------------------------------
const IMX501Utils::tensor_header *IMX501Utils::GetOutputTensorHeader()
{
  if (mChunkBuf == NULL)
    return NULL;
  const uint8_t *bufPtr = mChunkBuf;
  bufPtr += ((size_t)mFPKinfo.dnn[0].dd_ch7_x * (size_t)mFPKinfo.dnn[0].dd_ch7_y);
  return (tensor_header *)bufPtr;
}

// -----------------------------------------------------------------------------
//  GetOutputTensorAPParameterBufPtr
// -----------------------------------------------------------------------------
const void *IMX501Utils::GetOutputTensorAPParameterBufPtr()
{
  if (mChunkBuf == NULL)
    return NULL;
  return (((uint8_t *)GetOutputTensorHeader()) + sizeof(tensor_header));
}

// -----------------------------------------------------------------------------
//  GetOutputTensorsBufPtr
// -----------------------------------------------------------------------------
const void *IMX501Utils::GetOutputTensorsBufPtr()
{
  return mOutputTensorsPtr;
}

// -----------------------------------------------------------------------------
//  GetOutputTensorsBufSize
// -----------------------------------------------------------------------------
size_t IMX501Utils::GetOutputTensorsBufSize()
{
  return mOutputTensorsSize;
}

// -----------------------------------------------------------------------------
//  GetOutputTensorNum
// -----------------------------------------------------------------------------
uint32_t IMX501Utils::GetOutputTensorNum()
{
  return mApParams->networks()->Get(0)->outputTensors()->size();
}

// -----------------------------------------------------------------------------
//  GetOutputTensorInfo
// -----------------------------------------------------------------------------
bool IMX501Utils::GetOutputTensorInfo(uint32_t inIndex, output_tensor_info *outInfo)
{
  if (inIndex >= mApParams->networks()->Get(0)->outputTensors()->size())
    return false;

  outInfo->id = mApParams->networks()->Get(0)->outputTensors()->Get(inIndex)->id();
  outInfo->numOfDimensions = mApParams->networks()->Get(0)->outputTensors()->Get(inIndex)->numOfDimensions();
  outInfo->bitsPerElement = mApParams->networks()->Get(0)->outputTensors()->Get(inIndex)->bitsPerElement();
  outInfo->shift = mApParams->networks()->Get(0)->outputTensors()->Get(inIndex)->shift();
  outInfo->scale = mApParams->networks()->Get(0)->outputTensors()->Get(inIndex)->scale();
  outInfo->format = mApParams->networks()->Get(0)->outputTensors()->Get(inIndex)->format();
  return true;
}

// -----------------------------------------------------------------------------
//  GetOutputTensorScale
// -----------------------------------------------------------------------------
double IMX501Utils::GetOutputTensorScale(uint32_t inIndex)
{
  if (inIndex >= mApParams->networks()->Get(0)->outputTensors()->size())
    return 0;
  return mApParams->networks()->Get(0)->outputTensors()->Get(inIndex)->scale();
}

// -----------------------------------------------------------------------------
//  GetOutputTensorDimensionInfo
// -----------------------------------------------------------------------------
bool IMX501Utils::GetOutputTensorDimensionInfo(uint32_t inIndex, uint32_t inDim, dimension_info *outInfo)
{
  if (inIndex >= mApParams->networks()->Get(0)->outputTensors()->size())
    return false;
  if (inDim >= mApParams->networks()->Get(0)->outputTensors()->Get(inIndex)->numOfDimensions())
    return false;

  outInfo->id = mApParams->networks()->Get(0)->outputTensors()->Get(inIndex)->dimensions()->Get(inDim)->id();
  outInfo->size = mApParams->networks()->Get(0)->outputTensors()->Get(inIndex)->dimensions()->Get(inDim)->size();
  outInfo->serializationIndex = mApParams->networks()->Get(0)->outputTensors()->Get(inIndex)->dimensions()->Get(inDim)->serializationIndex();
  outInfo->padding = mApParams->networks()->Get(0)->outputTensors()->Get(inIndex)->dimensions()->Get(inDim)->padding();
  return true;
}

// -----------------------------------------------------------------------------
//  GetOutputTensorSize
// -----------------------------------------------------------------------------
size_t  IMX501Utils::GetOutputTensorSize(uint32_t inIndex)
{
  output_tensor_info  info;
  if (GetOutputTensorInfo(inIndex, &info) == false)
    return 0;

  dimension_info dimInfo;
  size_t  tensorSize = 1;
  for (uint32_t i = 0; i < info.numOfDimensions; i++)
  {
    GetOutputTensorDimensionInfo(inIndex, i, &dimInfo);
    tensorSize *= ((size_t)dimInfo.size + (size_t)dimInfo.padding);
  }
  tensorSize *= ((size_t)info.bitsPerElement / 8);
  return tensorSize;
}

// -----------------------------------------------------------------------------
//  GetOutputTensorPtr
// -----------------------------------------------------------------------------
const void *IMX501Utils::GetOutputTensorPtr(uint32_t inIndex)
{
  const tensor_header *header = GetOutputTensorHeader();
  if (header == NULL)
    return NULL;
  if (inIndex >= mApParams->networks()->Get(0)->outputTensors()->size())
    return NULL;
  size_t  offset = 0;

  for (uint32_t i = 0; i < inIndex; i++)
  {
    size_t tensorSize = GetOutputTensorSize(i);
    if (tensorSize == 0)
      return NULL;
    size_t lineNum = tensorSize / header->max_length_of_line;
    if ((tensorSize % header->max_length_of_line) != 0)
      lineNum++;
    tensorSize = lineNum * header->max_length_of_line;
    offset += tensorSize;
  }

  if (offset > mOutputTensorsSize)
    return NULL;
  return &(mOutputTensorsPtr[offset]);
}

// -----------------------------------------------------------------------------
//  GetInputImagePtr
// -----------------------------------------------------------------------------
const uint8_t *IMX501Utils::GetInputImagePtr()
{
  return mInputImageBuf;
}

// -----------------------------------------------------------------------------
//  GetInputImageSize
// -----------------------------------------------------------------------------
size_t IMX501Utils::GetInputImageSize()
{
  return mInputImageSize;
}

// -----------------------------------------------------------------------------
//  GetInputImageWidth
// -----------------------------------------------------------------------------
int32_t IMX501Utils::GetInputImageWidth()
{
  return mInputImageWidth;
}

// -----------------------------------------------------------------------------
//  GetInputImageHeight
// -----------------------------------------------------------------------------
int32_t IMX501Utils::GetInputImageHeight()
{
  return mInputImageHeight;
}

// -----------------------------------------------------------------------------
//  GetInputImageType
// -----------------------------------------------------------------------------
IMX501Utils::InputImageType IMX501Utils::GetInputImageType()
{
  return mInputImageType;
}

// -----------------------------------------------------------------------------
//  IsVerboseMode
// -----------------------------------------------------------------------------
bool IMX501Utils::IsVerboseMode()
{
  return mVerboseMode;
}

// -----------------------------------------------------------------------------
//  IsDataExtracted
// -----------------------------------------------------------------------------
bool IMX501Utils::IsDataExtracted()
{
  return mDataExtracted;
}

// -----------------------------------------------------------------------------
//  GetLabelNum
// -----------------------------------------------------------------------------
size_t IMX501Utils::GetLabelNum()
{
  return mLabelList.size();
}

// -----------------------------------------------------------------------------
//  GetLabelStr
// -----------------------------------------------------------------------------
std::string IMX501Utils::GetLabelStr(size_t inIndex)
{
  if (inIndex >= mLabelList.size())
    return "ERROR: list index out of range";
  return mLabelList[inIndex];
}

// -----------------------------------------------------------------------------
//  GetLabelAndScoreStr
// -----------------------------------------------------------------------------
std::string IMX501Utils::GetLabelAndScoreStr(size_t inIndex, double inScore)
{
  if (inIndex >= mLabelList.size())
    return "ERROR: list index out of range";

  char buf[80];
  sprintf_s(buf, sizeof(buf), " (%d%%)", (int)(inScore * 100.0));
  std::string str = mLabelList[inIndex] + buf;
  return str;
}

// -----------------------------------------------------------------------------
// Public static member functions -------------------------------------------
// -----------------------------------------------------------------------------
//  UploadFileToCamera
// -----------------------------------------------------------------------------
void IMX501Utils::UploadFileToCamera(Arena::IDevice* inDevice,
  CameraFileType inFileType, const void* inData, size_t inDataSize,
  int (*inProgressFunc)(size_t, size_t))
{
  const char* fileName = "";
  switch (inFileType)
  {
    case CameraFileType::FILE_DEEP_NEURAL_NETWORK_FIRMWARE:
      fileName = "DeepNeuralNetworkFirmware";
      break;
    case CameraFileType::FILE_DEEP_NEURAL_NETWORK_LOADER:
      fileName = "DeepNeuralNetworkLoader";
      break;
    case CameraFileType::FILE_DEEP_NEURAL_NETWORK_NETWORK:
      fileName = "DeepNeuralNetworkNetwork";
      break;
    case CameraFileType::FILE_DEEP_NEURAL_NETWORK_INFO:
      fileName = "DeepNeuralNetworkInfo";
      break;
    case CameraFileType::FILE_DEEP_NEURAL_NETWORK_CLASSIFICATION:
      fileName = "DeepNeuralNetworkClassification";
      break;
    default:
      throw std::runtime_error("inFileType == Unknown");
      break;
  }
  GenApi::INodeMap* pNodeMap = inDevice->GetNodeMap();

  WriteFile(pNodeMap, fileName, inData, inDataSize, 0, inProgressFunc);
}


// -----------------------------------------------------------------------------
// Protected static member functions -------------------------------------------
// -----------------------------------------------------------------------------
//  RetrieveFPKinfo
// -----------------------------------------------------------------------------
void IMX501Utils::RetrieveFPKinfo(GenApi::INodeMap *inNodeMap,
                              fpk_info *outInfo)
{
  ReadFile(inNodeMap, "DeepNeuralNetworkInfo",
               outInfo, sizeof(fpk_info));
}

// -----------------------------------------------------------------------------
//  RetrieveLabelData
// -----------------------------------------------------------------------------
uint8_t *IMX501Utils::RetrieveLabelData(GenApi::INodeMap *inNodeMap, size_t *outDataSize)
{
  *outDataSize = (size_t )GetFileSize(inNodeMap, "DeepNeuralNetworkClassification");
  if (*outDataSize == 0)
    return NULL;
  uint8_t *dataBuf = new uint8_t[*outDataSize];
  if (dataBuf == NULL)
    return NULL;
  ReadFile(inNodeMap, "DeepNeuralNetworkClassification",
               dataBuf, *outDataSize);
  return dataBuf;
}

// -----------------------------------------------------------------------------
//  MakeLabelList
// -----------------------------------------------------------------------------
void IMX501Utils::MakeLabelList(const uint8_t *inLabelData, size_t inDataSize,
                                  std::vector<std::string> *outList)
{
  if (inLabelData == NULL || inDataSize == 0 || outList == NULL)
    return;

  outList->clear();

  size_t  labelNum = 0;
  size_t  i, from = 0;
  for (i = 0; i < inDataSize; i++)
  {
    if (inLabelData[i] == 0x0A)
    {
      if (from == i)
        outList->push_back("");
      else
        outList->push_back(std::string((const char *)&(inLabelData[from]), i - from));
      from = i + 1;
    }
  }
  if (from != i)
    outList->push_back(std::string((const char*)&(inLabelData[from]), i - from));
}

// -----------------------------------------------------------------------------
//  ValidateFPKinfo
// -----------------------------------------------------------------------------
bool IMX501Utils::ValidateFPKinfo(const fpk_info *inInfo)
{
  // check the header consistency
  if (inInfo->signature      != FPK_INFO_SIGNATURE ||
      inInfo->version_major  >  0x02 ||
      inInfo->chip_id        != FPK_INFO_CHIP_ID   ||
      inInfo->data_type      != FPK_INFO_DATA_TYPE)
    return false;

  // sanity check
  if (inInfo->dnn[0].dd_ch7_x  == 0 ||
      inInfo->dnn[0].dd_ch7_y  == 0 ||
      inInfo->dnn[0].dd_ch8_x  == 0 ||
      inInfo->dnn[0].dd_ch8_y  == 0 )
    return false;

  // the current version doesn't support the following configuration
  if (inInfo->dnn[0].dd_ch7_x != inInfo->dnn[0].dd_ch8_x)
    return false;

  return true;
}

// -----------------------------------------------------------------------------
//  DumpFPKinfo
// -----------------------------------------------------------------------------
void IMX501Utils::DumpFPKinfo(const fpk_info *inInfo)
{
  printf("[fpk_info]\n");
  printf("signature:     0x%08X\n", inInfo->signature);
  printf("data_size:     %d\n", inInfo->data_size);
  printf("version_major: %d\n", (int )inInfo->version_major);
  printf("version_minor: %d\n", (int )inInfo->version_minor);
  printf("chip_id:       %d\n", (int )inInfo->chip_id);
  printf("data_type:     %d\n", (int )inInfo->data_type);

  char  buf[65];
  memcpy(buf, inInfo->fpk_info_str, 64);
  buf[64] = 0;
  if (strlen(buf) > 63)
    printf("warning: fpk_info_str is not null terminated\n");
  printf("fpk_info_str:  %s\n", buf);
  printf("network_num:   %d\n", inInfo->network_num);

  printf("dnn[0]\n");
  printf("dd_ch7_x:      %d\n", (int )inInfo->dnn[0].dd_ch7_x);
  printf("dd_ch7_y:      %d\n", (int )inInfo->dnn[0].dd_ch7_y);
  printf("dd_ch8_x:      %d\n", (int )inInfo->dnn[0].dd_ch8_x);
  printf("dd_ch8_y:      %d\n", (int )inInfo->dnn[0].dd_ch8_y);

  for (int i = 0; i < 8; i++)
    printf("input_tensor_norm_k[%d]: 0x%04X\n", i, (int )inInfo->dnn[0].input_tensor_norm_k[i]);
  printf("input_tensor_format:     %d\n", (int )inInfo->dnn[0].input_tensor_format);
  printf("input_tensor_norm_ygain: 0x%04X\n", (int )inInfo->dnn[0].input_tensor_norm_ygain);
  printf("input_tensor_norm_yadd:  0x%04X\n", (int )inInfo->dnn[0].input_tensor_norm_yadd);
  printf("y_clip:        0x%08X\n", inInfo->dnn[0].y_clip);
  printf("cb_clip:       0x%08X\n", inInfo->dnn[0].cb_clip);
  printf("cr_clip:       0x%08X\n", inInfo->dnn[0].cr_clip);
  for (int i = 0; i < 4; i++)
  {
    printf("input_norm[%d]:         0x%04X\n", i, (int )inInfo->dnn[0].input_norm[i]);
    printf("input_norm_shift[%d]:   0x%02X\n", i, (int )inInfo->dnn[0].input_norm_shift[i]);
    printf("input_norm_clip[%d]:    0x%08X\n", i, inInfo->dnn[0].input_norm_clip[i]);
  }
  printf("\n");
}

// -----------------------------------------------------------------------------
//  GetChunkNeuralNetworkData
// -----------------------------------------------------------------------------
void IMX501Utils::GetChunkNeuralNetworkData(Arena::IChunkData *inChunkData,
                              uint8_t *outBuf, size_t  inBufSize,
                              int64_t *outDataSize)
{
  GenApi::CIntegerPtr pChunkDeepNeuralNetworkLength = inChunkData->GetChunk("ChunkDeepNeuralNetworkLength");
  if (!pChunkDeepNeuralNetworkLength ||
      !GenApi::IsAvailable(pChunkDeepNeuralNetworkLength) ||
      !GenApi::IsReadable(pChunkDeepNeuralNetworkLength))
  {
    throw GenICam::GenericException("missing chunk data [ChunkDeepNeuralNetworkLength]", __FILE__, __LINE__);
  }
  *outDataSize = pChunkDeepNeuralNetworkLength->GetValue();
  if (*outDataSize == 0)
    throw GenICam::GenericException("ChunkDeepNeuralNetworkLength is 0", __FILE__, __LINE__);
  if (*outDataSize > (int64_t )inBufSize)
    throw GenICam::GenericException("ChunkDeepNeuralNetworkLength is bigger than inBufSize", __FILE__, __LINE__);

  GenApi::CRegisterPtr pChunkDeepNeuralNetwork = inChunkData->GetChunk("ChunkDeepNeuralNetwork");
  if (!pChunkDeepNeuralNetwork ||
      !GenApi::IsAvailable(pChunkDeepNeuralNetwork) ||
      !GenApi::IsReadable(pChunkDeepNeuralNetwork))
  {
    throw GenICam::GenericException("missing chunk data [ChunkDeepNeuralNetwork]", __FILE__, __LINE__);
  }

  pChunkDeepNeuralNetwork->Get(outBuf, *outDataSize);
}

// -----------------------------------------------------------------------------
//  DumpTensorHeader
// -----------------------------------------------------------------------------
void IMX501Utils::DumpTensorHeader(const tensor_header *inHeader)
{
  printf("[Header]\n");
  printf("sizeof(tensor_header)       = %zd bytes\n", sizeof(tensor_header));
  printf("header.valid_flag:            0x%02X\n", inHeader->valid_flag);
  printf("header.frame_count:           %d\n", inHeader->frame_count);
  printf("header.max_length_of_line:    %d\n", inHeader->max_length_of_line);
  printf("header.size_of_ap_parameter:  %d\n", inHeader->size_of_ap_parameter);
  printf("header.network_id:            0x%02X\n", inHeader->network_id);
  printf("header.indicator :            0x%02X\n", inHeader->indicator);
  printf("\n");
}

// -----------------------------------------------------------------------------
//  DumpAPparameter
// -----------------------------------------------------------------------------
void IMX501Utils::DumpAPparameter(const apParams::fb::FBApParams *inParameter)
{
  int i, size_i, j, size_j;

  printf("[AP parameter]\n");
  printf(" networks.size  = %d\n", inParameter->networks()->size());
  printf(" networks[0]\n");
  printf("  id            = %d\n", inParameter->networks()->Get(0)->id());
  printf("  name          = %s\n", inParameter->networks()->Get(0)->name()->c_str());
  printf("  type          = %s\n", inParameter->networks()->Get(0)->type()->c_str());
  printf("\n");
  //
  size_i = inParameter->networks()->Get(0)->inputTensors()->size();
  printf("  inputTensors.size = %d\n", size_i);
  for (i = 0; i < size_i; i++)
  {
    printf("  inputTensors[%d]\n", i);
    printf("   id               = %d\n", inParameter->networks()->Get(0)->inputTensors()->Get(i)->id());
    printf("   name             = %s\n", inParameter->networks()->Get(0)->inputTensors()->Get(i)->name()->c_str());
    printf("   numOfDimensions  = %d\n", inParameter->networks()->Get(0)->inputTensors()->Get(i)->numOfDimensions());
    size_j = inParameter->networks()->Get(0)->inputTensors()->Get(i)->dimensions()->size();
    printf("   dimensions.size  = %d\n", size_j);
    for (j = 0; j < size_j; j++)
    {
      printf("   dimensions[%d]\n", j);
      printf("    id                  = %d\n", inParameter->networks()->Get(0)->inputTensors()->Get(i)->dimensions()->Get(j)->id());
      printf("    size                = %d\n", inParameter->networks()->Get(0)->inputTensors()->Get(i)->dimensions()->Get(j)->size());
      printf("    serializationIndex  = %d\n", inParameter->networks()->Get(0)->inputTensors()->Get(i)->dimensions()->Get(j)->serializationIndex());
      printf("    padding             = %d\n", inParameter->networks()->Get(0)->inputTensors()->Get(i)->dimensions()->Get(j)->padding());
    }
    printf("   shift            = %d\n", inParameter->networks()->Get(0)->inputTensors()->Get(i)->shift());
    printf("   scale            = %f\n", (double)inParameter->networks()->Get(0)->inputTensors()->Get(i)->scale());
    printf("   format           = %d\n", inParameter->networks()->Get(0)->inputTensors()->Get(i)->format());
  }
  printf("\n");
  //
  size_i = inParameter->networks()->Get(0)->outputTensors()->size();
  printf("  outputTensors.size = %d\n", size_i);
  for (i = 0; i < size_i; i++)
  {
    printf("  outputTensors[%d]\n", i);
    printf("   id               = %d\n", inParameter->networks()->Get(0)->outputTensors()->Get(i)->id());
    printf("   name             = %s\n", inParameter->networks()->Get(0)->outputTensors()->Get(i)->name()->c_str());
    printf("   numOfDimensions  = %d\n", inParameter->networks()->Get(0)->outputTensors()->Get(i)->numOfDimensions());
    size_j = inParameter->networks()->Get(0)->outputTensors()->Get(i)->dimensions()->size();
    printf("   dimensions.size  = %d\n", size_j);
    for (j = 0; j < size_j; j++)
    {
      printf("   dimensions[%d]\n", j);
      printf("    id                  = %d\n", inParameter->networks()->Get(0)->outputTensors()->Get(i)->dimensions()->Get(j)->id());
      printf("    size                = %d\n", inParameter->networks()->Get(0)->outputTensors()->Get(i)->dimensions()->Get(j)->size());
      printf("    serializationIndex  = %d\n", inParameter->networks()->Get(0)->outputTensors()->Get(i)->dimensions()->Get(j)->serializationIndex());
      printf("    padding             = %d\n", inParameter->networks()->Get(0)->outputTensors()->Get(i)->dimensions()->Get(j)->padding());
    }
    printf("   bitsPerElement = %d\n", inParameter->networks()->Get(0)->outputTensors()->Get(i)->bitsPerElement());
    printf("   shift          = %d\n", inParameter->networks()->Get(0)->outputTensors()->Get(i)->shift());
    printf("   scale          = %f\n", (double)inParameter->networks()->Get(0)->outputTensors()->Get(i)->scale());
    printf("   format         = %d\n", inParameter->networks()->Get(0)->outputTensors()->Get(i)->format());
  }
  printf("\n");
}

// -----------------------------------------------------------------------------
//  ExtractTensorData
// -----------------------------------------------------------------------------
void IMX501Utils::ExtractTensorData(const fpk_info *inInfo, const tensor_header *inHeader,
                                           const uint8_t *inChunkBuf, size_t inReceiveBufSize,
                                           uint8_t *inTensorBuf)
{
  size_t lineLen = (size_t)inInfo->dnn[0].dd_ch7_x;
  size_t lineNum = (size_t)inInfo->dnn[0].dd_ch7_y + (size_t)inInfo->dnn[0].dd_ch8_y;
  size_t dataWidth = inHeader->max_length_of_line;

  // sanity check
  if (inChunkBuf == NULL || inTensorBuf == NULL ||
      lineLen * lineNum > inReceiveBufSize ||
      dataWidth > lineLen)
  {
    throw std::runtime_error("ExtractTensorData was called with wrong parameters");
  }

  for (int i = 0; i < lineNum; i++)
  {
    memcpy(inTensorBuf, inChunkBuf, dataWidth);
    inChunkBuf += lineLen;
    inTensorBuf += dataWidth;
  }
}

// -----------------------------------------------------------------------------
//  ExtractTensors
// -----------------------------------------------------------------------------
void IMX501Utils::ExtractTensors(const fpk_info *inInfo, const tensor_header *inHeader,
                                        uint8_t *inTensorBuf, size_t inReceiveBufSize,
                                        uint8_t **outInputTensorPtr, size_t *outInputTensorsSize,
                                        uint8_t **outOutputTensorPtr, size_t *outOutputTensorsSize)
{
  size_t lineLen = (size_t)inInfo->dnn[0].dd_ch7_x;
  size_t lineNum = (size_t)inInfo->dnn[0].dd_ch7_y + (size_t)inInfo->dnn[0].dd_ch8_y;
  size_t dataWidth = inHeader->max_length_of_line;

  // sanity check
  if (inTensorBuf == NULL ||
      lineLen * lineNum > inReceiveBufSize ||
      dataWidth > lineLen)
  {
    throw std::runtime_error("ExtractTensors was called with wrong parameters");
  }

  // skip input tensor header
  *outInputTensorPtr    = &(inTensorBuf[dataWidth * 1]);
  *outInputTensorsSize   = dataWidth * ((size_t)inInfo->dnn[0].dd_ch7_y - 1);

  // skip output tensor header
  *outOutputTensorPtr   = &(inTensorBuf[dataWidth * ((size_t)inInfo->dnn[0].dd_ch7_y + 1)]);
  *outOutputTensorsSize  = dataWidth * ((size_t)inInfo->dnn[0].dd_ch8_y - 1);
}

// -----------------------------------------------------------------------------
//  ExtractInputImage
// -----------------------------------------------------------------------------
void IMX501Utils::ExtractInputImage(const fpk_info* inInfo,
                                    const apParams::fb::FBApParams *inParameter,
                                    const uint8_t *inInputTensorPtr, size_t inInputTensorSize,
                                    uint8_t *mInputImageBuf, size_t *outInputImageSize,
                                    int32_t *outInputImageWidth, int32_t *outInputImageHeight)
{
  // sanity check
  if (inInputTensorPtr == NULL)
    throw std::runtime_error("ExtractInputImage was called with wrong parameters");

  // format check
  if (inParameter->networks()->Get(0)->inputTensors()->Get(0)->numOfDimensions() != 3)
    throw std::runtime_error("Unknown input tensor format");

  // Note: we need to get the dimension from serializationIndex
  size_t width, height, x_padding, y_padding;
  for (int i = 0; i < 3; i++)
  {
    int index = inParameter->networks()->Get(0)->inputTensors()->Get(0)->dimensions()->Get(i)->serializationIndex();
    if (index == 0)
    {
      width = (size_t )inParameter->networks()->Get(0)->inputTensors()->Get(0)->dimensions()->Get(i)->size();
      x_padding = (size_t)inParameter->networks()->Get(0)->inputTensors()->Get(0)->dimensions()->Get(i)->padding();
    }
    if (index == 1)
    {
      height = (size_t)inParameter->networks()->Get(0)->inputTensors()->Get(0)->dimensions()->Get(i)->size();
      y_padding = (size_t)inParameter->networks()->Get(0)->inputTensors()->Get(0)->dimensions()->Get(i)->padding();
    }
  }

  // input tensor size sanity check
  size_t  inputTensorSize = (width + x_padding) * height * 3 + (width + x_padding) * y_padding * 2;
  if (inputTensorSize > inInputTensorSize)
  {
    printf("\n\nDEBUG: %zd, %zd, %zd, %zd\n", width, height, x_padding, y_padding);
    printf("DEBUG: inputTensorSize:%zd, inInputTensorSize:%zd\n", inputTensorSize, inInputTensorSize);
    throw std::runtime_error("InputTensor size mismatch");
  }

  *outInputImageSize = width * height * 3;
  const uint8_t *src = inInputTensorPtr;
  uint8_t *dst = mInputImageBuf;
  for (size_t i = 0; i < 3; i++)
  {
    for (size_t y = 0; y < height; y++)
    {
      for (size_t x = 0; x < width; x++)
      {
        uint8_t val = *src;
        if (inInfo->dnn[0].input_tensor_norm_k[0] == 0x0400 &&
            inInfo->dnn[0].input_tensor_norm_k[1] == 0x0000 &&
            inInfo->dnn[0].input_tensor_norm_k[2] == 0x1800)
        //if (inInfo->dnn[0].input_tensor_norm_yadd != 0)
        {
          if (val >= 0x80)
            val = val - 0x80;
          else
            val = val + 0x80;
        }
        //
        if (i == 0)
          dst[x * 3 + 2 + width * 3 * y] = val;
        if (i == 1)
          dst[x * 3 + 1 + width * 3 * y] = val;
        if (i == 2)
          dst[x * 3 + 0 + width * 3 * y] = val;
        src++;
      }
      if (x_padding != 0)
        src += x_padding;
    }
    if (y_padding != 0)
      src += (width * y_padding);
  }
  
  *outInputImageWidth  = (int32_t )width;
  *outInputImageHeight = (int32_t )height;
}

// -----------------------------------------------------------------------------
//  ReadFile
// -----------------------------------------------------------------------------
void IMX501Utils::ReadFile(GenApi::INodeMap *inNodeMap,
                              const char *inFileName,
                              void *inFileBuf, size_t inFileSize,
                              int inOffset)
{
  SetEnumNode(inNodeMap, "FileSelector", inFileName);
  SetEnumNode(inNodeMap, "FileOperationSelector", "Open");
  SetEnumNode(inNodeMap, "FileOpenMode", "Read");
  ExecuteNode(inNodeMap, "FileOperationExecute");

  SetEnumNode(inNodeMap, "FileOperationSelector", "Read");
  SetIntNode(inNodeMap, "FileAccessOffset", inOffset);
  SetIntNode(inNodeMap, "FileAccessLength", (int64_t )inFileSize);
  ExecuteNode(inNodeMap, "FileOperationExecute");

  GenApi::CRegisterPtr pRegister = inNodeMap->GetNode("FileAccessBuffer");
  pRegister->Get((uint8_t *)inFileBuf, inFileSize);

  SetEnumNode(inNodeMap, "FileOperationSelector", "Close");
  ExecuteNode(inNodeMap, "FileOperationExecute");
}

// -----------------------------------------------------------------------------
//  GetFileSize
// -----------------------------------------------------------------------------
int64_t IMX501Utils::GetFileSize(GenApi::INodeMap *inNodeMap,
                                  const char *inFileName)
{
  SetEnumNode(inNodeMap, "FileSelector", inFileName);
  return GetIntNode(inNodeMap, "FileSize");
}

// -----------------------------------------------------------------------------
//  WriteFile
// -----------------------------------------------------------------------------
void IMX501Utils::WriteFile(GenApi::INodeMap* inNodeMap,
  const char* inFileName,
  const void* inFileBuf, size_t inFileSize,
  int inOffset,
  int (*inProgressFunc)(size_t, size_t))
{
  SetEnumNode(inNodeMap, "FileSelector", inFileName);
  SetEnumNode(inNodeMap, "FileOperationSelector", "Open");
  SetEnumNode(inNodeMap, "FileOpenMode", "Write");
  ExecuteNode(inNodeMap, "FileOperationExecute");

  GenApi::CRegisterPtr pRegister = inNodeMap->GetNode("FileAccessBuffer");
  size_t buf_length = pRegister->GetLength();
  SetEnumNode(inNodeMap, "FileOperationSelector", "Write");

  size_t  remainingSize = inFileSize;
  size_t  offset = inOffset;
  const uint8_t* dataPtr = (uint8_t*)inFileBuf;
  while (remainingSize != 0)
  {
    size_t upload_size = remainingSize;
    if (upload_size > buf_length)
      upload_size = buf_length;

    SetIntNode(inNodeMap, "FileAccessOffset", offset);
    SetIntNode(inNodeMap, "FileAccessLength", (int64_t)upload_size);
    pRegister->Set(dataPtr, upload_size);
    ExecuteNode(inNodeMap, "FileOperationExecute");
    //
    remainingSize -= upload_size;
    offset += upload_size;
    dataPtr += upload_size;
    if (inProgressFunc != NULL)
    {
      int result;
      // result = 0 : no action
      // result = 1 : terminate the file write 
      result = inProgressFunc(inFileSize, inFileSize - remainingSize);
      if (result != 0)
        break;
    }
  }

  SetEnumNode(inNodeMap, "FileOperationSelector", "Close");
  ExecuteNode(inNodeMap, "FileOperationExecute");
}

// -----------------------------------------------------------------------------
//  SetEnumNode
// -----------------------------------------------------------------------------
void IMX501Utils::SetEnumNode(GenApi::INodeMap *inNodeMap,
                              const char *inNodeName, const char *inValue)
{
  GenApi::CEnumerationPtr pEnumeration = inNodeMap->GetNode(inNodeName);
  if (pEnumeration == NULL)
    throw std::runtime_error("Couldn't find the Enumeration node");
  GenApi::CEnumEntryPtr pEntry = pEnumeration->GetEntryByName(inValue);
  if (pEntry == NULL)
    throw std::runtime_error("Couldn't find the Enumeration entry");
  pEnumeration->SetIntValue(pEntry->GetValue());
}

// -----------------------------------------------------------------------------
//  SetFloatNode
// -----------------------------------------------------------------------------
void IMX501Utils::SetFloatNode(GenApi::INodeMap* inNodeMap,
                                const char* inNodeName, double inValue)
{
  GenApi::CFloatPtr pFloat = inNodeMap->GetNode(inNodeName);
  if (pFloat == NULL)
    throw std::runtime_error("Couldn't find the Float node");
  pFloat->SetValue(inValue);
}

// -----------------------------------------------------------------------------
//  SetBooleanNode
// -----------------------------------------------------------------------------
void IMX501Utils::SetBooleanNode(GenApi::INodeMap* inNodeMap,
                                  const char* inNodeName, bool inValue)
{
  GenApi::CBooleanPtr pBoolean = inNodeMap->GetNode(inNodeName);
  if (pBoolean == NULL)
    throw std::runtime_error("Couldn't find the Boolean node");
  pBoolean->SetValue(inValue);
}

// -----------------------------------------------------------------------------
//  SetIntNode
// -----------------------------------------------------------------------------
void IMX501Utils::SetIntNode(GenApi::INodeMap *inNodeMap,
                              const char *inNodeName, int64_t inValue)
{
  GenApi::CIntegerPtr pInteger = inNodeMap->GetNode(inNodeName);
  if (pInteger == NULL)
    throw std::runtime_error("Couldn't find the Integer node");
  pInteger->SetValue(inValue);
}

// -----------------------------------------------------------------------------
//  GetIntNode
// -----------------------------------------------------------------------------
int64_t IMX501Utils::GetIntNode(GenApi::INodeMap *inNodeMap,
                              const char *inNodeName)
{
  GenApi::CIntegerPtr pInteger = inNodeMap->GetNode(inNodeName);
  if (pInteger == NULL)
    throw std::runtime_error("Couldn't find the Integer node");
  return pInteger->GetValue();
}

// -----------------------------------------------------------------------------
//  ExecuteNode
// -----------------------------------------------------------------------------
void IMX501Utils::ExecuteNode(GenApi::INodeMap *inNodeMap,
                              const char *inNodeName)
{
  GenApi::CCommandPtr pCommand = inNodeMap->GetNode(inNodeName);
  if (pCommand == NULL)
    throw std::runtime_error("Couldn't find the Command node");
  pCommand->Execute();
}

// Namespace -------------------------------------------------------------------
};
