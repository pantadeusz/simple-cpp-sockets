#ifndef __TP_SIMPLECPPSOCKETS_HPP___
#define __TP_SIMPLECPPSOCKETS_HPP___

#include <future>
#include <memory>
#include <optional>
#include <vector>

namespace tp {
std::vector<std::future<void>> accept_connections(std::function<void(int, std::string, std::string)> on_connection,
                   const char *server_name = "0.0.0.0",
                   const char *port_name = "12000", const int max_queue = 32,
                   std::function<void(int)> on_listen_socket = [](auto){},
                   std::function<void(const char *)> error = [](auto){});

}

#endif
