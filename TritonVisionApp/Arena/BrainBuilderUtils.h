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
#ifndef ARENA_EXAMPLE_BRAIN_BUILDER_UTILS_H
#define ARENA_EXAMPLE_BRAIN_BUILDER_UTILS_H

// Includes --------------------------------------------------------------------
#include <vector>
#include <iostream>
#include <string>
#include <exception>
#include <stdexcept>
#include "ClassificationUtils.h"

// Namespace -------------------------------------------------------------------
namespace ArenaExample {

// ><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
//  ArenaExample::BrainBuilderUtils class
// ><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
class BrainBuilderUtils : public ClassificationUtils
{
public:
  // Constructors and Destructor -----------------------------------------------
  // ---------------------------------------------------------------------------
  //  BrainBuilderUtils
  // ---------------------------------------------------------------------------
  BrainBuilderUtils(IMX501Utils *inIMX501Utils) :
    ClassificationUtils(inIMX501Utils)
  {
    mOutputTensorSize = 0;
    mOutputTensorScale = 0;
    mOutputTensor = NULL;
  }
  // ---------------------------------------------------------------------------
  //  ~BrainBuilderUtils
  // ---------------------------------------------------------------------------
  virtual ~BrainBuilderUtils()
  {
  }

  // Member functions ----------------------------------------------------------
  // ---------------------------------------------------------------------------
  //  ProcessOutputTensor
  // ---------------------------------------------------------------------------
  bool  ProcessOutputTensor()
  {
    mOutputTensorSize = 0;
    mOutputTensorScale = 0;
    mOutputTensor = NULL;

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
    mOutputTensorSize = mIMX501Utils->GetOutputTensorSize(0);
    mOutputTensorScale = mIMX501Utils->GetOutputTensorScale(0);
    mOutputTensor = (const uint8_t*)mIMX501Utils->GetOutputTensorPtr(0);

    return true;
  }
  // ---------------------------------------------------------------------------
  //  DumpOutputTensor
  // ---------------------------------------------------------------------------
  void  DumpOutputTensor()
  {
    if (mOutputTensor == NULL)
    {
      printf("Error: mOutputTensor == NULL\n");
      return;
    }
    printf("[BrainBuilder]\n");
    printf("num = %zd\n", mOutputTensorSize);
    for (size_t i = 0; i < mOutputTensorSize; i++)
    {
      double score;
      score = ScaleValue(mOutputTensor[i], mOutputTensorScale, 0, 1.0);
      printf("%zd: %f\n", i, score);
    }
    printf("\n");
  }
  // ---------------------------------------------------------------------------
  //  GetClassNum
  // ---------------------------------------------------------------------------
  size_t GetClassNum()
  {
    return mOutputTensorSize;
  }
  // ---------------------------------------------------------------------------
  //  GetClassScore
  // ---------------------------------------------------------------------------
  bool  GetClassScore(size_t inIndex, double *outScore)
  {
    if (mOutputTensor == NULL)
    {
      if (mIMX501Utils->IsVerboseMode())
        printf("Error: mOutputTensor == NULL\n");
      return false;
    }
    if (inIndex >= mOutputTensorSize)
    {
      if (mIMX501Utils->IsVerboseMode())
        printf("Error: inIndex >= mOutputTensorSize\n");
      return false;
    }

    *outScore = ScaleValue(mOutputTensor[inIndex], mOutputTensorScale, 0, 1.0);
    return true;
  }
  // ---------------------------------------------------------------------------
  //  GetClassWithHighestScore
  // ---------------------------------------------------------------------------
  bool  GetClassWithHighestScore(size_t *outIndex, double *outScore)
  {
    if (mOutputTensor == NULL)
    {
      if (mIMX501Utils->IsVerboseMode())
        printf("Error: mOutputTensor == NULL\n");
      return false;
    }
    if (mOutputTensorSize == 0)
    {
      if (mIMX501Utils->IsVerboseMode())
        printf("Error: mResultNum == 0\n");
      return false;
    }

    uint8_t max = 0;
    *outIndex = 0;
    for (size_t i = 0; i < mOutputTensorSize; i++)
    {
      if (mOutputTensor[i] >= max)
      {
        max = mOutputTensor[i];
        *outIndex = i;
      }
    }
    *outScore = ScaleValue(mOutputTensor[*outIndex],
                            mOutputTensorScale, 0, 1.0);
    return true;
  }

protected:
  // Member variables ----------------------------------------------------------
  size_t  mOutputTensorSize;
  double  mOutputTensorScale;
  const uint8_t   *mOutputTensor;
};

// Namespace -------------------------------------------------------------------
}
#endif //ARENA_EXAMPLE_BRAIN_BUILDER_UTILS_H
