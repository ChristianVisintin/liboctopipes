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

#include <octopipespp/octopipespp.hpp>

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
  this->user_data = nullptr;
  //Set this in user_data
  client->user_data = reinterpret_cast<void*>(this);
}

/**
 * @brief Client constructor with user data
 * @param string client id
 * @param string cap path
 * @param ProtocolVersion version
 * @param void* user_data
 * @throw std::bad_alloc
 */

Client::Client(const std::string& client_id, const std::string& cap_path, const ProtocolVersion version, void* user_data) : Client(client_id, cap_path, version) {
  this->user_data = user_data;
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
  const char** groups_c = new const char*[groups.size()];
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
  return translate_octopipes_error(octopipes_send_ex(client, remote.c_str(), data, data_size, ttl, static_cast<OctopipesOptions>(options)));
}

//Callbacks

Error Client::setReceivedCB(std::function<void(const Client*, const Message*)> on_received) {
  OctopipesClient* client = reinterpret_cast<OctopipesClient*>(octopipes_client);
  this->on_received = on_received;
  //Use a lambda expression to convert OctopipesMessage to message
  client->on_received = [](const OctopipesClient* client, const OctopipesMessage* message) {
    Client* client_ptr = reinterpret_cast<Client*>(client->user_data); //User data is always set here
    if (client_ptr->on_received == nullptr) {
      return; //Just return
    }
    //Convert OctopipesMessage to message
    Message* msg = new Message(reinterpret_cast<const void*>(message));
    client_ptr->on_received(client_ptr, msg);
    delete msg;
  };
  return Error::SUCCESS;
}

/**
 * @brief set on_sent Callback
 * @param function on_sent callback
 * @return octopipes::Error
 */

Error Client::setSentCB(std::function<void(const Client*, const Message*)> on_sent) {
  OctopipesClient* client = reinterpret_cast<OctopipesClient*>(octopipes_client);
  this->on_sent = on_sent;
  //Use a lambda expression to convert OctopipesMessage to message
  client->on_sent = [](const OctopipesClient* client, const OctopipesMessage* message) {
    Client* client_ptr = reinterpret_cast<Client*>(client->user_data); //User data is always set here
    if (client_ptr->on_sent == nullptr) {
      return; //Just return
    }
    //Convert OctopipesMessage to message
    Message* msg = new Message(reinterpret_cast<const void*>(message));
    client_ptr->on_sent(client_ptr, msg);
    delete msg;
  };
  return Error::SUCCESS;
}

/**
 * @brief set on_error Callback
 * @param function on_error callback
 * @return octopipes::Error
 */

Error Client::setReceive_errorCB(std::function<void(const Client*, const Error)> on_receive_error) {
  OctopipesClient* client = reinterpret_cast<OctopipesClient*>(octopipes_client);
  this->on_receive_error = on_receive_error;
  //Use a lambda expression
  client->on_receive_error = [](const OctopipesClient* client, const OctopipesError error) {
    Client* client_ptr = reinterpret_cast<Client*>(client->user_data); //User data is always set here
    if (client_ptr->on_receive_error == nullptr) {
      return; //Just return
    }
    client_ptr->on_receive_error(client_ptr, translate_octopipes_error(error));
  };
  return Error::SUCCESS;
}

/**
 * @brief set on_subscribed Callback
 * @param function on_subscribed callback
 * @return octopipes::Error
 */

Error Client::setSubscribedCB(std::function<void(const Client*)> on_subscribed) {
  OctopipesClient* client = reinterpret_cast<OctopipesClient*>(octopipes_client);
  this->on_subscribed = on_subscribed;
  //Use a lambda expression
  client->on_subscribed = [](const OctopipesClient* client) {
    Client* client_ptr = reinterpret_cast<Client*>(client->user_data); //User data is always set here
    if (client_ptr->on_subscribed == nullptr) {
      return; //Just return
    }
    client_ptr->on_subscribed(client_ptr);
  };
  return Error::SUCCESS;
}

/**
 * @brief set on_unsubscribed Callback
 * @param function on_unsubscribed callback
 * @return octopipes::Error
 */

Error Client::setUnsubscribedCB(std::function<void(const Client*)> on_unsubscribed) {
  OctopipesClient* client = reinterpret_cast<OctopipesClient*>(octopipes_client);
  this->on_unsubscribed = on_unsubscribed;
  //Use a lambda expression
  client->on_unsubscribed = [](const OctopipesClient* client) {
    Client* client_ptr = reinterpret_cast<Client*>(client->user_data); //User data is always set here
    if (client_ptr->on_unsubscribed == nullptr) {
      return; //Just return
    }
    client_ptr->on_unsubscribed(client_ptr);
  };
  return Error::SUCCESS;
}

/**
 * @brief returns user data (or nullptr)
 * @return void*
 */

void* Client::getUserData() {
  return user_data;
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
    case OCTOPIPES_ERROR_UNSUPPORTED_VERSION:
      return Error::UNSUPPORTED_VERSION;
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

/**
 * @brief get error description from the provided error code
 * @param Error
 * @return const std::string
 */

const std::string Client::getErrorDesc(const Error error) {
  switch (error) {
    case Error::BAD_ALLOC:
      return "Could not allocate more memory in the heap";
    case Error::BAD_CHECKSUM:
      return "The last packet has a bad checksum and the ignore checksum flag is set to 0";
    case Error::BAD_PACKET:
      return "The packet syntax is invalid or is unexpected";
    case Error::CAP_TIMEOUT:
      return "The CAP timeout. The request hasn't been fulfilled in time.";
    case Error::NO_DATA_AVAILABLE:
      return "There's no data available to be read";
    case Error::NOT_SUBSCRIBED:
      return "The client is not subscribed yet to Octopipes server. Sending of messages to server is allowed only using the CAP.";
    case Error::NOT_UNSUBSCRIBED:
      return "This operation is not permitted, since the client isn't unsubscribed";
    case Error::OPEN_FAILED:
      return "Could not open the FIFO";
    case Error::READ_FAILED:
      return "An error occurred while trying to read from FIFO";
    case Error::SUCCESS:
      return "Not an error";
    case Error::THREAD:
      return "Could not start loop thread";
    case Error::UNINITIALIZED:
      return "The OctopipesClient must be initialized calling octopipes_init() first";
    case Error::UNSUPPORTED_VERSION:
      return "This protocol version is unsupported by Octopipes Version " OCTOPIPES_LIB_VERSION;
    case Error::WRITE_FAILED:
      return "Could not write data to FIFO";
    case Error::UNKNOWN_ERROR:
    default:
      return "Unknown error";
  }
}

}
