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

#include <octopipes/message.hpp>
#include <octopipes/octopipes.h>
#include <octopipes/serializer.h>

#include <cstring>

namespace octopipes {

Error translate_octopipes_error(OctopipesError error);

/**
 * @brief Message default class constructor
 */

Message::Message() {
  this->payload = nullptr;
  this->payload_size = 0;
  this->checksum = 0;
  this->ttl = 0;
  this->options = Options::NONE;
}

/**
 * @brief Message class constructor
 * @param ProtocolVersion version
 * @param string origin
 * @param string remote
 * @param uint8_t* payload
 * @param size_t payload size
 * @param Options options
 * @param int ttl
 * @throw std::bad_alloc
 * NOTE: data is copied to message, so the user must free the payload himself
 */

Message::Message(const ProtocolVersion version, const std::string& origin, const std::string& remote, const uint8_t* payload, const size_t payload_size, const Options options, const int ttl) {
  this->version = version;
  this->origin = origin;
  this->remote = remote;
  this->payload_size = payload_size;
  if (this->payload_size > 0) {
    this->payload = new uint8_t[payload_size];
    memcpy(this->payload, payload, payload_size);
  }
  this->ttl = ttl;
  this->options = options;
}

/**
 * @brief Message class destructor
 */

Message::~Message() {
  if (this->payload != nullptr) {
    delete[] this->payload;
  }
}

/**
 * @brief decode data into Message class attributes
 * @param uint8_t* data
 * @param size_t data size
 * @return octopipes::Error
 * @throw std::bad_alloc
 */

Error Message::decodeData(const uint8_t* data, size_t data_size) {
  OctopipesMessage* msg;
  OctopipesError rc;
  if ((rc = octopipes_decode(data, data_size, &msg)) != OCTOPIPES_ERROR_SUCCESS) {
    return translate_octopipes_error(rc);
  }
  this->origin = msg->origin;
  this->remote = msg->remote;
  this->checksum = msg->checksum;
  this->options = static_cast<Options>(msg->options);
  this->ttl = msg->ttl;
  this->payload_size = msg->data_size;
  if (this->payload_size > 0) {
    this->payload = new uint8_t[payload_size];
    memcpy(this->payload, msg->data, this->payload_size);
  }
  octopipes_cleanup_message(msg);
  return Error::SUCCESS;
}

/**
 * @brief encode Message instance to message buffer
 * @param uint8_t*& data
 * @param size_t& data_size
 * @return octopipes::Error
 * @throw std::bad_alloc
 */

Error Message::encodeData(uint8_t*& data, size_t& data_size) {
  //Fill message attributes; don't use new, since we're going to use cleanup message then. Don't ever mix C with C++!
  OctopipesMessage* msg = (OctopipesMessage*) malloc(sizeof(OctopipesMessage));
  if (msg == NULL) {
    return Error::BAD_ALLOC;
  }
  //Version
  msg->version = static_cast<OctopipesVersion>(version);
  //Origin
  msg->origin_size = origin.length();
  msg->origin = (char*) malloc(sizeof(char) * (msg->origin_size + 1));
  if (msg->origin == NULL) {
    octopipes_cleanup_message(msg);
    return Error::BAD_ALLOC;
  }
  memcpy(msg->origin, origin.c_str(), msg->origin_size);
  msg->origin[msg->origin_size] = 0x00;
  //Remote
  msg->remote_size = remote.length();
  msg->remote = (char*) malloc(sizeof(char) * (msg->remote_size + 1));
  if (msg->remote == NULL) {
    octopipes_cleanup_message(msg);
    return Error::BAD_ALLOC;
  }
  memcpy(msg->remote, remote.c_str(), msg->remote_size);
  msg->remote[msg->remote_size] = 0x00;
  //Options
  msg->options = static_cast<OctopipesOptions>(options);
  //TTL
  msg->ttl = ttl;
  //Assign data
  msg->data_size = this->payload_size;
  msg->data = (uint8_t*) malloc(sizeof(uint8_t) * msg->data_size);
  if (msg->data == NULL) {
    octopipes_cleanup_message(msg);
    return Error::BAD_ALLOC;
  }
  memcpy(msg->data, this->payload, msg->data_size);
  //If checksum has to be calculated, calc checksum
  if (!getOption(Options::IGNORE_CHECKSUM)) {
    msg->checksum = calculate_checksum(msg);
  }
  //Encode message
  uint8_t* out_data;
  size_t out_data_size;
  OctopipesError rc;
  if ((rc = octopipes_encode(msg, &out_data, &out_data_size)) != OCTOPIPES_ERROR_SUCCESS) {
    octopipes_cleanup_message(msg);
    return translate_octopipes_error(rc);
  }
  //Assign data
  data = out_data;
  data_size = out_data_size;
  octopipes_cleanup_message(msg);
  return Error::SUCCESS;
}

/**
 * @brief return the Message protocol version
 * @return ProtocolVersion
 */

ProtocolVersion Message::getVersion() {
  return version;
}

/**
 * @brief return the Message origin
 * @return string
 */

const std::string Message::getOrigin() {
  return origin;
}

/**
 * @brief return the Message remote
 * @return string
 */

const std::string Message::getRemote() {
  return remote;
}

/**
 * @brief return the Message payload and its size
 * @param size_t& data_size
 * @return uint8_t*
 */

const uint8_t* Message::getPayload(size_t& data_size) {
  data_size = this->payload_size;
  return payload;
}

/**
 * @brief return the Message TTL
 * @return int
 */

const int Message::getTTL() {
  return ttl;
}

/**
 * @brief return the Message checksum
 * @return int
 */

const int Message::getChecksum() {
  return checksum;
}

/**
 * @brief return the desidered Option value from Message options
 * @param Options desired option get get
 * @return bool
 */

bool Message::getOption(const Options option) {
  return (static_cast<uint8_t>(this->options) & static_cast<uint8_t>(option)) != 0;
}

/**
 * @brief get Octopipes++ error from OctopipesError
 * @param OctopipesError
 * @return octopipes::Error
 */

Error translate_octopipes_error(OctopipesError error) {
  switch (error) {
    case OCTOPIPES_ERROR_BAD_ALLOC:
      return Error::BAD_ALLOC;
    case OCTOPIPES_ERROR_BAD_CHECKSUM:
      return Error::BAD_CHECKSUM;
    case OCTOPIPES_ERROR_BAD_PACKET:
      return Error::BAD_PACKET;
    case OCTOPIPES_ERROR_CAP_TIMEOUT:
      return Error::CAP_TIMEOUT;
    case OCTOPIPES_ERROR_NO_DATA_AVAILABLE:
      return Error::NO_DATA_AVAILABLE;
    case OCTOPIPES_ERROR_NOT_SUBSCRIBED:
      return Error::NOT_SUBSCRIBED;
    case OCTOPIPES_ERROR_NOT_UNSUBSCRIBED:
      return Error::NOT_UNSUBSCRIBED;
    case OCTOPIPES_ERROR_OPEN_FAILED:
      return Error::OPEN_FAILED;
    case OCTOPIPES_ERROR_READ_FAILED:
      return Error::READ_FAILED;
    case OCTOPIPES_ERROR_THREAD:
      return Error::THREAD;
    case OCTOPIPES_ERROR_UNINITIALIZED:
      return Error::UNINITIALIZED;
    case OCTOPIPES_ERROR_UNKNOWN_ERROR:
    default:
      return Error::UNKNOWN_ERROR;
  }
}

}
