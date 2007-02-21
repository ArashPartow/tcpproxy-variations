//
// tcpproxy_server_v04.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2007 Arash Partow (http://www.partow.net)
// URL: http://www.partow.net/programming/tcpproxy/index.html
//
// Distributed under the Boost Software License, Version 1.0.
//
//
// Variation: Limit the number of concurrent client connections
// In this example the acceptor will limit the number of concurrent
// client connections made to the server. When the limit has been
// reached, any subsequent connections made will be immediately
// disconnected, until such time as the concurrent client count
// has decreased to below the user specified limit.
//
//


#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>


namespace tcp_proxy
{
   namespace ip = boost::asio::ip;

   class safe_counter
   {
   public:

      safe_counter(const std::size_t& max_val,
                   const std::size_t& counter = 0)
      : counter_(counter),
        max_value_(max_val)
      {}

      bool operator < (const std::size_t& v)
      {
         boost::mutex::scoped_lock lock(mutex_);
         return counter_ < v;
      }

      void operator++()
      {
         boost::mutex::scoped_lock lock(mutex_);
         ++counter_;
      }

      void operator--()
      {
         boost::mutex::scoped_lock lock(mutex_);
         --counter_;
      }

      operator std::size_t()
      {
         boost::mutex::scoped_lock lock(mutex_);
         return counter_;
      }

      std::size_t max_value() const
      {
         return max_value_;
      }

      bool compare_and_inc()
      {
         boost::mutex::scoped_lock lock(mutex_);

         if (counter_ >= max_value_)
            return false;

         ++counter_;

         return true;
      }

   private:

      safe_counter(const safe_counter&);
      safe_counter& operator=(const safe_counter&);

      volatile std::size_t counter_;
      std::size_t max_value_;
      boost::mutex mutex_;
   };

   class bridge : public boost::enable_shared_from_this<bridge>
   {
   public:

      typedef ip::tcp::socket socket_type;
      typedef boost::shared_ptr<bridge> ptr_type;

      bridge(boost::asio::io_service& ios, safe_counter& connection_count)
      : downstream_socket_(ios),
        upstream_socket_(ios),
        connection_count_(connection_count),
        upstream_connected_(false)
      {}

      socket_type& downstream_socket()
      {
         return downstream_socket_;
      }

      socket_type& upstream_socket()
      {
         return upstream_socket_;
      }

      void start(const std::string& upstream_host, unsigned short upstream_port)
      {
         upstream_socket_.async_connect(
              ip::tcp::endpoint(
                   boost::asio::ip::address::from_string(upstream_host),
                   upstream_port),
               boost::bind(&bridge::handle_upstream_connect,
                    shared_from_this(),
                    boost::asio::placeholders::error));
      }

