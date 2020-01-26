# LibOctopipes

Current Version: 0.1.0 (13/01/2020)
Developed by *Christian Visintin*

LibOctopipes is a C++ Wrapper of liboctopipes which provides functions to implement Octopipes server and clients in C++.

- [LibOctopipes](#liboctopipes)
  - [Build](#build)
  - [Client Implementation](#client-implementation)
  - [Server Implementation](#server-implementation)
  - [Documentation](#documentation)
    - [Data Types (types.hpp)](#data-types-typeshpp)
      - [Error](#error)
      - [CapMessage](#capmessage)
      - [CapError](#caperror)
      - [Options](#options)
      - [ProtocolVersion](#protocolversion)
      - [ServerError](#servererror)
    - [Client (client.hpp)](#client-clienthpp)
      - [Client class constructors](#client-class-constructors)
      - [startLoop](#startloop)
      - [stopLoop](#stoploop)
      - [subscribe](#subscribe)
      - [unsubscribe](#unsubscribe)
      - [send](#send)
      - [send_ex](#sendex)
      - [Callback Setters](#callback-setters)
      - [Client Getters](#client-getters)
    - [Message (message.hpp)](#message-messagehpp)
      - [Message class constructors](#message-class-constructors)
      - [Message Encoding/Decoding](#message-encodingdecoding)
      - [Message getters](#message-getters)
    - [Server (server.hpp)](#server-serverhpp)
      - [Server class constructor](#server-class-constructor)
      - [startCapListener](#startcaplistener)
      - [stopCapListener](#stopcaplistener)
      - [processCapOnce](#processcaponce)
      - [processCapAll](#processcapall)
      - [startWorker](#startworker)
      - [stopWorker](#stopworker)
      - [processFirst](#processfirst)
      - [processOnce](#processonce)
      - [processAll](#processall)
      - [Server getters](#server-getters)
  - [Changelog](#changelog)
  - [License](#license)

## Build

Dependencies:

- C++11 compiler
- Cmake
- pthread

```sh
mkdir build/
cmake ..
make
make install
```

## Client Implementation

Initialize the client

```cpp
octopipes::Client* client = new octopipes::Client(clientId, capPipe, octopipes::ProtocolVersion::VERSION_1);
```

Set callbacks (optional)

```cpp
client->setReceive_errorCB(on_receive_error);
client->setReceivedCB(on_received);
client->setSubscribedCB(on_subscribed);
client->setUnsubscribedCB(on_unsubscribed);
```

Subscribe to server

```cpp
if ((ret = client->subscribe(groups, cap_error)) != octopipes::Error::SUCCESS) {
  //Handle error
}
```

Start loop (after that you will start receiving messages through the on_received callback):

```cpp
if ((ret = client->startLoop() != octopipes::Error::SUCCESS) {
  //Handle error
}
```

Send messages

```cpp
if ((ret = client->send(remote, data, data_size)) != octopipes::Error::SUCCESS) {
  //Handle error
}
```

Unsubscribe

```cpp
if ((ret = client->unsubscribe()) != octopipes::Error::SUCCESS) {
  //Handle error
}
```

Free resources

```cpp
delete client;
```

Two clients implementation can be found in and are provided with liboctopipes [Here](https://github.com/ChristianVisintin/Octopipes/tree/master/libs/liboctopipespp/tests/client/)

## Server Implementation

Initialize the server.
These parameters are required:

- CAP pipe path
- Directory where the clients pipes will be stored (if doesn't exist, will be created)
- Protocol version

```cpp
octopipes::Server* octopipesServer = new octopipes::Server(capPipe, clientDir, octopipes::ProtocolVersion::VERSION_1);
```

Start the CAP listener (in order to receive message on the CAP)

```cpp
if ((error = octopipesServer->startCapListener()) != octopipes::ServerError::SUCCESS) {
  std::cout << "Could not start CAP listener: " << octopipes::Server::getServerErrorDesc(error) << std::endl;
  goto cleanup;
}
```

Server main loop. This simple loop takes care of:

- CAP:
  - listening on the CAP inbox for new messages
  - handling subscription requests
  - handling unsubscription requests
- Clients:
  - Waiting for messages from clients
  - Dispatching the client messages to the other clients subscribed to the associated remote

```cpp
while (true) {
  //Process CAP
  size_t requests = 0;
  if ((error = octopipesServer->processCapOnce(requests)) != octopipes::ServerError::SUCCESS) {
    std::cout << "Error while processing CAP: " << octopipes::Server::getServerErrorDesc(error) << std::endl;
  }
  //Process clients
  std::string faultClient;
  if ((error = octopipesServer->processOnce(requests, faultClient)) != octopipes::ServerError::SUCCESS) {
    std::cout << "Error while processing CLIENT '" << faultClient << "':" << octopipes::Server::getServerErrorDesc(error) << std::endl;
  }
  if (requests > 0) {
    std::cout << "Processed " << requests << " requests from clients" << std::endl;
  }
  usleep(100000); //100ms
}
```

Stop server
It's enough to delete the server object.

```cpp
delete octopipesServer;
```

## Documentation

liboctopipes is a wrapper of liboctopipes, so it justs wraps all the public calls it provides in c++14 safe calls.
The library is structured as follows

- client.hpp : library header to include to implement an OctopipesClient
- server.hpp : library header to include to implement an OctopipesServer
  - message.hpp : Message class, which represents an OctopipesMessage
  - typespp.hpp : contains the declaration of each type (except classes)

All the classes and types are inside the **octopipes** namespace.

### Data Types (types.hpp)

#### Error

*public*  
Describes a Client error, it's the C++ version of OctopipesError.

```cpp
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
```

#### CapMessage

*private*  
Represents the type of a CAP message.

```cpp
enum class CapMessage {
  UNKNOWN = 0x00,
  SUBSCRIPTION = 0x01,
  ASSIGNMENT = 0xFF,
  UNSUBSCRIPTION = 0x02
};
```

#### CapError

*public*  
Represents the error type received from the server through the CAP

```cpp
enum class CapError {
  SUCCESS = 0,
  NAME_ALREADY_TAKEN = 1,
  FS = 2,
  UNKNOWN = 255
};
```

#### Options

*public*  
Represents the message options.

```cpp
enum class Options {
  NONE = 0,
  REQUIRE_ACK = 1,
  ACK = 2,
  IGNORE_CHECKSUM = 4
};
```

#### ProtocolVersion

*public*  
Represents the protocol version used by the client/server.

```cpp
enum class ProtocolVersion {
  VERSION_1 = 1
};
```

#### ServerError

*public*  
Represents an error returned by an operation on the server.

```cpp
enum class ServerError {
  SUCCESS,
  BAD_ALLOC,
  UNINITIALIZED,
  BAD_PACKET,
  BAD_CHECKSUM,
  UNSUPPORTED_VERSION,
  OPEN_FAILED,
  WRITE_FAILED,
  READ_FAILED,
  CAP_TIMEOUT,
  THREAD_ERROR,
  THREAD_ALREADY_RUNNING,
  WORKER_EXISTS,
  WORKER_NOT_FOUND,
  WORKER_ALREADY_RUNNING,
  WORKER_NOT_RUNNING,
  NO_RECIPIENT,
  BAD_CLIENT_DIR,
  UNKNOWN
};
```

### Client (client.hpp)

The Client class represents an Octopipes Client and it's used to implement a client.

The client has as attributes the callbacks:

```cpp
std::function<void(const Client*, const Message*)> on_received;
std::function<void(const Client*, const Message*)> on_sent;
std::function<void(const Client*, const Error)> on_receive_error;
std::function<void(const Client*)> on_subscribed;
std::function<void(const Client*)> on_unsubscribed;
```

a pointer to an OctopipesClient struct

```cpp
void* octopipes_client;
```

and some user data which can be stored:

```cpp
void* user_data;
```

Let's see the class methods in the details; for each method one or more errors can be returned. Check liboctopipes to see exactly which error can be returned for each method.

#### Client class constructors

```cpp
Client(const std::string& client_id, const std::string& cap_path, const ProtocolVersion version);
Client(const std::string& client_id, const std::string& cap_path, const ProtocolVersion version, void* user_data);
```

To allocate a new Client just provide a client id, the CAP path and the protocol version to use.

#### startLoop

*public*  
Starts the client listener. The client must be subscribed to start it.

```cpp
Error startLoop();
```

#### stopLoop

*public*  
Stops the client listener. The client must be unsubscribed to stop it.

```cpp
Error stopLoop();
```

#### subscribe

*public*  
Subscribes to the server.

```cpp
Error subscribe(const std::list<std::string>& groups, CapError& assignment_error);
```

#### unsubscribe

*public*  
Unsubscribes from the server.

```cpp
Error unsubscribe();
```

#### send

*public*  
Sends a message to a certain remote.

```cpp
Error send(const std::string& remote, const void* data, const uint64_t data_size);
```

#### send_ex

*public*  
Sends a message to a certain remote with options and TTL.

```cpp
Error sendEx(const std::string& remote, const void* data, const uint64_t data_size, const uint8_t ttl, const Options options);
```

#### Callback Setters

These methods are used to set the callbacks. Check the liboctopipes documentation to see what they actually are used for

```cpp
Error setReceivedCB(std::function<void(const Client*, const Message*)> on_received);
Error setSentCB(std::function<void(const Client*, const Message*)> on_sent);
Error setReceive_errorCB(std::function<void(const Client*, const Error)> on_receive_error);
Error setSubscribedCB(std::function<void(const Client*)> on_subscribed);
Error setUnsubscribedCB(std::function<void(const Client*)> on_unsubscribed);
```

#### Client Getters

These methods are used to retrieve exactly what their names say.

```cpp
void* getUserData() const;
std::string getClientId() const;
std::string getCapPath() const;
std::string getPipeTx() const;
std::string getPipeRx() const;
```

It's possible to get the description of Error calling:

```cpp
static const std::string getErrorDesc(const Error error);
```

### Message (message.hpp)

The Message class is a C++ wrapper for an OctopipesMessage struct.

#### Message class constructors

```cpp
Message(); //Instantiate an empty message
Message(const ProtocolVersion version, const std::string& origin, const std::string& remote, const uint8_t* payload, const size_t payload_size, const Options options, const int ttl); //Instantiate a Message with predefined parameters
Message(const void* octopipes_message); //Instantiate a Message starting from an OctopipesMessage pointer
```

#### Message Encoding/Decoding

Use these functions to encode or decode a message:

```cpp
Error decodeData(const uint8_t* data, size_t data_size);
Error encodeData(uint8_t*& data, size_t& data_size);
```

#### Message getters

```cpp
ProtocolVersion getVersion() const;
const std::string getOrigin() const;
const std::string getRemote() const;
const uint8_t* getPayload(size_t& data_size) const;
const int getTTL() const;
const int getChecksum() const;
bool getOption(const Options option) const;
```

### Server (server.hpp)

The server class represents an OctopipesServer and it must be used indeed to implement an OctopipesServer.

#### Server class constructor

```cpp
Server(const std::string& cap_path, const std::string& client_dir, const ProtocolVersion version);
```

#### startCapListener

*public*  
This method must be used to start the CAP listener, which is the thread which checks for incoming messages on the CAP.

```cpp
ServerError startCapListener();
```

#### stopCapListener

*public*  
This method must be used to stop the CAP listener.

```cpp
ServerError stopCapListener();
```

#### processCapOnce

*public*  
This methods process, if possible the first message available on the CAP inbox. This method takes also care of handling subscriptions, unsubscriptions and assignations itself.

```cpp
ServerError processCapOnce(size_t& requests);
```

#### processCapAll

*public*  
This methods process, if possible all the messages available on the CAP inbox, until none is remaining. This method takes also care of handling subscriptions, unsubscriptions and assignations itself.

```cpp
ServerError processCapAll(size_t& requests);
```

#### startWorker

*public/unsafe*  
Starts a new worker whith a certain name
**WARNING**: this function should be considered UNSAFE. The CAP process functions already take care of this task for a new subscription, so this should only used to force a subscription of a certain client.

```cpp
ServerError startWorker(const std::string& client, const std::list<std::string>& subscriptions, const std::string& cli_tx_pipe, const std::string& cli_rx_pipe);
```

#### stopWorker

*public, unsafe*  
Stops a running worker with a certain name
**WARNING**: this function should be considered UNSAFE. The CAP process functions already take care of this task in case of an unsubscription, so this should only used to force an unsubscription of a certain client.

```cpp
ServerError stopWorker(const std::string& client);
```

#### processFirst

*public*  
Process the first available message on any worker inbox.

```cpp
ServerError processFirst(size_t& requests, std::string& client);
```

#### processOnce

*public*  
Process one message for each worker inbox (if possible).

```cpp
ServerError processOnce(size_t& requests, std::string& client);
```

#### processAll

*public*  
Process all the available messages on each worker inbox.
**WARNING**: octopipes_server_process_once should be preferred

```cpp
ServerError processAll(size_t& requests, std::string& client);
```

#### Server getters

```cpp
bool isSubscribed(const std::string& client);
std::list<std::string> getSubscriptions(const std::string& client);
std::list<std::string> getClients();
```

It is finally possible to retrieve the description of a server error calling:

```cpp
static const std::string getServerErrorDesc(const ServerError error);
```

---

## Changelog

---

## License

```txt
MIT License

Copyright (c) 2019-2020 Christian Visintin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
