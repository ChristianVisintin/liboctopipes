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

#include <octopipespp/client.hpp>
#include <octopipespp/server.hpp>

#include <cstring>
#include <getopt.h>
#include <iostream>
#include <string>
#include <thread>

#include <unistd.h>
#include <sys/time.h>

#define PROGRAM_NAME "test_serverpp"
#define USAGE PROGRAM_NAME "Usage: " PROGRAM_NAME " [Options]\n\
\t -c <capPath>\t\tSpecify the CAP Pipe for this instance\n\
\t -d <client dir>\t\tSpecify the clients directory\n\
\t -h\t\t\tShow this page\n\
"

#define PIPE_TIMEOUT 5000
#define FIRST_CLIENT_NAME "client1"
#define SECOND_CLIENT_NAME "client2"
#define FIRST_CLIENT_SUBSCRIPTION "TEST"

//Colors
#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"

enum class ClientStep {
  INITIALIZED,
  SUBSCRIBED,
  RUNNING,
  UNSUBSCRIBE,
  TERMINATED
};

void onReceived(const octopipes::Client* client, const octopipes::Message* message) {
  //Set on received to true
  void* userData = client->getUserData();
  bool* messageReceived = reinterpret_cast<bool*>(userData);
  *messageReceived = true;
  size_t payloadSize;
  const uint8_t* payload = message->getPayload(payloadSize);
  std::cout << KCYN << client->getClientId() << ": message received from " << message->getOrigin() << " (" << payloadSize << "): ";
  for (size_t i = 0; i < payloadSize; i++) {
    printf("%02x ", payload[i]);
  }
  std::cout << KNRM << std::endl;
}

int first_client_thread(const std::string capPipe) {
  //Init client
  bool messageReceived = false;
  octopipes::Client* client = new octopipes::Client(FIRST_CLIENT_NAME, capPipe, octopipes::ProtocolVersion::VERSION_1, static_cast<void*>(&messageReceived));
  std::cout << KCYN << "First client: initialized (CAP Pipe: " << capPipe << "); Starting in 1 second..." << KNRM << std::endl;
  //Set onReceived callback
  client->setReceivedCB(onReceived);
  sleep(1);
  unsigned long int totalElapsedTime = 0;
  unsigned long int tElapsedSubscribed, tElapsedRecv, tElapsedUnsubscribed;
  struct timeval t_start, t_subscribed, t_recv, t_unsubscribed;
  time_t recvTimeoutElapsed = 0;
  //Subscribe
  std::cout << KCYN << "First client: subscribing to 'TEST'" << KNRM << std::endl;
  octopipes::Error error = octopipes::Error::SUCCESS;;
  std::list<std::string> groups;
  groups.push_back(FIRST_CLIENT_SUBSCRIPTION);
  octopipes::CapError assignmentError;
  gettimeofday(&t_start, NULL);
  if ((error = client->subscribe(groups, assignmentError)) != octopipes::Error::SUCCESS) {
    std::cout << KRED << "First client: could not subscribe to server: " << octopipes::Client::getErrorDesc(error) << "; CAP error: " << static_cast<int>(assignmentError) << KNRM << std::endl;
    goto client1_cleanup;
  }
  gettimeofday(&t_subscribed, NULL);
  tElapsedSubscribed = t_subscribed.tv_usec - t_start.tv_usec;
  totalElapsedTime += tElapsedSubscribed;
  std::cout << KCYN << "First client: SUBSCRIBED; elapsed time: " << tElapsedSubscribed << " uSeconds" << KNRM << std::endl;
  std::cout << KCYN << "Waiting for a message on 'TEST' from second_client!" << KNRM << std::endl;
  //Start loop
  if ((error = client->startLoop()) != octopipes::Error::SUCCESS) {
    std::cout << KRED << "First client: Could not start client loop: " << octopipes::Client::getErrorDesc(error) << KNRM << std::endl;
    goto client1_cleanup;
  }
  std::cout << KCYN << "Client loop started" << KNRM << std::endl;
  while (!messageReceived && recvTimeoutElapsed < 10000) { //Timeout at 10 seconds
    usleep(100000); //100ms
    recvTimeoutElapsed += 100;
  }
  gettimeofday(&t_recv, NULL);
  tElapsedRecv = t_recv.tv_usec - t_subscribed.tv_usec;
  totalElapsedTime += tElapsedRecv;
  if (!messageReceived) {
    std::cout << KCYN << "First client: TIMEOUT" << KNRM << std::endl;
    goto client1_cleanup;
  }
  std::cout << KCYN << "First client: Message received; elapsed time: " << tElapsedRecv << " uSeconds" << KNRM << std::endl;
client1_cleanup:
  //Unsubscribe
  if ((error = client->unsubscribe()) != octopipes::Error::SUCCESS) {
    std::cout << KRED << "First client: could not unsubscribe from server: " << octopipes::Client::getErrorDesc(error) << KNRM << std::endl;
  }
  gettimeofday(&t_unsubscribed, NULL);
  tElapsedUnsubscribed = t_unsubscribed.tv_usec - t_recv.tv_usec;
  totalElapsedTime += tElapsedUnsubscribed;
  std::cout << KCYN << "First client: UNSUBSCRIBED; elapsed time: " << tElapsedUnsubscribed << " uSeconds" << KNRM << std::endl;
  delete client;
  std::cout << KCYN << "First client: terminated; TOTAL elapsed time: " << totalElapsedTime << " uSeconds" << KNRM << std::endl;
  return static_cast<int>(error);
}

