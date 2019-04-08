#include <fcntl.h>
// for network code
#include <netdb.h>
#include <unistd.h>

// for functional code
#include <functional>

// for the main function
#include <iostream>
#include <optional>
#include <vector>
#include <future>
#include <algorithm>

#include <list>

#include <thread>




namespace tp {



std::vector < std::future<void> > mock_server(int port, std::function < void(int connected_socket, char const *host, char const *service) > on_connection, std::function < void(int listening_socket) > on_listen) {
  using namespace std;
  const char * server_name = "localhost";
  const char * port_name = to_string(port).c_str();
  const int max_queue = 100;

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
    throw invalid_argument(gai_strerror(s));
  }

  std::vector<int> listening_sockets;
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    listening_socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (listening_socket != -1) {
      if (int yes = 1; setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR,
                                  &yes, sizeof(yes)) == -1) {
        throw invalid_argument("setsockopt( ... ) error");
      }
      if (bind(listening_socket, rp->ai_addr, rp->ai_addrlen) == 0) {
        if (listen(listening_socket, max_queue) == -1) {
          ::close(listening_socket);
          throw invalid_argument("listen error after bind of address");
        } else {
          listening_sockets.push_back(listening_socket);
        }
      } else {
        ::close(listening_socket);
      }
    }
  }
  freeaddrinfo(result);
  std::vector < std::future<void> > listening_threads;
  if (listening_sockets.size()) {
    for (auto sockfd : listening_sockets) {

      auto t = std::async(std::launch::async, [=]() {
        int flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
        bool do_the_job = true;
        int delay_next_accept = 1;
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
            on_connection(connected_socket, host, service);
            ::close(connected_socket);
            delay_next_accept = 1;
          }
        }
      });
      on_listen(sockfd);
      //t.detach();
      listening_threads.push_back(std::move(t));
    }
    //for (auto &t: listening_threads) {t.detach();}
    return std::move(listening_threads);
  } return {};
}

}

