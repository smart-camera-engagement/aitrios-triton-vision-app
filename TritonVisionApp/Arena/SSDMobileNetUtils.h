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
#ifndef ARENA_EXAMPLE_SSD_MOBILENET_UTILS_H
#define ARENA_EXAMPLE_SSD_MOBILENET_UTILS_H

// Includes --------------------------------------------------------------------
#include <vector>
#include <iostream>
#include <string>
#include <exception>
#include <stdexcept>
#include "ObjectDetectionUtils.h"

// Namespace -------------------------------------------------------------------
namespace ArenaExample {

// ><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
//  ArenaExample::SSDMobileNetUtils class
// ><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
class SSDMobileNetUtils : public ObjectDetectionUtils
{
public:
  // Constants -----------------------------------------------------------------
    const int  kResultNum          = 10;
    const int  kClassNum           = 100;
    const size_t  kOutputTensor0Size = 2 * 4 * (size_t)kResultNum;  // int16_t [4][kResultNum]
    const size_t  kOutputTensor1Size = 2 * (size_t)kResultNum;      // int16_t [kResultNum]
    const size_t  kOutputTensor2Size = 1 * (size_t)kResultNum;      // uint8_t [kResultNum]
    const size_t  kOutputTensor3Size   = 2;           // int16_t [1]

  // Constructors and Destructor -----------------------------------------------
  // ---------------------------------------------------------------------------
  //  SSDMobileNetUtils
  // ---------------------------------------------------------------------------
  SSDMobileNetUtils(IMX501Utils *inIMX501Utils) :
    ObjectDetectionUtils(inIMX501Utils)
  {
    mResultNum = 0;
    mThreshold = 0.0;

    mResult = new object_info[kResultNum];
    if (mResult == NULL)
      return;
    memset(mResult, 0, sizeof(object_info) * kResultNum);
  }
  // ---------------------------------------------------------------------------
  //  ~SSDMobileNetUtils
  // ---------------------------------------------------------------------------
  virtual ~SSDMobileNetUtils()
  {
  }

  // Member functions ----------------------------------------------------------
  // ---------------------------------------------------------------------------
  //  ProcessOutputTensor
  // ---------------------------------------------------------------------------
  bool  ProcessOutputTensor()
  {
    if (mResult == NULL)
      return false;

    mResultNum = 0;
    memset(mResult, 0, sizeof(mResult));

    // Validate Output Tensors
    if (mIMX501Utils->IsDataExtracted() == false)
    {
      if (mIMX501Utils->IsVerboseMode())
        printf("Error: mIMX501Utils->IsDataExtracted() == false\n");
      return false;
    }
    if (mIMX501Utils->GetOutputTensorNum() != 4)
    {
      if (mIMX501Utils->IsVerboseMode())
        printf("Error: mIMX501Utils->GetOutputTensorNum() != 4\n");
      return false;
    }
    size_t outputTensor0Size = mIMX501Utils->GetOutputTensorSize(0);
    size_t outputTensor1Size = mIMX501Utils->GetOutputTensorSize(1);
    size_t outputTensor2Size = mIMX501Utils->GetOutputTensorSize(2);
    size_t outputTensor3Size = mIMX501Utils->GetOutputTensorSize(3);
    if (outputTensor0Size != kOutputTensor0Size ||
        outputTensor1Size != kOutputTensor1Size ||
        outputTensor2Size != kOutputTensor2Size ||
        outputTensor3Size != kOutputTensor3Size)
    {
      if (mIMX501Utils->IsVerboseMode())
      {
        printf("Error: outputTensor0Size = %zd (%zd)\n", outputTensor0Size, kOutputTensor0Size);
        printf("Error: outputTensor1Size = %zd (%zd)\n", outputTensor1Size, kOutputTensor1Size);
        printf("Error: outputTensor2Size = %zd (%zd)\n", outputTensor2Size, kOutputTensor2Size);
        printf("Error: outputTensor3Size = %zd (%zd)\n", outputTensor3Size, kOutputTensor3Size);
      }
      return false;
    }

    int width = mIMX501Utils->GetInputImageWidth();
    int height = mIMX501Utils->GetInputImageHeight();
    double scale0 = mIMX501Utils->GetOutputTensorScale(0);
    double scale2 = mIMX501Utils->GetOutputTensorScale(2);
    const int16_t *tensor0 = (const int16_t*)mIMX501Utils->GetOutputTensorPtr(0);
    const int16_t* tensor1 = (const int16_t*)mIMX501Utils->GetOutputTensorPtr(1);
    const uint8_t* tensor2 = (const uint8_t*)mIMX501Utils->GetOutputTensorPtr(2);
    const int16_t* tensor3 = (const int16_t*)mIMX501Utils->GetOutputTensorPtr(3);

    for (int i = 0; i < kResultNum; i++)
    {
      mResult[i].location.left    = ScaleValue(tensor0[i + kResultNum * 1], scale0, 0, 1.0);
      mResult[i].location.top     = ScaleValue(tensor0[i + kResultNum * 0], scale0, 0, 1.0);
      mResult[i].location.right   = ScaleValue(tensor0[i + kResultNum * 3], scale0, 0, 1.0);
      mResult[i].location.bottom  = ScaleValue(tensor0[i + kResultNum * 2], scale0, 0, 1.0);
      mResult[i].index  = ClipValue(tensor1[i], 0, kClassNum);
      mResult[i].score  = ScaleValue(tensor2[i], scale2, 0, 1.0);
    }
    mResultNum = ClipValue(tensor3[0], 0, kResultNum);

    return true;
  }
  // ---------------------------------------------------------------------------
  //  DumpOutputTensor
  // ---------------------------------------------------------------------------
  void  DumpOutputTensor()
  {
    if (mResult == NULL)
      return;

    printf("[SSD MobileNet]\n");
    printf("num = %zd\n", mResultNum);
    for (size_t i = 0; i < mResultNum; i++)
    {
      rect_uint32 rect;
      rect = ToInputImageRect(mResult[i].location);
      printf("(%d,%d)-(%d-%d), %03d, %f\n",
              rect.left, rect.top,
              rect.right, rect.bottom,
              mResult[i].index,
              mResult[i].score);
    }
    printf("\n");
  }
  // ---------------------------------------------------------------------------
  //  GetObjectNum
  // ---------------------------------------------------------------------------
  int GetObjectNum(double inThreshold)
  {
    if (mResult == NULL)
      return 0;

    int num = 0;
    for (size_t i = 0; i < mResultNum; i++)
      if (mResult[i].score >= inThreshold)
        num++;
    return num;
  }
  // ---------------------------------------------------------------------------
  //  GetObjectInfo
  // ---------------------------------------------------------------------------
  bool  GetObjectInfo(size_t inIndex, object_info *outInfo)
  {
    if (mResult == NULL)
      return false;

    if (inIndex >= mResultNum)
    {
      if (mIMX501Utils->IsVerboseMode())
        printf("Error: inIndex >= mResultNum\n");
      return false;
    }
    *outInfo = mResult[inIndex];
    return true;
  }


protected:
  // Member variables ----------------------------------------------------------
  size_t      mResultNum;
  object_info *mResult;
  double      mThreshold;
};

// Namespace -------------------------------------------------------------------
}
#endif //ARENA_EXAMPLE_SSD_MOBILENET_UTILS_H
