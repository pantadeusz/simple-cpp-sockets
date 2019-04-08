/**
MIT License

Copyright (c) 2019 Tadeusz Puźniakowski

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

 * */



#include <tepsoc.hpp>

#include <stdexcept>

#include <future>
#include <ios>
#include <iostream>
#include <list>
#include <memory>
#include <thread>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

namespace tp
{
namespace net
{

void socket::_on_connect()
{
  tp::net::socket_event_callback_f cb;
  int cb_count = 0;
  _callback_guard([&]() {
    cb_count = _callbacks.count(socket_event::CONNECT);
    if (cb_count)
      cb = _callbacks.at(socket_event::CONNECT);
  });

  if (cb_count)
  {
    switch (cb.index())
    {
    case 0:
      _handler_guard([&]() { std::get<0>(cb)(); });
      break;
    case 2:
      _handler_guard([&]() { std::get<2>(cb)(*this); });
      break;
    default:
      _on_error("bad CONNECT callback: " + std::to_string(cb.index()));
    }
  }
  _handle_incoming_data();
}

int socket::_on_data(const std::vector<char> &recvbuff)
{
  tp::net::socket_event_callback_f cb;

  cb = get_callback(socket_event::DATA);

  switch (cb.index())
  {
  case 1:
    _handler_guard([&]() { std::get<1>(cb)(std::string(recvbuff.begin(), recvbuff.end())); });
    return 0;
  case 3:
    _handler_guard([&]() { std::get<3>(cb)(recvbuff); });
    return 0;
  default:
    _on_error("bad DATA callback");
    return -1;
  }
}
void socket::_on_error(const std::string err)
{
  tp::net::socket_event_callback_f cb;

  cb = get_callback(socket_event::ERROR);
  switch (cb.index())
  {
  case 0:
    _handler_guard([&]() { std::get<0>(cb)(); });
    return;
  case 1:
    _handler_guard([&]() { std::get<1>(cb)(err); });
    return;
  default:
    _on_error(std::string("socket::_on_error:: Error handler bad:") + std::to_string(cb.index()) + "; err:" + err);
    return;
  }
}
void socket::_on_end()
{
  tp::net::socket_event_callback_f cb;
  cb = get_callback(socket_event::END);
  if (cb.index() == 0)
    _handler_guard([&]() { std::get<0>(cb)(); });
}

void socket::_handle_incoming_data()
{
  int ret = 0;
  std::vector<char> recvbuff(4096);
  std::list<std::vector<char>> backlog;
  // MSG_DONTWAIT
  while (true)
  {
    ret = ::recv(connected_socket, recvbuff.data(), recvbuff.size(), 0);
    if (ret == 0)
      break;
    if (ret == -1)
    {
      if (!((errno == EAGAIN) || (errno == EWOULDBLOCK)))
      {
        break;
      }
    }
    if (ret > 0)
    {
      recvbuff.resize(ret);
      backlog.push_back(recvbuff);
      recvbuff.resize(4096);
    }
    while ((backlog.size() > 0) && (_on_data(backlog.front()) == 0))
    {
      backlog.pop_front();
    }
  }
  if (ret != 0)
  {
    tp::net::socket_event_callback_f cb;
    cb = get_callback(socket_event::ERROR);
    _handler_guard([&]() { std::get<1>(cb)("connection broken"); });
  }
  _on_end();
  ::close(connected_socket);
  connected_socket = -1;
  _active_connection = false;
}

socket &socket::on(const socket_event evnt, socket_event_callback_f f)
{
  _callback_guard([&]() { _callbacks[evnt] = f; });
  return *this;
}

template <class T>
socket &socket_write(socket &sckt, const T &data_)
{
  std::vector<char> data(data_.begin(), data_.end());
  while (data.size() > 0)
  {
    auto s = ::send(sckt.connected_socket, data.data(), data.size(), 0);
    data.erase(data.begin(), data.begin() + std::min((int)s, (int)data.size()));
  }
  return sckt;
}

socket &socket::write(const std::string data_)
{
  return socket_write(*this, data_);
}
socket &socket::write(const std::vector<char> data_)
{
  return socket_write(*this, data_);
}
socket &socket::write(const std::list<char> data_)
{
  return socket_write(*this, data_);
}

socket &socket::end(std::string data)
{
  while (data.size() > 0)
  {
    auto s = ::send(connected_socket, data.data(), data.size(), 0);
    data = data.substr(s, data.size());
  }
  ::shutdown(connected_socket, SHUT_WR);
  return *this;
}

socket &socket::wrap(int connected_socket_)
{
  this->connected_socket = connected_socket_;
  _active_connection = true;
  _on_connect();
  return *this;
}

socket &socket::connect(unsigned int port_, char const *addr_c)
{
  if (connected_socket >= 0)
    throw std::invalid_argument("socket already connected");
  std::string addrr(addr_c);
  std::thread t([=]() {
    char const *addr_ = addrr.c_str();
    struct addrinfo hints;
    std::fill((char *)&hints, (char *)&hints + sizeof(struct addrinfo), 0);
    hints.ai_family = AF_UNSPEC;     ///< IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; ///< stream socket
    hints.ai_flags = 0;              ///< no additional flags
    hints.ai_protocol = 0;           ///< any protocol

    struct addrinfo *addr_p;
    if (int err =
            getaddrinfo(addr_, std::to_string(port_).c_str(), &hints, &addr_p);
        err)
    {
      throw std::invalid_argument(gai_strerror(err));
    }
    struct addrinfo *rp;
    std::shared_ptr<struct addrinfo> addr_sp(
        addr_p, [](struct addrinfo *addr_p) { freeaddrinfo(addr_p); });

    for (rp = addr_p; rp != NULL; rp = rp->ai_next)
    {
      connected_socket =
          ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (connected_socket != -1)
      {
        if (::connect(connected_socket, rp->ai_addr, rp->ai_addrlen) != -1)
        {
          _active_connection = true;
          _on_connect();
          return;
        }
        ::close(connected_socket);
        connected_socket = -1;
      }
    }
    tp::net::socket_event_callback_f cb;
    cb = get_callback(socket_event::ERROR);
    _handler_guard([&]() { std::get<1>(cb)("could not create connection"); });
  });
  t.detach();
  return *this;
}

socket::socket()
{
  static auto signal_ready = ::signal(SIGPIPE, SIG_IGN);
  // if (signal_ready == nullptr) throw std::runtime_error("could not setup
  // signal");
  connected_socket = -1;
  _callbacks[ERROR] = [](std::string err) {
    std::cerr << "socket error: " << err << std::endl;
  };
  _callbacks[CONNECT] = []() {};
  _callbacks[END] = []() {};
  _active_connection = false;
}

socket::~socket()
{
  // std::lock_guard<std::mutex> lock(_in_handler_mutex);
  //::close(connected_socket);
  if (_recv_fut.valid())
    _recv_fut.get();
  {
    std::lock_guard<std::mutex> lock(_in_handler_mutex);
    _active_connection = false;
  }
}

////////////////////// SERVER //////////////////////////////////////////

void server::_on_error(const std::string &err)
{
  tp::net::socket_event_callback_f errcb;

  _callback_guard([&]() { errcb = _callbacks.at(socket_event::ERROR); });
  switch (errcb.index())
  {
  case 0:
    std::get<0>(errcb)();
    ;
    break;
  case 1:
    std::get<1>(errcb)(err);
    break;
  }
}

void server::_on_listen(const unsigned int port_, const std::string addr_)
{
  tp::net::socket_event_callback_f cb;

  cb = get_callback(LISTENING);

  switch (cb.index())
  {
  case 0:
    std::get<0>(cb)();
    break;
  case 4:
    std::get<4>(cb)(port_, addr_);
    break;
  default:
    _on_error("bad LISTEN callback index " + std::to_string(cb.index()));
  }
}

void server::_on_connect(socket &connected_socket_)
{
  tp::net::socket_event_callback_f cb;

  _callback_guard([&]() {
    cb = _callbacks.at(CONNECTION);
  });
  switch (cb.index())
  {
  case 2:
    std::get<2>(cb)(connected_socket_);
    break;
  default:
    _on_error("bad CONNECTION callback index " + std::to_string(cb.index()));
  }
}

server &server::on(socket_event evnt, socket_event_callback_f callback_)
{
  std::lock_guard<std::mutex> lock(_callbacks_mutex);
  _callbacks[evnt] = callback_;
  return *this;
}

void server::_close()
{
  for (auto s : listening_sockets)
    ::close(s);
}

server::server(std::function<void(tp::net::socket &s)> on_connection_)
{
  _callbacks[LISTENING] = []() {};
  _callbacks[ERROR] = [](std::string err) {
    std::cerr << "server::ERROR: " << err << std::endl;
  };
  _callbacks[CONNECTION] = on_connection_; //[](socket &cs) {};
}

server &server::listen(unsigned int port_, char const *server_name)
{
  if (listening_sockets.size())
  {
    _on_error("server already listening.");
    return *this;
  }
  using namespace std;
  std::string port_name_s = to_string(port_);
  const char *port_name = port_name_s.c_str();
  const int max_queue = 100;
  // std::cout << server_name << " : " << port_name << std::endl;
  int listening_socket;
  struct addrinfo hints;
  std::fill((char *)&hints, (char *)&hints + sizeof(struct addrinfo), 0);

  hints.ai_family = AF_UNSPEC;     ///< IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; ///< Stream socket
  hints.ai_flags = AI_PASSIVE;     ///< For wildcard IP address
  hints.ai_protocol = 0;           ///< Any protocol
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  map<int, string> server_names;

  struct addrinfo *result, *rp;
  if (int s = getaddrinfo(server_name, port_name, &hints, &result); s != 0)
  {
    throw invalid_argument(gai_strerror(s));
  }
  std::shared_ptr<struct addrinfo> addrinfo_ptr(
      result, [](auto *p) { freeaddrinfo(p); });

  for (rp = result; rp != NULL; rp = rp->ai_next)
  {
    listening_socket =
        ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (listening_socket != -1)
    {
      if (int yes = 1; setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR,
                                  &yes, sizeof(yes)) == -1)
      {
        // throw invalid_argument("setsockopt( ... ) error");
        _on_error("could not setsockopt"); // continue with error
      }
      if (::bind(listening_socket, rp->ai_addr, rp->ai_addrlen) == 0)
      {
        if (::listen(listening_socket, max_queue) == -1)
        {
          ::close(listening_socket);
          _on_error("listen error after bind of address");
        }
        else
        {
          listening_sockets.push_back(listening_socket);
          char host[NI_MAXHOST], service[NI_MAXSERV];
          getnameinfo((struct sockaddr *)rp->ai_addr, rp->ai_addrlen, host, NI_MAXHOST,
                      service, NI_MAXSERV, NI_NUMERICSERV);
          server_names[listening_socket] = host; // std::string(rp->ai_canonname);
        }
      }
      else
      {
        ::close(listening_socket);
      }
    }
  }

