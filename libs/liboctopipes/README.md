# LibOctopipes

Current Version: 0.1.0 (??/??/2020)
Developed by *Christian Visintin*

LibOctopipes is a C library which provides functions to implement Octopipes server and clients.

- [LibOctopipes](#liboctopipes)
  - [Build](#build)
  - [Client Implementation](#client-implementation)
  - [Server Implementation](#server-implementation)
  - [Documentation](#documentation)
  - [Changelog](#changelog)
  - [License](#license)

## Build

Dependencies:

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

```c
OctopipesClient* client;
if ((rc = octopipes_init(&client, client_id, cap_path, protocol_version)) != OCTOPIPES_ERROR_SUCCESS) {
  //Handle error
}
```

Set callbacks (optional)

```c
octopipes_set_received_cb(client, on_received);
octopipes_set_receive_error_cb(client, on_error);
octopipes_set_sent_cb(client, on_sent);
octopipes_set_subscribed_cb(client, on_subscribed);
octopipes_set_unsubscribed_cb(client, on_unsubscribed);

//All callbacks takes in an OctopipesClient; received and sent takes also an OctopipesMessage*, while receive_error the returned error from receive:

OctopipesError octopipes_set_received_cb(OctopipesClient* client, void (*on_received)(const OctopipesClient* client, const OctopipesMessage*));
OctopipesError octopipes_set_sent_cb(OctopipesClient* client, void (*on_sent)(const OctopipesClient* client, const OctopipesMessage*));
OctopipesError octopipes_set_receive_error_cb(OctopipesClient* client, void (*on_receive_error)(const OctopipesClient* client, const OctopipesError));
OctopipesError octopipes_set_subscribed_cb(OctopipesClient* client, void (*on_subscribed)(const OctopipesClient* client));
OctopipesError octopipes_set_unsubscribed_cb(OctopipesClient* client, void (*on_unsubscribed)(const OctopipesClient* client));
```

Subscribe to server

```c
OctopipesCapError cap_error;
if ((rc = octopipes_subscribe(client, groups, groups_amount, &cap_error)) != OCTOPIPES_ERROR_SUCCESS) {
  //Handle error
  //Check also OctopipesCapError here
}
```

Start loop (after that you will start receiving messages through the on_received callback):

```c
if ((rc = octopipes_loop_start(client)) != OCTOPIPES_ERROR_SUCCESS) {
  //Handle error
}
```

Send messages

```c
if ((rc = octopipes_send(client, remote, (void*) data, data_size)) != OCTOPIPES_ERROR_SUCCESS) {
  //Handle error
}
```

Unsubscribe

```c
if ((rc = octopipes_unsubscribe(client)) != OCTOPIPES_ERROR_SUCCESS) {
  //Handle error
}
```

Free resources

```c
octopipes_cleanup(client);
```

Two clients implementation can be found in and are provided with liboctopipes [Here](https://github.com/ChristianVisintin/Octopipes/tree/master/libs/liboctopipes/clients)

## Server Implementation

Initialize the server.
These parameters are required:

- CAP pipe path
- Directory where the clients pipes will be stored (if doesn't exist, will be created)
- Protocol version

```c
OctopipesServer* octopipesServer;
if ((error = octopipes_server_init(&octopipesServer, cap_path, client_folder, OCTOPIPES_VERSION_1)) != OCTOPIPES_SERVER_ERROR_SUCCESS) {
  printf("Could not allocate server: %s\n", octopipes_server_get_error_desc(error));
}
```

Start the CAP listener (in order to receive message on the CAP)

```c
if ((error = octopipes_server_start_cap_listener(server)) != OCTOPIPES_SERVER_ERROR_SUCCESS) {
  printf("Could not start CAP listener: %s\n", octopipes_server_get_error_desc(error));
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

```c
while (true) {
  //Process CAP
  size_t requests = 0;
  if ((error = octopipes_server_process_cap_oncee(server, &requests)) != OCTOPIPES_SERVER_ERROR_SUCCESS) {
    printf("Error while processing CAP: %s\n", octopipes_server_get_error_desc(error));
  }
  //Process clients
  const char* faultClient;
  if ((error = octopipes_server_process_once(server, &requests, &faultClient)) != octopipes::ServerError::SUCCESS) {
    printf("Error while processing CLIENT %s: %s\n", faultClient, octopipes_server_get_error_desc(error));
  }
  if (requests > 0) {
    printf("Processed %zu requests\n", requests);
  }
  usleep(100000); //100ms
}
```

Stop server
It's enough to delete the server object.

```c
octopipes_server_cleanup(server);
```

## Documentation

TODO

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
