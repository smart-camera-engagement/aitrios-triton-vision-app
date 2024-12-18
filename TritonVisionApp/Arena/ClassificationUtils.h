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
#ifndef ARENA_EXAMPLE_CLASSIFICATION_UTILS_H
#define ARENA_EXAMPLE_CLASSIFICATION_UTILS_H

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
//  ArenaExample::ClassificationUtils class
// ><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
class ClassificationUtils : public OutputTensorUtils
{
public:
  // Constructors and Destructor -----------------------------------------------
  // ---------------------------------------------------------------------------
  //  ClassificationUtils
  // ---------------------------------------------------------------------------
  ClassificationUtils(IMX501Utils *inIMX501Utils) :
    OutputTensorUtils(inIMX501Utils)
  {
  }
  // ---------------------------------------------------------------------------
  //  ~ClassificationUtils
  // ---------------------------------------------------------------------------
  virtual ~ClassificationUtils()
  {
  }

  // Member functions ----------------------------------------------------------
  virtual size_t  GetClassNum() = 0;
  virtual bool    GetClassScore(size_t inIndex, double *outScore) = 0;
  virtual bool    GetClassWithHighestScore(size_t *outIndex, double *outScore) = 0;
};

// Namespace -------------------------------------------------------------------
}
#endif //ARENA_EXAMPLE_CLASSIFICATION_UTILS_H
