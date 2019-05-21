/**
 * Example http server
 *
 * **/

#include <tepsoc.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <regex>
#include <sstream>
#include <thread>

enum url_mapping_method_e { GET, POST, PUT };
enum input_stage_e { HTTP, HEADER, BODY };
struct request_t {
  std::string method;
  std::string originalUrl;
  std::string http_version;
  input_stage_e stage;
  std::map<std::string, std::string> headers;
  std::list<std::string> req_lines;
  std::string last_req_line;
  std::size_t content_length;
  tp::net::socket_p connection;
};
struct response_t {
  tp::net::socket_p connection;
  std::string val;
  void send(std::string s) {val = s;}
};
using request_p = std::shared_ptr<request_t>;
using response_p = std::shared_ptr<response_t>;

using mapping_callback_f = std::function<void(request_p, response_p)>;

using mapping_callback_f = std::function<void(request_p, response_p)>;

class http_express_t {
public:
  std::map<std::string,
           std::list<std::pair<std::string, mapping_callback_f>>>
      mappings;
  tp::net::server srv;

  void on_header_finished(tp::net::socket_p s, request_p req) {
    std::cout << "header complete: " << req->method << " " << req->originalUrl
              << " " << req->http_version << std::endl;
    for (auto &[k, v] : req->headers) {
      std::cout << "'" << k << "': '" << v << "'" << std::endl;
    }
    req->stage = BODY;
  }
  void on_request_finished(tp::net::socket_p s, request_p req,response_p res) {
    if (mappings[req->method].size() > 0) {

      mappings[req->method].back().second(req,res);
      s->end(res->val);
    } else {
      s->end(std::string("HTTP/1.1 200 OK\r\n") +
             std::string("Date: Mon, 27 Jul 2009 12:28:53 GMT\r\n") +
             std::string("Content-Type: text/html\r\n") + std::string("\r\n") +
             std::string("<html>\r\n") + std::string("<body>\r\n") +
             std::string("<h1>Not found</h1>\r\n") +
             std::string("</body>\r\n") + std::string("</html>\r\n"));
    }
  }

  std::map<input_stage_e, std::function<void(std::string &data, request_p)>>
      handle_incoming_data_map;

  void on_incoming_data(tp::net::socket_p s, request_p req, response_p res,std::string &str) {
    for (auto &c : str) {
      req->last_req_line = req->last_req_line + c;
      switch (req->stage) {
      case HTTP:
        if ((req->last_req_line.size() > 1) &&
            (req->last_req_line.substr(req->last_req_line.size() - 2) ==
             "\r\n")) {

          std::string line(req->last_req_line.begin(),
                           req->last_req_line.end() - 2);
          static const std::regex rgx(" ");
          std::sregex_token_iterator iter(line.begin(), line.end(), rgx, -1);
          std::sregex_token_iterator end;
          req->method = *iter;
          ++iter;
          if (iter == end)
            throw std::invalid_argument("first http line bad (1)");
          req->originalUrl = *iter;
          ++iter;
          if (iter == end)
            throw std::invalid_argument("first http line bad (2)");
          req->http_version = *iter;
          ++iter;
          req->stage = HEADER;
          req->last_req_line = "";
        }

        break;
      case HEADER:
        if ((req->last_req_line.size() > 1) &&
            (req->last_req_line.substr(req->last_req_line.size() - 2) ==
             "\r\n")) {
          std::string line(req->last_req_line.begin(),
                           req->last_req_line.end() - 2);
          if (line.size() > 0) {
            int i;
            std::string k = line.substr(0, i = line.find_first_of(":"));
            std::string v = line.substr(i + 2);
            std::transform(k.begin(), k.end(), k.begin(), ::tolower);
            req->headers[k] = v;
          } else {
            on_header_finished(s, req);
            req->stage = BODY;
          }
          req->req_lines.push_back(line);
          req->last_req_line = "";
        }
        break;
      case BODY:
        req->content_length += 1;
        break;
      }
    }
    if (req->stage == BODY) {
      if ((req->headers.count("content-length") == 0) ||
          (std::stol(req->headers.at("content-length")) == req->content_length))
        on_request_finished(s, req, res);
    }
  }

  http_express_t &listen(int port) {
    using namespace tp::net;
    srv.on(CONNECTION, [&](tp::net::socket_p s) {
      request_p req = std::make_shared<request_t>();
      response_p res = std::make_shared<response_t>();
      req->connection = s;
      res->connection = s;
      std::cout << "client connected on socket " << s->get_wrapped_socket()
                << std::endl;
      s->on(DATA, [=](std::string str) {
        req->stage = HTTP;
        on_incoming_data(s, req,res, str);
      });
      s->on(END,
            [s, req]() { std::cout << "client clsed channel" << std::endl; });
    });
    srv.on(LISTENING,
           [](int port, std::string addr) {
             std::cout << "listening HTTP: " << port << " on addr " << addr
                       << std::endl;
           })
        .listen(port);
    return *this;
  }

  http_express_t &get(std::string mapping, mapping_callback_f f) {
    mappings["GET"].push_back({mapping, f});
    return *this;
  }
};

http_express_t http_express() { return {}; }

int main() {

  auto app = http_express();

  app.get("/", [](request_p req, response_p res) { res->send("BLA"); });

  app.listen(9010);
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}