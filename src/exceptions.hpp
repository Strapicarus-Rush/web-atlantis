#pragma once

#include <stdexcept>
#include <string>
#include <sstream>

class BaseException : public std::runtime_error {
public:
    BaseException(const std::string& type,
                  const std::string& message,
                  const char* file,
                  int line)
        : std::runtime_error(format(type, message, file, line)) {}

private:
    static std::string format(const std::string& type,
                              const std::string& message,
                              const char* file,
                              int line) {
        std::ostringstream oss;
        oss << "[" << type << "] " << message
            << " (" << file << ":" << line << ")";
        return oss.str();
    }
};


class DBException : public BaseException {
public:
    DBException(const std::string& msg, const char* file, int line)
        : BaseException("DBException", msg, file, line) {}
};

class AuthException : public BaseException {
public:
    AuthException(const std::string& msg, const char* file, int line)
        : BaseException("AuthException", msg, file, line) {}
};

#define THROW_DB(msg)    throw DBException((msg), __FILE__, __LINE__)
#define THROW_AUTH(msg)  throw AuthException((msg), __FILE__, __LINE__)