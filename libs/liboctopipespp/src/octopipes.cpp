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

#include <octopipes/octopipespp.hpp>

#include <octopipes/octopipes.h>

#define DEFAULT_TTL 0

namespace octopipes {
  
//Local functions
Error translate_octopipes_error(OctopipesError error);
CapError translate_cap_error(OctopipesCapError error);

/**
 * @brief Client constructor
 * @param string client id
 * @param string cap path
 * @param ProtocolVersion version
 * @throw std::bad_alloc
 */

Client::Client(const std::string& client_id, const std::string& cap_path, const ProtocolVersion version) {
  OctopipesClient* client;
  OctopipesError rc;
  if ((rc = octopipes_init(&client, client_id.c_str(), cap_path.c_str(), static_cast<OctopipesVersion>(version))) != OCTOPIPES_ERROR_SUCCESS) {
    throw std::bad_alloc();
  }
  octopipes_client = reinterpret_cast<void*>(client);
  //Initialize callbacks to nullptr
  this->on_receive_error = nullptr;
  this->on_received = nullptr;
  this->on_sent = nullptr;
  this->on_subscribed = nullptr;
  this->on_unsubscribed = nullptr;
}

/**
 * @brief Client destructor
 */

Client::~Client() {
  OctopipesClient* client = reinterpret_cast<OctopipesClient*>(octopipes_client);
  octopipes_cleanup(client);
  octopipes_client = nullptr;
}

/**
 * @brief start loop
 * @return octopipes::Error
 */

Error Client::startLoop() {
  OctopipesClient* client = reinterpret_cast<OctopipesClient*>(octopipes_client);
  return translate_octopipes_error(octopipes_loop_start(client));
}

/**
 * @brief stop loop
 * @return octopipes::Error
 */

Error Client::stopLoop() {
  OctopipesClient* client = reinterpret_cast<OctopipesClient*>(octopipes_client);
  return translate_octopipes_error(octopipes_loop_stop(client));
}

/**
 * @brief subscribe to Octopipes server
 * @param list<std::string> groups
 * @param CapError& assignment error
 * @return octopipes::Error
 */

Error Client::subscribe(const std::list<std::string>& groups, CapError& assignment_error) {
  OctopipesClient* client = reinterpret_cast<OctopipesClient*>(octopipes_client);
  //Prepare groups
  const char** groups_c = new char*[groups.size()];
  size_t i = 0;
  for (const auto& group : groups) {
    groups_c[i] = group.c_str();
  }
  OctopipesError rc;
  OctopipesCapError cap_error;
  //Subscribe
  if ((rc = octopipes_subscribe(client, groups_c, groups.size(), &cap_error)) != OCTOPIPES_ERROR_SUCCESS) {
    delete[] groups_c;
    //Translate CAP error
    assignment_error = translate_cap_error(cap_error);
    return translate_octopipes_error(rc);
  }
  //Free groups C
  delete[] groups_c;
  //Return success
  assignment_error = CapError::SUCCESS;
  return Error::SUCCESS;
}

/**
 * @brief unsubscribe from Octopipes Server
 * @return octopipes::Error
 */

Error Client::unsubscribe() {
  OctopipesClient* client = reinterpret_cast<OctopipesClient*>(octopipes_client);
  return translate_octopipes_error(octopipes_unsubscribe(client));
}

/**
 * @brief send a simple message to a certain node or group
 * @param string remote
 * @param void* data
 * @param uint64_t data size
 * @return octopipes::Error
 */

Error Client::send(const std::string& remote, const void* data, const uint64_t data_size) {
  return sendEx(remote, data, data_size, DEFAULT_TTL, Options::NONE);
}

/**
 * @brief send a message to a certain node or group with extended options
 * @param string remote
 * @param void* data
 * @param uint64_t data size
 * @param int ttl
 * @param Options options
 * @return octopipes::Error
 */

Error Client::sendEx(const std::string& remote, const void* data, const uint64_t data_size, const uint8_t ttl, const Options options) {
  OctopipesClient* client = reinterpret_cast<OctopipesClient*>(octopipes_client);
  //Send message
  OctopipesError rc;
  return translate_octopipes_error(octopipes_send_ex(client, remote.c_str(), data, data_size, ttl, static_cast<OctopipesOptions>(options)));
}

//Callbacks

Error Client::setReceivedCB(std::function<void(const Message*)> on_received) {
  OctopipesClient* client = reinterpret_cast<OctopipesClient*>(octopipes_client);
  this->on_received = on_received;
  //TODO: implement
  //Pass a LAMBDA to the client
  return Error::SUCCESS;
}



Error Client::setSentCB(std::function<void(Message*)> on_sent) {
  OctopipesClient* client = reinterpret_cast<OctopipesClient*>(octopipes_client);
  this->on_sent = on_sent;
  //TODO: implement
  return Error::SUCCESS;
}



Error Client::setReceive_errorCB(std::function<void(const Error)> on_receive_error) {
  OctopipesClient* client = reinterpret_cast<OctopipesClient*>(octopipes_client);
  this->on_receive_error = on_receive_error;
  //TODO: implement
  return Error::SUCCESS;
}



Error Client::setSubscribedCB(std::function<void()> on_subscribed) {
  OctopipesClient* client = reinterpret_cast<OctopipesClient*>(octopipes_client);
  this->on_subscribed = on_subscribed;
  //TODO: implement
  return Error::SUCCESS;
}



Error Client::setUnsubscribedCB(std::function<void()> on_unsubscribed) {
  OctopipesClient* client = reinterpret_cast<OctopipesClient*>(octopipes_client);
  this->on_unsubscribed = on_unsubscribed;
  //TODO: implement
  return Error::SUCCESS;
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

/**
 * @brief get Octopipes++ CAP error from OctopipesCapError
 * @param OctopipesCapError
 * @return octopipes::CapError
 */

CapError translate_cap_error(OctopipesCapError error) {
  switch (error) {
    case OCTOPIPES_CAP_ERROR_SUCCESS:
      return CapError::SUCCESS;
    case OCTOPIPES_CAP_ERROR_NAME_ALREADY_TAKEN:
      return CapError::NAME_ALREADY_TAKEN;
    case OCTOPIPES_CAP_ERROR_FS:
      return CapError::FS;
    default:
      return CapError::UNKNOWN;
  }
}


}
