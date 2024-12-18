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
#ifndef ARENA_EXAMPLE_OBJECT_DETECTION_UTILS_H
#define ARENA_EXAMPLE_OBJECT_DETECTION_UTILS_H

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
//  ArenaExample::ObjectDetectionUtils class
// ><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
class ObjectDetectionUtils : public OutputTensorUtils
{
public:
  // typedefs ------------------------------------------------------------------
  typedef struct
  {
    double            left;     // range: 0...1.0
    double            top;      // range: 0...1.0
    double            right;    // range: 0...1.0
    double            bottom;   // range: 0...1.0
  } bounding_box;

  typedef struct
  {
    uint32_t          left;
    uint32_t          top;
    uint32_t          right;
    uint32_t          bottom;
  } rect_uint32;

  typedef struct
  {
    bounding_box      location;
    //
    int               index;
    double            score;
  } object_info;

  // Constructors and Destructor -----------------------------------------------
  // ---------------------------------------------------------------------------
  //  ObjectDetectionUtils
  // ---------------------------------------------------------------------------
  ObjectDetectionUtils(IMX501Utils *inIMX501Utils) :
    OutputTensorUtils(inIMX501Utils)
  {
  }
  // ---------------------------------------------------------------------------
  //  ~ObjectDetectionUtils
  // ---------------------------------------------------------------------------
  virtual ~ObjectDetectionUtils()
  {
  }

  // Member functions ----------------------------------------------------------
  virtual int   GetObjectNum(double inThreshold) = 0;
  virtual bool  GetObjectInfo(size_t inIndex, object_info *outInfo) = 0;

  // ---------------------------------------------------------------------------
  //  ToRect_uint32
  // ---------------------------------------------------------------------------
  rect_uint32 ToRect_uint32(const bounding_box &inBox,
                            uint32_t inWidth, uint32_t inHeight)
  {
    rect_uint32 rect;
    rect.left   = (uint32_t)ScaleValue(inBox.left,   inWidth,  0, inWidth);
    rect.top    = (uint32_t)ScaleValue(inBox.top,    inHeight, 0, inHeight);
    rect.right  = (uint32_t)ScaleValue(inBox.right,  inWidth,  0, inWidth);
    rect.bottom = (uint32_t)ScaleValue(inBox.bottom, inHeight, 0, inHeight);
    //
    if (rect.left > rect.right)
      rect.left = rect.right;
    if (rect.top > rect.bottom)
      rect.top = rect.bottom;
    //
    return rect;
  }
  // ---------------------------------------------------------------------------
  //  ToInputImageRect
  // ---------------------------------------------------------------------------
  rect_uint32 ToInputImageRect(const bounding_box &inBox)
  {
    return ToRect_uint32( inBox,
                          mIMX501Utils->GetInputImageWidth(),
                          mIMX501Utils->GetInputImageHeight());
  }
};

// Namespace -------------------------------------------------------------------
}
#endif //ARENA_EXAMPLE_OBJECT_DETECTION_UTILS_H
