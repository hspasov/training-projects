#ifndef CLIENT_CONNECTION_HPP
#define CLIENT_CONNECTION_HPP

#include <string>
#include <iostream>
#include "socket.hpp"
#include "logger.hpp"
#include "config.hpp"
#include "http_msg_formatter.hpp"

enum client_conn_state {
  ESTABLISHED,
  RECEIVING,
  SENDING,
  SHUTDOWN,
  CLOSED
};

class ClientConnection {
  protected:
    Socket conn;
    std::string req_meta_raw;
  public:
    request_meta req_meta;
    client_conn_state state;
    ClientConnection (const int conn)
      : conn(Socket(conn)), state(ESTABLISHED) {

    }
    // TODO later ~ClientConnection();

    // TODO check why Socket cant be passed by reference

    void receive_meta () {
      error_log_fields fields = { DEBUG };
      Logger::error(fields);

      this->state = RECEIVING;

      while (true) {
        if (this->req_meta_raw.length() > Config::config["req_meta_limit"].GetUint()) {
          // TODO send 400
          return;
        }

        error_log_fields fields = { DEBUG };
        fields.msg = "receiving data...";
        Logger::error(fields);

        this->conn.receive();

        this->req_meta_raw.append(this->conn.buffer, this->conn.bytes_received_amount);

        if (this->conn.bytes_received_amount == 0) {
          error_log_fields fields = { DEBUG };
          fields.msg = "connection closed by peer";
          Logger::error(fields);
          // TODO handle
        }

        size_t double_crlf_pos = this->req_meta_raw.find("\r\n\r\n");

        if (double_crlf_pos != std::string::npos) {
          error_log_fields fields = { DEBUG };
          fields.msg = "reached end of request meta";
          Logger::error(fields);

          std::string body_beg = this->req_meta_raw.substr(double_crlf_pos);
          body_beg.erase(0, 4); // remove CR-LF-CR-LF at the beginning
          strcpy(this->conn.buffer, body_beg.c_str());
          this->conn.bytes_received_amount = body_beg.length();

          this->req_meta_raw = this->req_meta_raw.substr(0, double_crlf_pos);

          break;
        }
      }

      fields.msg = this->req_meta_raw;
      Logger::error(fields);

      fields.msg = "Parsing request msg..";
      Logger::error(fields);

      this->req_meta = http_msg_formatter::parse_req_meta(this->req_meta_raw);

      // TODO refactor this
      std::string req_meta_stringified = "method: ";
      req_meta_stringified += this->req_meta.method;
      req_meta_stringified += "; target: ";
      req_meta_stringified += this->req_meta.target;
      req_meta_stringified += "; path: ";
      req_meta_stringified += this->req_meta.path;
      req_meta_stringified += "; query_string: ";
      req_meta_stringified += this->req_meta.query_string;
      req_meta_stringified += "; http_version: ";
      req_meta_stringified += this->req_meta.http_version;
      req_meta_stringified += "; user agent: ";
      req_meta_stringified += this->req_meta.user_agent;

      fields.msg = req_meta_stringified;
      Logger::error(fields);
    }

    void serve_static_file(const std::string path) {
      // TODO add traces

      error_log_fields fields = { DEBUG };
      fields.var_name = "path";
      fields.var_value = web_server_utils::resolve_static_file_path(path).c_str();
      Logger::error(fields);

      const int fd = open(web_server_utils::resolve_static_file_path(path).c_str(), O_RDONLY);

      if (fd < 0) {
        // TODO handle file does not exist, file is a dir...
        throw Error(ERROR, "open: " + std::string(std::strerror(errno)));
      }

      fields.msg = "requested file opened";
      Logger::error(fields);

      if (close(fd) < 0) {
        throw Error(ERROR, "close: " + std::string(std::strerror(errno)));
      }
    }

    void send_meta();
};

#endif