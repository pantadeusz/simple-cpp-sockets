#include <fcntl.h>
#include <simplecppsockets.hpp>
// for network code
#include <netdb.h>
#include <unistd.h>

// for functional code
#include <functional>

// for the main function
#include <iostream>
#include <optional>
#include <vector>

#include <list>

namespace tp {

/**
 * @brief function that performs accepting of connections.
 *
 * @param listening_socket the correct listening socket
 * @param on_connection callback that will get (connected socket, connected host name,
 * connected service name)
 * @param error callback on error
 *
 * @return connected or failed socket descriptor
 */
// std::future<std::optional<simplesocket_p> > listeningsocket::accept(
//              std::function<void(simplesocket_p, std::string, std::string)>
//              on_connection, /* connected_socket, host, service */
//              std::function<void(const char *)> error) {
//  return std::async(std::launch::async, [=](){
//  });
//}

/**
 * @brief create listening socket
 *
 * @param on_connection callback that receive bind-ed socket
 * @param error callback on error
 * @param server_name the address on which we should listen
 * @param port_name port on which we listen
 * @param max_queue the number of waiting connections
 */

std::vector<std::future<void>>
accept_connections(std::function<void(int, std::string, std::string)> on_connection,
                   const char *server_name, const char *port_name,
                   const int max_queue,
                   std::function<void(int)> on_listen_socket,
                   std::function<void(const char *)> error) {
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

  struct addrinfo *result, *rp;
  if (int s = getaddrinfo(server_name, port_name, &hints, &result); s != 0) {
    error(gai_strerror(s));
  }

  std::vector<int> listening_sockets;
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    listening_socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (listening_socket != -1) {
      if (int yes = 1; setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR,
                                  &yes, sizeof(yes)) == -1) {
        error("setsockopt( ... ) error");
      }
      if (bind(listening_socket, rp->ai_addr, rp->ai_addrlen) == 0) {
        if (listen(listening_socket, max_queue) == -1) {
          ::close(listening_socket);
          error("listen error after bind of address");
        } else {
          listening_sockets.push_back(listening_socket);
        }
      } else {
        ::close(listening_socket);
      }
    }
  }
  freeaddrinfo(result);
  if (listening_sockets.size()) {
    std::vector<std::future<void>> ret;
    for (auto sockfd : listening_sockets) {
      on_listen_socket(sockfd);
      ret.push_back(std::async(std::launch::async, [=]() {
        int flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
        bool do_the_job = true;
        int delay_next_accept = 1;
        std::list < std::future < void > > tasks_running;
        while (do_the_job) {
          delay_next_accept ++;
          if (delay_next_accept > 1000) delay_next_accept = 1000;
          // listen for all
          struct sockaddr_storage peer_addr;
          socklen_t peer_addr_len = sizeof(struct sockaddr_storage);
          int connected_socket;
          if ((connected_socket =
                   ::accept(sockfd, (struct sockaddr *)&peer_addr,
                            &peer_addr_len)) < 0) {
            // wooudlblock?
            if (errno != EWOULDBLOCK) {
              do_the_job = false;
            }
            std::this_thread::sleep_for (std::chrono::milliseconds(delay_next_accept));
          } else {
            char host[NI_MAXHOST], service[NI_MAXSERV];
            getnameinfo((struct sockaddr *)&peer_addr, peer_addr_len, host,
                        NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV);
            tasks_running.push_back( std::async(std::launch::async, [=]() {
              on_connection(connected_socket, host, service);
              ::close(connected_socket);
            }));
            delay_next_accept = 1;
          }
          tasks_running.erase(std::remove_if (tasks_running.begin(), tasks_running.end(), [](auto &t){
            return t.wait_for(std::chrono::milliseconds(10)) == std::future_status::ready;
          }),tasks_running.end());
        }
        for (auto &t : tasks_running ) {
          t.get();
        }
      }));
    }
    return ret;

  } else {
    error("error binding adress");
    return {};
  }
}

} // namespace tp