int second_client_thread(const std::string capPipe) {
  //Init client
  bool messageReceived = false;
  octopipes::Client* client = new octopipes::Client(SECOND_CLIENT_NAME, capPipe, octopipes::ProtocolVersion::VERSION_1, static_cast<void*>(&messageReceived));
  std::cout << KMAG << "Second client: initialized; Starting in 3 second..." << KNRM << std::endl;
  uint8_t* dataOut = nullptr;
  size_t dataOutSize = 12;
  //Set onReceived callback
  client->setReceivedCB(onReceived);
  sleep(3);
  unsigned long int totalElapsedTime = 0;
  unsigned long int tElapsedSubscribed, tElapsedSent, tElapsedUnsubscribed;
  struct timeval t_start, t_subscribed, t_sent, t_unsubscribed;
  //Subscribe
  std::cout << KMAG << "Second client: subscribing to 'TEST'" << KNRM << std::endl;
  octopipes::Error error = octopipes::Error::SUCCESS;;
  std::list<std::string> groups;
  octopipes::CapError assignmentError;
  gettimeofday(&t_start, NULL);
  if ((error = client->subscribe(groups, assignmentError)) != octopipes::Error::SUCCESS) {
    std::cout << KRED << "Second client: could not subscribe to server: " << octopipes::Client::getErrorDesc(error) << "; CAP error: " << static_cast<int>(assignmentError) << KNRM << std::endl;
    goto client2_cleanup;
  }
  gettimeofday(&t_subscribed, NULL);
  tElapsedSubscribed = t_subscribed.tv_usec - t_start.tv_usec;
  totalElapsedTime += tElapsedSubscribed;
  std::cout << KMAG << "Second client: SUBSCRIBED; elapsed time: " << tElapsedSubscribed << " uSeconds" << KNRM << std::endl;
  std::cout << KMAG << "Going to send a message to first_client!" << KNRM << std::endl;
  //Send message
  dataOut = new uint8_t[dataOutSize];
  dataOut[0] = 'H';
  dataOut[1] = 'E';
  dataOut[2] = 'L';
  dataOut[3] = 'L';
  dataOut[4] = 'O';
  dataOut[5] = ' ';
  dataOut[6] = 'W';
  dataOut[7] = 'O';
  dataOut[8] = 'R';
  dataOut[9] = 'L';
  dataOut[10] = 'D';
  dataOut[11] = '!';
  if ((error = client->send(FIRST_CLIENT_SUBSCRIPTION, dataOut, dataOutSize)) != octopipes::Error::SUCCESS) {
    std::cout << KRED << "Second client: could not send message to " << FIRST_CLIENT_SUBSCRIPTION << ": " << octopipes::Client::getErrorDesc(error) << KNRM << std::endl;
    goto client2_cleanup;
  }
  gettimeofday(&t_sent, NULL);
  tElapsedSent = t_sent.tv_usec - t_subscribed.tv_usec;
  totalElapsedTime += tElapsedSent;
  std::cout << KMAG << "Second client: Message sent; time elapsed: " << tElapsedSent << " uSeconds" << KNRM << std::endl;
  sleep(1);
client2_cleanup:
  if (dataOut != nullptr) {
    delete[] dataOut;
  }
  //Unsubscribe
  if ((error = client->unsubscribe()) != octopipes::Error::SUCCESS) {
    std::cout << KRED << "Second client: could not unsubscribe from server: " << octopipes::Client::getErrorDesc(error) << KNRM << std::endl;
  }
  gettimeofday(&t_unsubscribed, NULL);
  tElapsedUnsubscribed = t_unsubscribed.tv_usec - t_sent.tv_usec;
  totalElapsedTime += tElapsedUnsubscribed;
  std::cout << KMAG << "Second client: UNSUBSCRIBED; elapsed time: " << tElapsedUnsubscribed << " uSeconds" << KNRM << std::endl;
  delete client;
  std::cout << KMAG << "Second client: terminated; TOTAL elapsed time: " << totalElapsedTime << " uSeconds" << KNRM << std::endl;
  return static_cast<int>(error);
}

