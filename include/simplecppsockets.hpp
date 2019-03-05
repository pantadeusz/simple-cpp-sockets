#ifndef __TP_SIMPLECPPSOCKETS_HPP___
#define __TP_SIMPLECPPSOCKETS_HPP___

#include <future>
#include <memory>
#include <optional>
#include <vector>

namespace tp {
/**
 * @brief create listening socket
 *
 * @param on_connection callback that receive bind-ed socket
 * @param error callback on error
 * @param server_name the address on which we should listen
 * @param port_name port on which we listen
 * @param max_queue the number of waiting connections
 * @return vector of created listening sockets
 */
std::vector<int> accept_connections(std::function<void(int, std::string, std::string)> on_connection,
                   const char *server_name = "0.0.0.0",
                   const char *port_name = "12000", const int max_queue = 32,
                   std::function<void(int)> on_listen_socket = [](auto){},
                   std::function<void(const char *)> error = [](auto){});



/**
 * @brief opens a connection to given host and port
 *
 * @param addr_txt
 * @param port_txt
 * @param success
 * @param error
 *
 * @return connected socket
 */
int connect_to(const char *addr_txt, const char *port_txt,
               std::function<void(int)> success,
               std::function<void(const char *)> error = [](auto) {});
}

#endif
