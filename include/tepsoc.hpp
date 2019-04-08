#ifndef __TP__NET__TEPSOC__HPP___
#define __TP__NET__TEPSOC__HPP___

#include <functional>
#include <map>
#include <thread>
#include <variant>
#include <vector>
#include <mutex>
#include <list>

#include <optional>
#include <future>

namespace tp
{
namespace net
{
class socket;
enum socket_event
{
  CONNECT,   // when client socket is connected
  ERROR,     // when error is reported
  DATA,      // data in form of string
  END,       // when connection is about to end
  LISTENING, // when starting listening
  CONNECTION // when someone connects to server socket
};
using socket_event_callback_f =
    std::variant<std::function<void()>,
                 std::function<void(std::string s)>,
                 std::function<void(socket &)>,
                 std::function<void(std::vector<char> v)>,
                 std::function<void(const unsigned int port_, const std::string addr_)>
                 >;

class socket
{
  std::mutex _callbacks_mutex;
  std::mutex _in_handler_mutex;

  inline void _callback_guard(std::function<void()> f)
  {
    std::lock_guard<std::mutex> lock(_callbacks_mutex);
    f();
  }
  inline void _handler_guard(std::function<void()> f)
  {
    std::lock_guard<std::mutex> lock(_in_handler_mutex);
    f();
  }

  inline socket_event_callback_f get_callback(socket_event e){
    std::lock_guard<std::mutex> lock(_callbacks_mutex);
    return _callbacks[e];
  };

  std::atomic<bool> _active_connection;

  std::map<socket_event, socket_event_callback_f> _callbacks;
  int connected_socket;
  std::future<void> _recv_fut;

  void _handle_incoming_data();
  void _on_connect();
  /**
   * @brief handles data
   * 
   * @return 0 if handled. -1 if there is no handler for data
   * 
   */
  int _on_data(const std::vector<char> &recvbuff);
  void _on_end();
  void _on_error(const std::string err);

public:
  /**
 * wraps connected socket into this object. It emits CONNECT event, and then consecutively DATA events
 * */
  socket &wrap(int s);
  /**
   * sets callback for event
   * */
  socket &on(const socket_event evnt, socket_event_callback_f f);
  /**
   * gracefully close connection.
   * */
  socket &end(std::string data_to_send_and_close = "");
  /**
   * write data to socket
   * */
  socket &write(const std::string data);
  /**
   * write data to socket
   * */
  socket &write(const std::vector<char> data);
  /**
   * write data to socket
   * */
  socket &write(const std::list<char> data);

  /**
   * connect to the port in the specified server
   * */
  socket &connect(unsigned int port_, char const *addr_);
  /**
   * check if the connection is still active.
   * Connection can be deactivated by the reading thread (when both sides closes connection).
   * 
   * The initial state of the socket is "not active"
   * */
  bool is_active() { return _active_connection; };

  /**
   * constructs not connected client socket
   * */
  socket();
  /**
   * destructor closes everything and waits for threads to finish
   * */
  virtual ~socket();

  //socket(socket const&) = delete;
  //socket& operator=(socket const&) = delete;
  template <class T>
  friend socket &socket_write(socket &sckt, const T &data_);
};

auto b = [](socket &) -> void {};

class server
{
protected:
  std::map<socket_event, socket_event_callback_f> _callbacks;
  std::mutex _callbacks_mutex;

  inline void _callback_guard(std::function<void()> f)
  {
    std::lock_guard<std::mutex> lock(_callbacks_mutex);
    f();
  }
  //  inline void _handler_guard(std::function<void()>f){
  //    std::lock_guard<std::mutex> lock(_in_handler_mutex);
  //    f();
  //  }
  inline socket_event_callback_f get_callback(socket_event e){
    std::lock_guard<std::mutex> lock(_callbacks_mutex);
    return _callbacks[e];
  };

  std::vector<int> listening_sockets;
  // std::map<int, std::future<void>> connections; // key: connected socket; value socket object

  //std::optional<std::future<void>> accepting_fut;
  std::future<void> accepting_fut;

  void _on_connect(socket &connected_socket_);
  void _on_listen(const unsigned int port_, const std::string addr_);
  void _on_error(const std::string &err);

  void _close();

public:
  /**
 * register callbacks
 * */
  void on(socket_event evnt, socket_event_callback_f callback_);
  /**
   * create server.
   * @arg oc - on connection callback
   * 
   * */
  server(std::function<void(socket &)> oc = b);

  /**
   * starts listening for connections
   * */
  server &listen(unsigned int port_, char const *addr_ = "*");
  /**
   * wait for everything to end
   * */
  virtual ~server()
  {
    _close();
  };
};

} // namespace net
} // namespace tp

#endif
