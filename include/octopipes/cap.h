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

#ifndef OCTOPIPES_CAP_H
#define OCTOPIPES_CAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

//Prepare
uint8_t* octopipes_cap_prepare_subscription(const char** groups, const size_t groups_size, size_t* data_size);
uint8_t* octopipes_cap_prepare_assign(OctopipesCapError error, const char* fifo_tx, const size_t fifo_tx_size, const char* fifo_rx, const size_t fifo_rx_size, size_t* data_size);
uint8_t* octopipes_cap_prepare_unsubscription(size_t* data_size);
//Parse
OctopipesCapMessage octopipes_cap_get_message(const uint8_t* data, const size_t data_size);
OctopipesError octopipes_cap_parse_subscribe(const uint8_t* data, const size_t data_size, char*** groups, size_t* groups_amount);
OctopipesError octopipes_cap_parse_assign(const uint8_t* data, const size_t data_size, OctopipesCapError* error, char** fifo_tx, char** fifo_rx);
OctopipesError octopipes_cap_parse_unsubscribe(const uint8_t* data, const size_t data_size);

#ifdef __cplusplus
}
#endif

#endif
