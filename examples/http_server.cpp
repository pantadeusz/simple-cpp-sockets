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

struct http_request_t {
  std::string method;
  std::string originalUrl;
  std::string http_version;
  bool header_finished;
  std::map<std::string, std::string> headers;
  std::list<std::string> req_lines;
  std::string last_req_line;
  std::size_t content_length;
};

namespace tp {
namespace sthttp {

void on_header_finished(tp::net::socket &s,
                        std::shared_ptr<http_request_t> req) {
  std::cout << "header complete: " << req->method << " " << req->originalUrl
            << " " << req->http_version << std::endl;
  for (auto &[k, v] : req->headers) {
    std::cout << "'" << k << "': '" << v << "'" << std::endl;
  }
  req->header_finished = true;
}

void on_request_finished(tp::net::socket &s,
                         std::shared_ptr<http_request_t> req) {

  s.end(std::string("HTTP/1.1 200 OK\r\n") +
        std::string("Date: Mon, 27 Jul 2009 12:28:53 GMT\r\n") +
        std::string("Content-Type: text/html\r\n") + std::string("\r\n") +
        std::string("<html>\r\n") + std::string("<body>\r\n") +
        std::string("<h1>Hello, World!</h1>\r\n") + std::string("</body>\r\n") +
        std::string("</html>\r\n"));
}

void on_request_line(tp::net::socket &s, std::shared_ptr<http_request_t> req,
                     const std::string &line) {
  std::cout << "have line: '" << line << "'" << std::endl;
  if (req->req_lines.size() == 0) {
    std::regex rgx(" ");
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
  } else if (line.size() > 0) {
    int i;
    std::string k = line.substr(0, i = line.find_first_of(":"));
    std::string v = line.substr(i + 2);
    std::transform(k.begin(), k.end(), k.begin(), ::tolower);
    req->headers[k] = v;
  } else {
    tp::sthttp::on_header_finished(s, req);
  }
  req->req_lines.push_back(line);
}

void on_incoming_data(tp::net::socket &s, std::shared_ptr<http_request_t> req,
                      std::string &str) {
  for (auto &c : str) {
    if (req->header_finished) {
      // todo
      req->content_length += 1;
      //      std::cout << ".. request data not parsed ..." << std::endl;
    } else {
      if (c == '\n') {
        if (req->last_req_line.size() > 0) {
          if (req->last_req_line.back() == '\r') {
            tp::sthttp::on_request_line(
                s, req,
                std::string(req->last_req_line.begin(),
                            req->last_req_line.end() - 1));
            req->last_req_line = "";
            continue;
          }
        }
      }
    }
    req->last_req_line = req->last_req_line + c;
  }
  if (req->header_finished) {
    if ((req->headers.count("content-length") == 0) ||
        (std::stol(req->headers.at("content-length")) == req->content_length))
      tp::sthttp::on_request_finished(s, req);
  }
}
} // namespace sthttp
} // namespace tp

int main() {
  using namespace tp::net;
  tp::net::server srv([&](tp::net::socket &s) {
    std::shared_ptr<http_request_t> req = std::make_shared<http_request_t>();
    std::cout << "client connected" << std::endl;
    s.on(DATA, [=, &s](std::string str) {
      tp::sthttp::on_incoming_data(s, req, str);
    });
    s.on(END,
         [&s, req]() { std::cout << "client clsed channel" << std::endl; });
  });
  srv.on(LISTENING,
         [](int port, std::string addr) {
           std::cout << "listening: " << port << " on addr " << addr
                     << std::endl;
         })
      .listen(2212);
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}