int main(int argc, char** argv) {
  printf(PROGRAM_NAME " liboctopipespp Build: " OCTOPIPESPP_LIB_VERSION "\n");
  std::string capPipe;
  std::string clientDir;
  int opt;
  while ((opt = getopt(argc, argv, "c:d:h")) != -1) {
    switch (opt) {
    case 'c':
      capPipe = optarg;
      break;
    case 'd':
      clientDir = optarg;
      break;
    case 'h':
      printf("%s\n", USAGE);
      return 0;
    }
  }

  //Check if tx pipe and rx pipe are set
  if (clientDir.empty() || capPipe.empty()) {
    printf("Missing ClientDir Pipe (%d) or CAP Pipe (%d)\n%s\n", clientDir.empty(), capPipe.empty(), USAGE);
    return 1;
  }
  octopipes::ServerError error = octopipes::ServerError::SUCCESS;
  //Init threads
  std::thread first_client;
  std::thread second_client;
  //Start server
  time_t serverElapsedTime = 0;
  size_t capRequest = 0;
  octopipes::Server* octopipesServer = new octopipes::Server(capPipe, clientDir, octopipes::ProtocolVersion::VERSION_1);
  std::cout << KYEL << "Starting CAP listener..." << KNRM << std::endl;
  //Start CAP listener
  if ((error = octopipesServer->startCapListener()) != octopipes::ServerError::SUCCESS) {
    std::cout << KRED << octopipes::Server::getServerErrorDesc(error) << KNRM << std::endl;
    goto cleanup;
  }
  std::cout << KYEL << "CAP listener started!" << KNRM << std::endl;
  //Start clients
  first_client = std::thread(first_client_thread, capPipe);
  second_client = std::thread(second_client_thread, capPipe);
  //Start looping
  while (serverElapsedTime < 10000 && capRequest < 4) { //Max 10 seconds; if is successful 4 cap requests will be processed (2 SUBSCRIPTION, 2 UNSUBSCRIPTIONS)
    //Process CAP
    size_t requests = 0;
    if ((error = octopipesServer->processCapOnce(requests)) != octopipes::ServerError::SUCCESS) {
      std::cout << KRED << "Error while processing CAP: " << octopipes::Server::getServerErrorDesc(error) << KNRM << std::endl;
    }
    capRequest += requests;
    if (requests > 0) {
      std::cout << KYEL << "Processed " << requests << " requests from CAP" << KNRM << std::endl;
    }
    //Process clients
    std::string faultClient;
    if ((error = octopipesServer->processOnce(requests, faultClient)) != octopipes::ServerError::SUCCESS) {
      std::cout << KRED << "Error while processing CLIENT '" << faultClient << "':" << octopipes::Server::getServerErrorDesc(error) << KNRM << std::endl;
    }
    if (requests > 0) {
      std::cout << KYEL << "Processed " << requests << " requests from clients" << KNRM << std::endl;
    }
    usleep(100000); //100ms
    serverElapsedTime += 100;
  }
  if (serverElapsedTime >= 10000) {
    std::cout << KRED << "Server TIMEOUT..." << KNRM << std::endl;
    error = octopipes::ServerError::UNKNOWN;
  }
  std::cout << KYEL << "Stopping server..." << KNRM << std::endl;

cleanup:
  if (first_client.joinable()) {
    first_client.join();
  }
  if (second_client.joinable()) {
    second_client.join();
  }
  delete octopipesServer;
  std::cout << KYEL << "Server stopped and cleaned up" << KNRM << std::endl;
  return static_cast<int>(error);
}
