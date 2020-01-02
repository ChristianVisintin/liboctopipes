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

#include <octopipespp/server.hpp>

#include <octopipes/octopipes.h>

#include <cstring>

namespace octopipes {

//Local functions
Error translate_octopipes_error(OctopipesError error);
ServerError translate_octopipes_server_error(OctopipesServerError error);
CapError translate_cap_error(OctopipesCapError error);

/**
 * @brief Server class constructor
 * @param string cap path
 * @param string client directory
 * @param ProtocolVersion
 * @throw std::bad_alloc
 */

Server::Server(const std::string& cap_path, const std::string& client_dir, const ProtocolVersion version) {
  OctopipesServer* server;
  if (octopipes_server_init(&server, cap_path.c_str(), client_dir.c_str(), static_cast<OctopipesVersion>(version)) != OCTOPIPES_SERVER_ERROR_SUCCESS) {
    throw std::bad_alloc();
  }
  octopipes_server = reinterpret_cast<void*>(server);
}

/**
 * @brief Server class destructor
 */

Server::~Server() {
  OctopipesServer* server = reinterpret_cast<OctopipesServer*>(octopipes_server);
  octopipes_server_cleanup(server);
}

/**
 * @brief start CAP listener
 * @return ServerError
 */

ServerError Server::startCapListener() {
  OctopipesServer* server = reinterpret_cast<OctopipesServer*>(octopipes_server);
  return translate_octopipes_server_error(octopipes_server_start_cap_listener(server));
}

/**
 * @brief stop CAP listener
 * @return ServerError
 */

ServerError Server::stopCapListener() {
  OctopipesServer* server = reinterpret_cast<OctopipesServer*>(octopipes_server);
  return translate_octopipes_server_error(octopipes_server_stop_cap_listener(server));
}

/**
 * @brief process one message (if found) in the CAP pipe inbox
 * @param size_t& amount of processed requests
 * @return ServerError
 */

ServerError Server::processCapOnce(size_t& requests) {
  OctopipesServer* server = reinterpret_cast<OctopipesServer*>(octopipes_server);
  return translate_octopipes_server_error(octopipes_server_process_cap_once(server, &requests));
}

/**
 * @brief process all the available messages in the CAP pipe inbox
 * @param size_t& amount of processed requests
 * @return ServerError
 */

ServerError Server::processCapAll(size_t& requests) {
  OctopipesServer* server = reinterpret_cast<OctopipesServer*>(octopipes_server);
  return translate_octopipes_server_error(octopipes_server_process_cap_all(server, &requests));
}

/**
 * @brief start a new worker on server
 * @param string client name
 * @param list_of_strings subsrciptions
 * @param string client tx pipe
 * @param string client rx pipe
 * @return ServerError
 */

ServerError Server::startWorker(const std::string& client, const std::list<std::string>& subscriptions, const std::string& cli_tx_pipe, const std::string& cli_rx_pipe) {
  OctopipesServer* server = reinterpret_cast<OctopipesServer*>(octopipes_server);
  char** subs = new char*[subscriptions.size()];
  size_t index = 0;
  for (const auto& sub : subscriptions) {
    subs[index] = new char[sub.length() + 1];
    memcpy(subs[index], sub.c_str(), sub.length());
    (subs[index])[sub.length()] = 0x00;
  }
  ServerError rc = translate_octopipes_server_error(octopipes_server_start_worker(server, client.c_str(), subs, subscriptions.size(), cli_tx_pipe.c_str(), cli_rx_pipe.c_str()));
  for (size_t i = 0; i < subscriptions.size(); i++) {
    delete[] subs[index];
  }
  delete[] subs;
  return rc;
}

/**
 * @brief stop a running worker
 * @param string client associated to worker
 * @return ServerError
 */

ServerError Server::stopWorker(const std::string& client) {
  OctopipesServer* server = reinterpret_cast<OctopipesServer*>(octopipes_server);
  return translate_octopipes_server_error(octopipes_server_stop_worker(server, client.c_str()));
}

/**
 * @brief process first available message in any worker inbox
 * @param size_t& requests processed
 * @param string& client which raised an error
 * @return ServerError
 */

ServerError Server::processFirst(size_t& requests, std::string& client) {
  OctopipesServer* server = reinterpret_cast<OctopipesServer*>(octopipes_server);
  const char* client_ptr = NULL;
  ServerError rc = translate_octopipes_server_error(octopipes_server_process_first(server, &requests, &client_ptr));
  if (client_ptr != NULL) {
    client = client_ptr;
  }
  return rc;
}

/**
 * @brief process each worker inbox once if possible
 * @param size_t& requests processed
 * @param string& client which raised an error
 * @return ServerError
 */

ServerError Server::processOnce(size_t& requests, std::string& client) {
  OctopipesServer* server = reinterpret_cast<OctopipesServer*>(octopipes_server);
  const char* client_ptr = NULL;
  ServerError rc = translate_octopipes_server_error(octopipes_server_process_once(server, &requests, &client_ptr));
  if (client_ptr != NULL) {
    client = client_ptr;
  }
  return rc;
}

/**
 * @brief process each worker inbox until all inboxes are empty (NOTE: this could be blocking for a very long time, ProcessOnce should be preferred)
 * @param size_t& requests processed
 * @param string& client which raised an error
 * @return ServerError
 */

ServerError Server::processAll(size_t& requests, std::string& client) {
  size_t this_session_requests = 0;
  do {
    ServerError ret = processOnce(this_session_requests, client);
    if (ret != ServerError::SUCCESS) {
      return ret;
    }
    requests += this_session_requests;
  } while(this_session_requests > 0);
  return ServerError::SUCCESS;
}

/**
 * @brief get whether provided client is subscribed
 * @param string client to query 
 * @return
 */

bool Server::isSubscribed(const std::string& client) {
  OctopipesServer* server = reinterpret_cast<OctopipesServer*>(octopipes_server);
  return octopipes_server_is_subscribed(server, client.c_str()) == OCTOPIPES_SERVER_ERROR_SUCCESS;
}

/**
 * @brief get all the subscriptions for the provided client
 * @param string client to query
 * @return string
 */

std::list<std::string> Server::getSubscriptions(const std::string& client) {
  OctopipesServer* server = reinterpret_cast<OctopipesServer*>(octopipes_server);
  std::list<std::string> subscriptions;
  char** sub_ptr = NULL;
  size_t sub_len = 0;
  octopipes_server_get_subscriptions(server, client.c_str(), &sub_ptr, &sub_len);
  for (size_t i = 0; i < sub_len; i++) {
    subscriptions.push_back(sub_ptr[i]);
  }
  return subscriptions;
}

/**
 * @brief get all the subscribed clients
 * @return list of strings
 */

std::list<std::string> Server::getClients() {
  OctopipesServer* server = reinterpret_cast<OctopipesServer*>(octopipes_server);
  std::list<std::string> clients;
  char** client_ptr = NULL;
  size_t cli_len = 0;
  octopipes_server_get_clients(server, &client_ptr, &cli_len);
  for (size_t i = 0; i < cli_len; i++) {
    clients.push_back(client_ptr[i]);
  }
  if (client_ptr != NULL) {
    free(client_ptr);
  }
  return clients;
}

/**
 * @brief return the description for a certain error
 * @param ServerError
 * @return string
 */

const std::string getServerErrorDesc(const ServerError error) {
  switch (error) {
    case ServerError::BAD_ALLOC: 
      return "Could not allocate more memory";
    case ServerError::BAD_CHECKSUM:
      return "Message has bad checksum";
    case ServerError::BAD_CLIENT_DIR:
      return "It was not possible to initialize the provided clients directory";
    case ServerError::BAD_PACKET:
      return "The received packet has an invalid syntax";
    case ServerError::CAP_TIMEOUT:
      return "CAP timeout";
    case ServerError::NO_RECIPIENT:
      return "The received message has no recipient, but it should had";
    case ServerError::OPEN_FAILED:
      return "Could not open or create the pipe";
    case ServerError::READ_FAILED:
      return "Could not read from pipe";
    case ServerError::SUCCESS:
      return "Not an error";
    case ServerError::THREAD_ALREADY_RUNNING:
      return "Thread is already running";
    case ServerError::THREAD_ERROR:
      return "There was an error in initializing the thread";
    case ServerError::UNINITIALIZED:
      return "Octopipes sever is not correctly initialized";
    case ServerError::UNSUPPORTED_VERSION:
      return "Unsupported protocol version";
    case ServerError::WORKER_ALREADY_RUNNING:
      return "A worker with these parameters is already running";
    case ServerError::WORKER_EXISTS:
      return "A worker with these parameters already exists";
    case ServerError::WORKER_NOT_FOUND:
      return "Could not find a worker with that name";
    case ServerError::WORKER_NOT_RUNNING:
      return "The requested worker is not running";
    case ServerError::WRITE_FAILED:
      return "Could not write to pipe";
    case ServerError::UNKNOWN:
    default:
      return "Unknown error";
  }
}

ServerError translate_octopipes_server_error(OctopipesServerError error) {
  switch (error) {
    case OCTOPIPES_SERVER_ERROR_BAD_ALLOC: 
      return ServerError::BAD_ALLOC;
    case OCTOPIPES_SERVER_ERROR_BAD_CHECKSUM:
      return ServerError::BAD_CHECKSUM;
    case OCTOPIPES_SERVER_ERROR_BAD_CLIENT_DIR:
      return ServerError::BAD_CLIENT_DIR;
    case OCTOPIPES_SERVER_ERROR_BAD_PACKET:
      return ServerError::BAD_PACKET;
    case OCTOPIPES_SERVER_ERROR_CAP_TIMEOUT:
      return ServerError::CAP_TIMEOUT;
    case OCTOPIPES_SERVER_ERROR_NO_RECIPIENT:
      return ServerError::NO_RECIPIENT;
    case OCTOPIPES_SERVER_ERROR_OPEN_FAILED:
      return ServerError::OPEN_FAILED;
    case OCTOPIPES_SERVER_ERROR_READ_FAILED:
      return ServerError::READ_FAILED;
    case OCTOPIPES_SERVER_ERROR_SUCCESS:
      return ServerError::SUCCESS;
    case OCTOPIPES_SERVER_ERROR_THREAD_ALREADY_RUNNING:
      return ServerError::THREAD_ALREADY_RUNNING;
    case OCTOPIPES_SERVER_ERROR_THREAD_ERROR:
      return ServerError::THREAD_ERROR;
    case OCTOPIPES_SERVER_ERROR_UNINITIALIZED:
      return ServerError::UNINITIALIZED;
    case OCTOPIPES_SERVER_ERROR_UNSUPPORTED_VERSION:
      return ServerError::UNSUPPORTED_VERSION;
    case OCTOPIPES_SERVER_ERROR_WORKER_ALREADY_RUNNING:
      return ServerError::WORKER_ALREADY_RUNNING;
    case OCTOPIPES_SERVER_ERROR_WORKER_EXISTS:
      return ServerError::WORKER_EXISTS;
    case OCTOPIPES_SERVER_ERROR_WORKER_NOT_FOUND:
      return ServerError::WORKER_NOT_FOUND;
    case OCTOPIPES_SERVER_ERROR_WORKER_NOT_RUNNING:
      return ServerError::WORKER_NOT_RUNNING;
    case OCTOPIPES_SERVER_ERROR_WRITE_FAILED:
      return ServerError::WRITE_FAILED;
    case OCTOPIPES_SERVER_ERROR_UNKNOWN:
    default:
      return ServerError::UNKNOWN;
  }
}

}
