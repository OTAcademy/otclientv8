/*
 * Copyright (c) 2010-2017 OTClient <https://github.com/edubart/otclient>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef FRAMEWORK_NET_DECLARATIONS_H
#define FRAMEWORK_NET_DECLARATIONS_H

#include <framework/global.h>

namespace asio = boost::asio;

class InputMessage;
class OutputMessage;
class Connection;
class Protocol;
class Server;
class PacketPlayer;
class PacketRecorder;

using InputMessagePtr = std::shared_ptr<InputMessage>;
using OutputMessagePtr = std::shared_ptr<OutputMessage>;
using ConnectionPtr = std::shared_ptr<Connection>;
using ProtocolPtr = std::shared_ptr<Protocol>;
using ServerPtr = std::shared_ptr<Server>;
using PacketPlayerPtr = std::shared_ptr<PacketPlayer>;
using PacketRecorderPtr = std::shared_ptr<PacketRecorder>;

#endif
