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

#include "typespp.hpp"
#include "message.hpp"

#include <functional>
#include <list>

namespace octopipes {

class Client {

public:
  Client(const std::string& client_id, const std::string& cap_path, const ProtocolVersion version);
  ~Client();
  Error startLoop();
  Error stopLoop();
  Error subscribe(const std::list<std::string>& groups, CapError& assignment_error);
  Error unsubscribe();
  Error octopipes_send(const std::string& remote, const void* data, const uint64_t data_size);
  Error octopipes_send_ex(const std::string& remote, const void* data, const uint64_t data_size, const uint8_t ttl, const Options options);
  //Callbacks
  Error octopipes_set_received_cb(std::function<void(const Message*)> on_received);
  Error octopipes_set_sent_cb(std::function<void(Message*)> on_sent);
  Error octopipes_set_receive_error_cb(std::function<void(const Error)> on_receive_error);
  Error octopipes_set_subscribed_cb(std::function<void()> on_subscribed);
  Error octopipes_set_unsubscribed_cb(std::function<void()> on_unsubscribed);
  //Error
  static const std::string get_error_desc(const Error error);

private:
  //Class attributes
  void* octopipes_client;

};

}

#endif
