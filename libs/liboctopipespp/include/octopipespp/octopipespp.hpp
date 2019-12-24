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

#ifndef OCTOPIPESPP_HPP
#define OCTOPIPESPP_HPP

//Version defines
#define OCTOPIPESPP_LIB_VERSION "0.1.0"
#define OCTOPIPESPP_LIB_VERSION_MAJOR 0
#define OCTOPIPESPP_LIB_VERSION_MINOR 1

#include "typespp.hpp"
#include "message.hpp"

#include <functional>
#include <list>

namespace octopipes {

class Client {

public:
  Client(const std::string& client_id, const std::string& cap_path, const ProtocolVersion version);
  Client(const std::string& client_id, const std::string& cap_path, const ProtocolVersion version, void* user_data);
  ~Client();
  Error startLoop();
  Error stopLoop();
  Error subscribe(const std::list<std::string>& groups, CapError& assignment_error);
  Error unsubscribe();
  Error send(const std::string& remote, const void* data, const uint64_t data_size);
  Error sendEx(const std::string& remote, const void* data, const uint64_t data_size, const uint8_t ttl, const Options options);
  //Callbacks
  Error setReceivedCB(std::function<void(const Client*, const Message*)> on_received);
  Error setSentCB(std::function<void(const Client*, const Message*)> on_sent);
  Error setReceive_errorCB(std::function<void(const Client*, const Error)> on_receive_error);
  Error setSubscribedCB(std::function<void(const Client*)> on_subscribed);
  Error setUnsubscribedCB(std::function<void(const Client*)> on_unsubscribed);
  //Getters
  void* getUserData();
  //Error
  static const std::string getErrorDesc(const Error error);

private:
  //Class attributes
  void* octopipes_client;
  void* user_data;
  //Callbacks
  std::function<void(const Client*, const Message*)> on_received;
  std::function<void(const Client*, const Message*)> on_sent;
  std::function<void(const Client*, const Error)> on_receive_error;
  std::function<void(const Client*)> on_subscribed;
  std::function<void(const Client*)> on_unsubscribed;

};

}

#endif
