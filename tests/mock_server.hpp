#ifndef __FAKE_SERVER__HPP___
#define __FAKE_SERVER__HPP___

#include <functional>
#include <vector>
#include <thread>
#include <future>



namespace tp {



std::vector < std::future<void> > mock_server(int port, std::function < void(int connected_socket, char const *host, char const *service) > on_connection, std::function < void(int listening_socket) > on_listen);

}


#endif
