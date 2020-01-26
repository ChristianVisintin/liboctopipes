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

#ifndef OCTOPIPESPP_SERVER_HPP
#define OCTOPIPESPP_SERVER_HPP

#include "typespp.hpp"
#include "message.hpp"

#include <functional>
#include <list>

namespace octopipes {

class Server {

public:
  Server(const std::string& cap_path, const std::string& client_dir, const ProtocolVersion version);
  ~Server();
  ServerError startCapListener();
  ServerError stopCapListener();
  ServerError processCapOnce(size_t& requests);
  ServerError processCapAll(size_t& requests);
  //Workers
  ServerError startWorker(const std::string& client, const std::list<std::string>& subscriptions, const std::string& cli_tx_pipe, const std::string& cli_rx_pipe);
  ServerError stopWorker(const std::string& client);
  ServerError processFirst(size_t& requests, std::string& client);
  ServerError processOnce(size_t& requests, std::string& client);
  ServerError processAll(size_t& requests, std::string& client);
  //Getters
  bool isSubscribed(const std::string& client);
  std::list<std::string> getSubscriptions(const std::string& client);
  std::list<std::string> getClients();
  //ServerError
  static const std::string getServerErrorDesc(const ServerError error);

private:
  //Class attributes
  void* octopipes_server;

};

}

#endif