      void handle_upstream_connect(const boost::system::error_code& error)
      {
         if (!error)
         {
            if (connection_count_.compare_and_inc())
            {
               upstream_connected_ = true;

               upstream_socket_.async_read_some(
                  boost::asio::buffer(upstream_data_,max_data_length),
                  boost::bind(&bridge::handle_upstream_read,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));

               downstream_socket_.async_read_some(
                  boost::asio::buffer(downstream_data_,max_data_length),
                  boost::bind(&bridge::handle_downstream_read,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
            }
            else
               close();
         }
         else
            close();
      }

   private:

      void handle_downstream_write(const boost::system::error_code& error)
      {
         if (!error)
         {
            upstream_socket_.async_read_some(
                 boost::asio::buffer(upstream_data_,max_data_length),
                 boost::bind(&bridge::handle_upstream_read,
                      shared_from_this(),
                      boost::asio::placeholders::error,
                      boost::asio::placeholders::bytes_transferred));
         }
         else
            close();
      }

      void handle_downstream_read(const boost::system::error_code& error,
                                  const size_t& bytes_transferred)
      {
         if (!error)
         {
            async_write(upstream_socket_,
                  boost::asio::buffer(downstream_data_,bytes_transferred),
                  boost::bind(&bridge::handle_upstream_write,
                        shared_from_this(),
                        boost::asio::placeholders::error));
         }
         else
            close();
      }

      void handle_upstream_write(const boost::system::error_code& error)
      {
         if (!error)
         {
            downstream_socket_.async_read_some(
                 boost::asio::buffer(downstream_data_,max_data_length),
                 boost::bind(&bridge::handle_downstream_read,
                      shared_from_this(),
                      boost::asio::placeholders::error,
                      boost::asio::placeholders::bytes_transferred));
         }
         else
            close();
      }

      void handle_upstream_read(const boost::system::error_code& error,
                                const size_t& bytes_transferred)
      {
         if (!error)
         {
            async_write(downstream_socket_,
                 boost::asio::buffer(upstream_data_,bytes_transferred),
                 boost::bind(&bridge::handle_downstream_write,
                      shared_from_this(),
                      boost::asio::placeholders::error));
         }
         else
            close();
      }

      void close()
      {
         boost::mutex::scoped_lock lock(mutex_);

         if (upstream_connected_)
         {
            --connection_count_;
            upstream_connected_ = false;
         }

         if (downstream_socket_.is_open())
         {
            downstream_socket_.close();
         }

         if (upstream_socket_.is_open())
         {
            upstream_socket_.close();
         }
      }

      socket_type downstream_socket_;
      socket_type upstream_socket_;

      enum { max_data_length = 8192 }; //8KB
      unsigned char downstream_data_[max_data_length];
      unsigned char upstream_data_[max_data_length];

      boost::mutex mutex_;
      safe_counter& connection_count_;
      bool upstream_connected_;

   public:

      class acceptor
      {
      public:

         acceptor(boost::asio::io_service& io_service,
                  const std::size_t& max_connections,
                  const std::string& local_host, unsigned short local_port,
                  const std::string& upstream_host, unsigned short upstream_port)
         : io_service_(io_service),
           localhost_address(boost::asio::ip::address_v4::from_string(local_host)),
           acceptor_(io_service_,ip::tcp::endpoint(localhost_address,local_port)),
           upstream_port_(upstream_port),
           upstream_host_(upstream_host),
           connection_count_(max_connections,0)
         {}

         bool accept_connections()
         {
            try
            {
               session_ = boost::shared_ptr<bridge>(new bridge(io_service_,connection_count_));

               acceptor_.async_accept(session_->downstream_socket(),
                    boost::bind(&acceptor::handle_accept,
                         this,
                         boost::asio::placeholders::error));
            }
            catch(std::exception& e)
            {
               std::cerr << "acceptor exception: " << e.what() << std::endl;
               return false;
            }

            return true;
         }

      private:

         void handle_accept(const boost::system::error_code& error)
         {
            if (!error)
            {
               if (connection_count_ < connection_count_.max_value())
                  session_->start(upstream_host_,upstream_port_);
               else
                  session_->downstream_socket().close();

               if (!accept_connections())
               {
                  std::cerr << "Failure during call to accept." << std::endl;
               }
            }
            else
            {
               std::cerr << "Error: " << error.message() << std::endl;
            }
         }

         boost::asio::io_service& io_service_;
         ip::address_v4 localhost_address;
         ip::tcp::acceptor acceptor_;
         ptr_type session_;
         unsigned short upstream_port_;
         std::string upstream_host_;
         safe_counter connection_count_;
      };

   };
}

int main(int argc, char* argv[])
{
   if (argc != 6)
   {
      std::cerr << "usage: tcpproxy_server <local host ip> <local port> <forward host ip> <forward port> <max connections>" << std::endl;
      return 1;
   }

   const unsigned short local_port   = static_cast<unsigned short>(::atoi(argv[2]));
   const unsigned short forward_port = static_cast<unsigned short>(::atoi(argv[4]));
   const std::string local_host      = argv[1];
   const std::string forward_host    = argv[3];
   const std::size_t max_connections = static_cast<std::size_t>(::atoi(argv[5]));

   boost::asio::io_service ios;

   try
   {
      tcp_proxy::bridge::acceptor acceptor(ios,
                                           max_connections,
                                           local_host, local_port,
                                           forward_host, forward_port);

      acceptor.accept_connections();

      ios.run();
   }
   catch(std::exception& e)
   {
      std::cerr << "Error: " << e.what() << std::endl;
      return 1;
   }

   return 0;
}

/*
 * [Note] On posix systems the tcp proxy server build command is as follows:
 * c++ -pedantic -ansi -Wall -Werror -O3 -o tcpproxy_server_v04 tcpproxy_server_v04.cpp -L/usr/lib -lstdc++ -lpthread -lboost_thread -lboost_system
 */
