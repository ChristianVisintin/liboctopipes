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

#ifndef OCTOPIPES_MESSAGE_HPP
#define OCTOPIPES_MESSAGE_HPP

#include "typespp.hpp"

namespace octopipes {

class Message {

public:
  Message();
  Message(const ProtocolVersion version, const std::string& origin, const std::string& remote, const uint8_t* payload, const size_t payload_size, const Options options, const int ttl);
  ~Message();
  Error decodeData(const uint8_t* data, size_t data_size);
  Error encodeData(uint8_t*& data, size_t& data_size);
  ProtocolVersion getVersion();
  const std::string getOrigin();
  const std::string getRemote();
  const uint8_t* getPayload(size_t& data_size);
  const int getTTL();
  const int getChecksum();
  bool getOption(const Options option);

private:
  ProtocolVersion version;
  std::string origin;
  std::string remote;
  uint8_t* payload;
  size_t payload_size;
  int ttl;
  int checksum;
  Options options;

};

}

#endif
