# LibOctopipes

Current Version: 0.1.0 (??/??/2020)
Developed by *Christian Visintin*

LibOctopipes is a C++ Wrapper of liboctopipes which provides functions to implement Octopipes server and clients in C++.

- [LibOctopipes](#liboctopipes)
  - [Build](#build)
  - [Client Implementation](#client-implementation)
  - [Server Implementation](#server-implementation)
  - [Documentation](#documentation)

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

TODO
