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

Dependencies:

- pthread
- liboctopipespp

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

TBD

## Documentation

TODO
