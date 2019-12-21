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

#ifndef OCTOPIPES_H
#define OCTOPIPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

//Version defines
#define OCTOPIPES_LIB_VERSION "0.1.0"
#define OCTOPIPES_LIB_VERSION_MAJOR 0
#define OCTOPIPES_LIB_VERSION_MINOR 1

//Functions
//Alloc operations
OctopipesError octopipes_init(OctopipesClient** client, const char* client_id, const char* cap_path, const OctopipesVersion version_to_use);
OctopipesError octopipes_cleanup(OctopipesClient* client);
//Thread operations
OctopipesError octopipes_loop_start(OctopipesClient* client);
OctopipesError octopipes_loop_stop(OctopipesClient* client);
//Cap operartions
OctopipesError octopipes_subscribe(OctopipesClient* client, const char** groups, size_t groups_amount, OctopipesCapError* assignment_error);
OctopipesError octopipes_unsubscribe(OctopipesClient* client);
//Tx operations
OctopipesError octopipes_send(OctopipesClient* client, const char* remote, const void* data, uint64_t data_size);
OctopipesError octopipes_send_ex(OctopipesClient* client, const char* remote, const void* data, uint64_t data_size, const uint8_t ttl, const OctopipesOptions options);
//Callbacks
OctopipesError octopipes_set_received_cb(OctopipesClient* client, void (*on_received)(const OctopipesMessage*));
OctopipesError octopipes_set_sent_cb(OctopipesClient* client, void (*on_sent)(const OctopipesMessage*));
OctopipesError octopipes_set_subscribed_cb(OctopipesClient* client, void (*on_subscribed)());
OctopipesError octopipes_set_unsubscribed_cb(OctopipesClient* client, void (*on_unsubscribed)());
//Errors
const char* octopipes_get_error_desc(const OctopipesError error);

#ifdef __cplusplus
}
#endif

#endif
