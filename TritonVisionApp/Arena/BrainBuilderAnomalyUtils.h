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
#ifndef ARENA_EXAMPLE_BRAIN_BUILDEDR_ANOMALY_UTILS_H
#define ARENA_EXAMPLE_BRAIN_BUILDEDR_ANOMALY_UTILS_H

// Includes --------------------------------------------------------------------
#include <vector>
#include <iostream>
#include <string>
#include <exception>
#include <stdexcept>
#include "OutputTensorUtils.h"

// Namespace -------------------------------------------------------------------
namespace ArenaExample {

// ><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
//  ArenaExample::BrainBuilderAnomalyUtils class
// ><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
class BrainBuilderAnomalyUtils : public OutputTensorUtils
{
public:
  // Constructors and Destructor -----------------------------------------------
  // ---------------------------------------------------------------------------
  //  BrainBuilderAnomalyUtils
  // ---------------------------------------------------------------------------
  BrainBuilderAnomalyUtils(IMX501Utils* inIMX501Utils,
                            double inImageThreshold = 0,
                            double inPixelThreshold = 0) :
    OutputTensorUtils(inIMX501Utils)
  {
    mHeatmapWidth = 0;
    mHeatmapHeight = 0;
    mHeatmap = NULL;
    //
    mAnomalyScore   = 0.0;
    //
    SetImageThreshold(inImageThreshold);
    SetPixelThreshold(inPixelThreshold);
  }
  // ---------------------------------------------------------------------------
  //  ~BrainBuilderAnomalyUtils
  // ---------------------------------------------------------------------------
  virtual ~BrainBuilderAnomalyUtils()
  {
    if (mHeatmap != NULL)
      delete[] mHeatmap;
  }

