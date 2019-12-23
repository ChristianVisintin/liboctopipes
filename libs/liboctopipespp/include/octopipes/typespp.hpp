/**
 *   Octopipes
 *   Developed by Christian Visintin
 * 
 * MIT License
 * Copyright (c) 2019-2020 Christian Visintin
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
**/

#ifndef OCTOPIPES_TYPESPP_HPP
#define OCTOPIPES_TYPESPP_HPP

#include <cinttypes>
#include <string>

namespace octopipes {

enum class Error {
  SUCCESS,
  UNINITIALIZED,
  BAD_PACKET,
  BAD_CHECKSUM,
  UNSUPPORTED_VERSION,
  NO_DATA_AVAILABLE,
  OPEN_FAILED,
  WRITE_FAILED,
  READ_FAILED,
  CAP_TIMEOUT,
  NOT_SUBSCRIBED,
  NOT_UNSUBSCRIBED,
  THREAD,
  BAD_ALLOC,
  UNKNOWN_ERROR
};

enum class CapMessage {
  UNKNOWN = 0x00,
  SUBSCRIBE = 0x01,
  ASSIGNMENT = 0xFF,
  UNSUBSCRIBE = 0x02
};

enum class CapError {
  ERROR_SUCCESS = 0,
  ERROR_NAME_ALREADY_TAKEN = 1,
  ERROR_FS = 2
};

enum class Options {
  NONE = 0,
  REQUIRE_ACK = 1,
  ACK = 2,
  IGNORE_CHECKSUM = 4
};

enum class ProtocolVersion {
  VERSION_1 = 1
};

}

#endif
