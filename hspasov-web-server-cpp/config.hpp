#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <iostream>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include "rapidjson/document.h"
#include "rapidjson/schema.h"
#include "error.hpp"

// TODO maybe it should not be global like this
// TODO add minimum and maximum for config parameters
// TODO check if host config option can also be ip addr

class Config {
  public:
    static rapidjson::Document config;

    static void init_config (const std::string config_file) {
      const std::string config_file_schema_path = "./config_schema.json";
      rapidjson::Document config_schema_document;

      const std::string config_schema_raw = Config::read_config_file(config_file_schema_path);

      if (config_schema_document.Parse(config_schema_raw.c_str()).HasParseError()) {
        std::cerr << "JSON parsing error: " << std::endl; // TODO show where the error is
        exit(-1);
      }

      const rapidjson::SchemaDocument config_schema(config_schema_document);
      rapidjson::SchemaValidator config_schema_validator(config_schema);

      const std::string config_raw = Config::read_config_file(config_file);

      if (Config::config.Parse(config_raw.c_str()).HasParseError()) {
        std::cerr << "JSON parsing error: " << std::endl; // TODO show where the error is
        exit(-1);
      }

      if (!Config::config.Accept(config_schema_validator)) {
        std::cerr << "JSON validation error: " << std::endl; // TODO show where the error is
      }
    }

    static std::string read_config_file (const std::string file_path) {
      // TODO add file size limit assert

      const int fd = open(file_path.c_str(), O_RDONLY);

      if (fd < 0) {
        // TODO check if file not exists
        throw Error(OSERR, "open: " + std::string(std::strerror(errno)));
      }

      std::string file_content;

      while (true) {
        const int buff_size = 10;
        char buffer[buff_size];
        const ssize_t bytes_read_amount = read(fd, buffer, buff_size);

        if (bytes_read_amount == 0) {
          break;
        } else if (bytes_read_amount < 0) {
          throw Error(OSERR, "read: " + std::string(std::strerror(errno)));
        } else {
          file_content.append(buffer, bytes_read_amount);
        }
      }

      if (close(fd) < 0) {
        throw Error(OSERR, "close: " + std::string(std::strerror(errno)));
      }

      return file_content;
    }
};

rapidjson::Document Config::config;

#endif
