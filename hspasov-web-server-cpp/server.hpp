#ifndef SERVER_HPP
#define SERVER_HPP

#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "socket.hpp"
#include "client_connection.hpp"
#include "logger.hpp"
#include "error.hpp"
#include "config.hpp"

class Server {
  protected:
    int socket_fd;
  public:
    Server () {
      // 0 is for protocol: "only a single protocol exists to support a particular socket type within a given protocol family, in which case protocol can be specified as 0" - from man page
      this->socket_fd = socket(AF_INET, SOCK_STREAM, 0);

      if (this->socket_fd < 0) {
        error_log_fields fields = { ERROR };
        fields.msg = "socket: " + std::string(std::strerror(errno));
        Logger::error(fields);

        throw Error(OSERR, "socket: " + std::string(std::strerror(errno)));
      }

      // setting socket options:

      const int on = 1;

      if (setsockopt(this->socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        error_log_fields fields = { ERROR };
        fields.msg = "setsockopt: " + std::string(std::strerror(errno));
        Logger::error(fields);

        // because destructor would not be called after throw in constructor
        if (close(this->socket_fd) < 0) {
          error_log_fields fields = { ERROR };
          fields.msg = "close: " + std::string(std::strerror(errno));
          Logger::error(fields);
        }

        throw Error(OSERR, "setsockopt: " + std::string(std::strerror(errno)));
      }
    }

    ClientConnection accept () {
      error_log_fields fields = { DEBUG };
      Logger::error(fields);

      sockaddr addr;
      socklen_t addrlen = sizeof(addr);

      int client_conn_fd = ::accept(this->socket_fd, &addr, &addrlen);

      if (client_conn_fd < 0) {
        error_log_fields fields = { ERROR };
        fields.msg = "accept: " + std::string(std::strerror(errno));
        Logger::error(fields);

        throw Error(OSERR, "accept: " + std::string(std::strerror(errno)));
      }

      // TODO put addr in ClientConnection

      return ClientConnection(client_conn_fd);
    }

    void run () {
      // https://en.wikipedia.org/wiki/Type_punning#Sockets_example
      in_addr host;

      int inet_pton_result = inet_pton(AF_INET, Config::config["host"].GetString(), &host);

      // TODO resolve host to ip address using getaddrinfo
      if (inet_pton_result < 0) {
        error_log_fields fields = { ERROR };
        fields.msg = "inet_pton: " + std::string(std::strerror(errno));
        Logger::error(fields);

        throw Error(OSERR, "inet_pton: " + std::string(std::strerror(errno)));
      } else if (inet_pton_result == 0) {
        throw Error(SERVERERR, "inet_pton got invalid network address");
      }

      sockaddr_in sa;
      sa.sin_family = AF_INET;
      sa.sin_port = htons(Config::config["port"].GetInt()); // host-to-network short. Makes sure number is stored in network byte order in memory, that means big-endian format (most significant byte comes first)
      sa.sin_addr = host;

      if (bind(this->socket_fd, (sockaddr*)&sa, sizeof(sa)) < 0) {
        error_log_fields fields = { ERROR };
        fields.msg = "bind: " + std::string(std::strerror(errno));
        Logger::error(fields);

        throw Error(OSERR, "bind: " + std::string(std::strerror(errno)));
      }

      if (listen(this->socket_fd, Config::config["backlog"].GetInt()) < 0) {
        error_log_fields fields = { ERROR };
        fields.msg = "listen: " + std::string(std::strerror(errno));
        Logger::error(fields);

        throw Error(OSERR, "listen: " + std::string(std::strerror(errno)));
      }

      error_log_fields fields = { DEBUG };
      fields.msg = "Listening on " + std::to_string(Config::config["port"].GetInt());
      Logger::error(fields);

      while (true) {
        ClientConnection client_conn = this->accept();

        // TODO fork

        try {
          error_log_fields fields = { DEBUG };
          fields.msg = "connection accepted";
          Logger::error(fields);

          client_conn.receive_meta();

          if (client_conn.state == RECEIVING) {
            client_conn.serve_static_file(client_conn.req_meta.path);
          }

          try {
            client_conn.shutdown();
          } catch (const Error err) {
            if (err._type == CLIENTERR) {
              error_log_fields fields = { DEBUG };
              fields.msg = "client already disconnected";
              Logger::error(fields);
            } else {
              throw err;
            }
          }

        } catch (const Error err) {
          // TODO
        }
        // TODO exit child process
      }
    }

    ~Server () {
      if (close(this->socket_fd) < 0) {
        error_log_fields fields = { ERROR };
        fields.msg = "close: " + std::string(std::strerror(errno));
        Logger::error(fields);
      }
    }
};

#endif