  // Member functions ----------------------------------------------------------
  // ---------------------------------------------------------------------------
  //  ProcessOutputTensor
  // ---------------------------------------------------------------------------
  bool  ProcessOutputTensor()
  {
    // Validate Output Tensors
    if (mIMX501Utils->IsDataExtracted() == false)
    {
      if (mIMX501Utils->IsVerboseMode())
        printf("Error: mIMX501Utils->IsDataExtracted() == false\n");
      return false;
    }
    if (mIMX501Utils->GetOutputTensorNum() != 1)
    {
      if (mIMX501Utils->IsVerboseMode())
        printf("Error: mIMX501Utils->GetOutputTensorNum() != 1\n");
      return false;
    }

    IMX501Utils::output_tensor_info ountInfo;
    if (mIMX501Utils->GetOutputTensorInfo(0, &ountInfo) == false)
    {
      if (mIMX501Utils->IsVerboseMode())
        printf("Error: mIMX501Utils->GetOutputTensorInfo() == false\n");
      return false;
    }
    if (ountInfo.numOfDimensions != 3)
    {
      if (mIMX501Utils->IsVerboseMode())
        printf("Error: ountInfo.numOfDimensions != 3\n");
      return false;
    }
    int width = 0, height = 0, data_num = 0;
    for (int i = 0; i < 3; i++)
    {
      IMX501Utils::dimension_info dimension_info;
      if (mIMX501Utils->GetOutputTensorDimensionInfo(0, i, &dimension_info) == false)
      {
        if (mIMX501Utils->IsVerboseMode())
          printf("Error: mIMX501Utils->GetOutputTensorDimensionInfo() == false\n");
        return false;
      }
      if (dimension_info.padding != 0)
      {
        if (mIMX501Utils->IsVerboseMode())
          printf("Error: dimension_info.padding != 0 (%d)\n", i);
        return false;
      }
      if (dimension_info.serializationIndex == 0)
        width = dimension_info.size;
      if (dimension_info.serializationIndex == 1)
        height = dimension_info.size;
      if (dimension_info.serializationIndex == 2)
        data_num = dimension_info.size;
    }
    if (data_num != 2)
    {
      if (mIMX501Utils->IsVerboseMode())
        printf("Error: data_num != 2\n");
      return false;
    }

    // Allocate buffers
    if (mHeatmapWidth != width || mHeatmapHeight != height)
    {
      mHeatmapWidth   = width;
      mHeatmapHeight  = height;
      size_t  bufSize = mHeatmapWidth * mHeatmapHeight;
      //
      if (mHeatmap != NULL)
        delete[] mHeatmap;
      mHeatmap = new uint8_t[bufSize];
      if (mHeatmap == NULL)
      {
        if (mIMX501Utils->IsVerboseMode())
          printf("Error: data_num != 2\n");
        return false;
      }
    }

    const uint8_t *src = (const uint8_t*)mIMX501Utils->GetOutputTensorPtr(0);
    uint8_t *dst = mHeatmap;
    for (size_t x = 0; x < mHeatmapWidth; x++)
    {
      for (size_t y = 0; y < mHeatmapHeight; y++)
      {
        uint8_t v = *src;
        dst[x + y * mHeatmapWidth] = v;
        src++;
      }
    }
    mAnomalyScore = ScaleValue(*src, 0.003921569, 0, 1.0);
    return true;
  }
  // ---------------------------------------------------------------------------
  //  DumpOutputTensor
  // ---------------------------------------------------------------------------
  void  DumpOutputTensor()
  {
    if (mHeatmap == NULL)
    {
      printf("Error: mHeatmap == NULL\n");
      return;
    }
    printf("[BrainBuilder Anormaly]\n");
    printf("Anomaly Score: %f ", mAnomalyScore);
    if (IsNormal())
      printf(" = Normal  (<  %f)\n", mImageThreshold);
    else
      printf(" = Anomaly (>= %f)\n", mImageThreshold);
    printf("\n");
  }
  // ---------------------------------------------------------------------------
  //  GetHeatmapWidth
  // ---------------------------------------------------------------------------
  uint32_t GetHeatmapWidth()
  {
    return mHeatmapWidth;
  }
  // ---------------------------------------------------------------------------
  //  GetHeatmapHeight
  // ---------------------------------------------------------------------------
  uint32_t GetHeatmapHeight()
  {
    return mHeatmapHeight;
  }
  // ---------------------------------------------------------------------------
  //  GetHeatmapPtr
  // ---------------------------------------------------------------------------
  const uint8_t *GetHeatmapPtr()
  {
    return mHeatmap;
  }
  // ---------------------------------------------------------------------------
  //  GetAnomalyScore
  // ---------------------------------------------------------------------------
  double GetAnomalyScore()
  {
    return mAnomalyScore;
  }
  // ---------------------------------------------------------------------------
  //  GetImageThreshold
  // ---------------------------------------------------------------------------
  double GetImageThreshold()
  {
    return mImageThreshold;
  }
  // ---------------------------------------------------------------------------
  //  SetImageThreshold
  // ---------------------------------------------------------------------------
  void SetImageThreshold(double inThreshold)
  {
    mImageThreshold = LimitThreshold(inThreshold);
  }
  // ---------------------------------------------------------------------------
  //  IsNormal
  // ---------------------------------------------------------------------------
  bool IsNormal()
  {
    if (mAnomalyScore < mImageThreshold)
      return true;
    return false;
  }
  // ---------------------------------------------------------------------------
  //  GetLabelIndexAndScore
  // ---------------------------------------------------------------------------
  bool  GetLabelIndexAndScore(size_t *outIndex, double *outScore)
  {
    if (mHeatmap == NULL)
    {
      if (mIMX501Utils->IsVerboseMode())
        printf("Error: mHeatmap == NULL\n");
      return false;
    }
    if (IsNormal())
      *outIndex = 0;
    else
      *outIndex = 1;
    *outScore = GetAnomalyScore();
    return true;
  }
  // ---------------------------------------------------------------------------
  // ---------------------------------------------------------------------------
  //  GetPixelThreshold
  // ---------------------------------------------------------------------------
  double GetPixelThreshold()
  {
    return mPixelThreshold;
  }
  // ---------------------------------------------------------------------------
  //  SetPixelThreshold
  // ---------------------------------------------------------------------------
  void SetPixelThreshold(double inThreshold)
  {
    mPixelThreshold = LimitThreshold(inThreshold);
  }
  // ---------------------------------------------------------------------------
  //  MakeMaskedImage
  // ---------------------------------------------------------------------------
  bool MakeMaskedImage(int inImageWidth, int inImageHeight,
                       const uint8_t *inSrcImage,
                       uint8_t *ioMaskedImage,
                       uint32_t inMaskColor = 0xFF0000,
                       double inMaskAlpha = 0.5)
  {
    if (mHeatmap == NULL)
    {
      if (mIMX501Utils->IsVerboseMode())
        printf("Error: mHeatmap == NULL\n");
      return false;
    }
    if (inImageWidth == 0 || inImageHeight == 0 ||
        inSrcImage == NULL || ioMaskedImage == NULL)
    {
      if (mIMX501Utils->IsVerboseMode())
        printf("Error: Invalid parameters\n");
      return false;
    }

    uint8_t maskColor[3];
    maskColor[0] = (inMaskColor >> 0) & 0xFF;
    maskColor[1] = (inMaskColor >> 8) & 0xFF;
    maskColor[2] = (inMaskColor >> 16) & 0xFF;

    uint8_t pixelThreshold = (uint8_t)(mPixelThreshold * 255.0);
    double scaleX = (double )mHeatmapWidth  / (double )inImageWidth;
    double scaleY = (double )mHeatmapHeight / (double )inImageHeight;
    const uint8_t *src = inSrcImage;
    uint8_t *dst = ioMaskedImage;
    for (int y = 0; y < inImageHeight; y++)
    {
      size_t heatY = (size_t)((double)y * scaleY);
      if (heatY > mHeatmapHeight - 1)
        heatY = mHeatmapHeight - 1;
      for (int x = 0; x < inImageWidth; x++)
      {
        size_t heatX = (size_t)((double)x * scaleX);
        if (heatX > mHeatmapWidth - 1)
          heatX = mHeatmapWidth - 1;

        uint8_t heat = mHeatmap[heatX + heatY * mHeatmapWidth];
        for (int i = 0; i < 3; i++)
        {
          if (heat < pixelThreshold)
          {
            *dst = *src;
          }
          else
          {
            double v;
            v  = (double )*src * (1.0 - inMaskAlpha);
            v += (double )maskColor[i] * inMaskAlpha;
            if (v > 255.0)
              v = 255.0;
            *dst = (uint8_t )v;
          }
          src ++;
          dst ++;
        }
      }
    }
    return true;
  }

protected:
  // Member variables ----------------------------------------------------------
  uint32_t mHeatmapWidth, mHeatmapHeight;
  uint8_t *mHeatmap;

  double mAnomalyScore;
  double mImageThreshold;
  double mPixelThreshold;

  // -----------------------------------------------------------------------------
  //  LimitThreshold
  // -----------------------------------------------------------------------------
  double LimitThreshold(double inThreshold)
  {
    if (inThreshold < 0.0)
      return 0.0;
    if (inThreshold > 1.0)
      return 1.0;
    return inThreshold;
  }
};

  // Namespace -------------------------------------------------------------------
}
#endif //ARENA_EXAMPLE_BRAIN_BUILDEDR_ANOMALY_UTILS_H
