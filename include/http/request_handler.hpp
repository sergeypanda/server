//
// request_handler.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER3_REQUEST_HANDLER_HPP
#define HTTP_SERVER3_REQUEST_HANDLER_HPP

#include <string>
#include <boost/noncopyable.hpp>

// #ifndef FUCKINGBITCH
// #include "server.hpp"
// #endif

// #ifndef SSLFUCKINGBITCH
// #include "ssl_server.hpp"
// #endif


struct reply;
struct request;
class TQueue;
//class TSocketManager;

/// The common handler for all incoming requests.
class request_handler
  : private boost::noncopyable
{
public:

 // TSocketManager * owner;
	TQueue * queue;

  /// Construct with a directory containing files to be served.
  explicit request_handler(const std::string& doc_root, TQueue * owner);

  /// Handle a request and produce a reply.
  void handle_request(request * req, reply& rep);

private:
  /// The directory containing the files to be served.
  std::string doc_root_;

  /// Perform URL-decoding on a string. Returns false if the encoding was
  /// invalid.
  static bool url_decode(const std::string& in, std::string& out);
};

 // namespace server3
 // namespace http

#endif // HTTP_SERVER3_REQUEST_HANDLER_HPP
