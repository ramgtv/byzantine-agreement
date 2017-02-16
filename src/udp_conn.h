#ifndef UDP_CONN_H_
#define UDP_CONN_H_

#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <functional>
#include <iostream>
#include <memory>
#include <string>

#include "net.h"
#include "net_exception.h"

#define BUFSIZE 1024

namespace udp {

typedef int Socket;

// Creates a new socket with the provided timeout.
Socket CreateSocket(struct timeval timeout);

// Wraps a C sockaddr_in with a number of useful functionality.
class SocketAddress {
 public:
  SocketAddress(struct sockaddr_in sockaddr) : addr_(sockaddr){};
  SocketAddress(net::Address addr);

  std::string Hostname() const;
  unsigned short Port() const;

  inline const struct sockaddr* addr() const {
    return (struct sockaddr*)&addr_;
  };
  inline socklen_t addr_len() const { return sizeof(addr_); };

 private:
  struct sockaddr_in addr_;
};

class Client;
typedef std::shared_ptr<const Client> ClientPtr;

// Defines the methods of interacting with a running server.
enum class ServerAction {
  Continue,
  Stop,
};

typedef std::function<ServerAction(ClientPtr, char*, size_t)> OnReceiveFn;
typedef std::function<ServerAction()> OnTimeout;

// Provides an interface to send UDP messages to a remote server.
class Client : public std::enable_shared_from_this<Client> {
 public:
  Client(net::Address addr, struct timeval timeout = {0, 0})
      : sockfd_(CreateSocket(timeout)), remote_address_(addr){};

  Client(struct sockaddr_in sockaddr)
      : sockfd_(CreateSocket({0, 0})), remote_address_(sockaddr){};

  ~Client() { close(sockfd_); };

  // Sends the message to the remote server.
  void Send(const char* buf, size_t size) const;

  // Sends the message to the remote server and waits for an acknowledgement.
  // Will send up to the number of attempts provided, unless attempts = 0, in
  // which case it will continue to send forever until an ack is seen.
  void SendWithAck(const char* buf, size_t size, unsigned int attempts,
                   OnReceiveFn validAck) const;

  // Returns the address of the remote server.
  inline net::Address RemoteAddress() const {
    return net::Address(remote_address_.Hostname(), remote_address_.Port());
  };
  // Returns the hostname of the remote server.
  inline std::string RemoteHostname() const {
    return remote_address_.Hostname();
  };

 private:
  const Socket sockfd_;
  const SocketAddress remote_address_;
};

// Listens for incoming UDP messages.
class Server {
 public:
  Server(unsigned short port, struct timeval timeout = {0, 0});

  ~Server() { close(sockfd_); };

  void Listen(OnReceiveFn rcv, OnTimeout timeout) const;

 private:
  const Socket sockfd_;
};

}  // namespace udp

#endif
