# LibOctopipes

Current Version: 0.1.0 (??/??/2020)
Developed by *Christian Visintin*

LibOctopipes is a C library which provides functions to implement Octopipes server and clients.

- [LibOctopipes](#liboctopipes)
  - [Build](#build)
  - [Client Implementation](#client-implementation)
  - [Server Implementation](#server-implementation)
  - [Documentation](#documentation)

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

TBD

## Documentation

TODO