  auto lsc = listening_sockets;
  listening_sockets.clear();
  for (auto sockfd : lsc)
  {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
      ::close(sockfd);
      _on_error(strerror(errno));
    }
    else
    {
      listening_sockets.push_back(sockfd);
      _on_listen(port_, server_names[sockfd]);
    }
  }
  if (listening_sockets.size() == 0)
  {
    _on_error("there are no valid listening sockets");
    return *this;
  }
  accepting_fut = std::async(std::launch::async, [this]() {
    std::mutex _connection_handling_mutex;
    std::map<int, std::thread> _connection_handling_threads;
    std::list<int>
        _connection_handling_threads_finished; // list of finished threads

    // std::list<socket *> connected_sockets;
    while (listening_sockets.size())
    {
      // std::cout << "waiting connection ..." << std::endl;
      std::vector<int> ls;
      for (auto sockfd : listening_sockets)
      {
        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len = sizeof(struct sockaddr_storage);
        int connected_socket;
        if ((connected_socket = ::accept(sockfd, (struct sockaddr *)&peer_addr,
                                         &peer_addr_len)) < 0)
        {
          if (errno == EWOULDBLOCK)
          {
            // artificial delay between connections
            ls.push_back(sockfd);
          }
          else
          {
            // socket was closed...
          }
        }
        else
        {
          ls.push_back(sockfd);
          char host[NI_MAXHOST], service[NI_MAXSERV];
          getnameinfo((struct sockaddr *)&peer_addr, peer_addr_len, host,
                      NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV);

          std::string host_str = host;
          std::string service_str = service;
          _connection_handling_threads[connected_socket] =
              std::thread([this, connected_socket, host_str, service_str,
                           &_connection_handling_threads_finished,
                           &_connection_handling_mutex]() {
                socket connected_socket_obj;
                // std::cout << "on connection .." << std::endl;
                connected_socket_obj.on(CONNECT,
                                        [this, &connected_socket_obj]() {
                                          _on_connect(connected_socket_obj);
                                        });
                connected_socket_obj.wrap(connected_socket);
                std::lock_guard<std::mutex> lock(_connection_handling_mutex);
                _connection_handling_threads_finished.push_back(
                    connected_socket);
              });
        }
      }
      listening_sockets = ls;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      {
        std::lock_guard<std::mutex> lock(_connection_handling_mutex);
        for (auto &e : _connection_handling_threads_finished)
        {
          _connection_handling_threads[e].join();
          _connection_handling_threads.erase(e);
        }
        _connection_handling_threads_finished.clear();
      }
    }
    for (auto &t : _connection_handling_threads)
      t.second.join();
  });
  return *this;
}

} // namespace net
} // namespace tp
