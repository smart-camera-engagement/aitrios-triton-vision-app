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
#ifndef ARENA_EXAMPLE_OUTPUT_TENSOR_UTILS_H
#define ARENA_EXAMPLE_OUTPUT_TENSOR_UTILS_H

// Includes --------------------------------------------------------------------
#include <vector>
#include <iostream>
#include <string>
#include <exception>
#include <stdexcept>
#include "IMX501Utils.h"

// Namespace -------------------------------------------------------------------
namespace ArenaExample {

// ><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
//  ArenaExample::OutputTensorUtils class
// ><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
class OutputTensorUtils
{
public:
  // Constructors and Destructor -----------------------------------------------
  // ---------------------------------------------------------------------------
  //  OutputTensorUtils
  // ---------------------------------------------------------------------------
  OutputTensorUtils(IMX501Utils *inIMX501Utils)
  {
    mIMX501Utils = inIMX501Utils;
  }
  // ---------------------------------------------------------------------------
  //  ~OutputTensorUtils
  // ---------------------------------------------------------------------------
  virtual ~OutputTensorUtils()
  {
  }

  // Member functions ----------------------------------------------------------
  virtual bool  ProcessOutputTensor() = 0;
  virtual void  DumpOutputTensor() = 0;

protected:
  // Member variables ----------------------------------------------------------
  IMX501Utils *mIMX501Utils;

  // Static functions ----------------------------------------------------------
  // ---------------------------------------------------------------------------
  //  ScaleValue
  // ---------------------------------------------------------------------------
  static double ScaleValue(double inValue, double inScale, double inMin, double inMax)
  {
    double t;
    t = inValue * inScale;
    if (t < inMin)
      t = inMin;
    if (t > inMax)
      t = inMax;
    return t;
  }
  // ---------------------------------------------------------------------------
  //  ClipValue
  // ---------------------------------------------------------------------------
  static int32_t ClipValue(int32_t inValue, int32_t inMin, int32_t inMax)
  {
    if (inValue < inMin)
      inValue = inMin;
    if (inValue > inMax)
      inValue = inMax;
    return inValue;
  }
};

// Namespace -------------------------------------------------------------------
}
#endif //ARENA_EXAMPLE_OUTPUT_TENSOR_UTILS_H
