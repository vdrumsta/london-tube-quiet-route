#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/system/error_code.hpp>

#include <iomanip>
#include <iostream>
#include <thread>

using tcp = boost::asio::ip::tcp;
using namespace boost::beast;
using namespace boost::beast::websocket;

void Log(boost::system::error_code ec, std::string&& context = "")
{
    std::cerr << context << " "
              << (ec ? "Error: " : "OK")
              << (ec ? ec.message() : "")
              << std::endl;
}

void OnConnect(boost::system::error_code ec)
{
    Log(ec);
}

int main()
{
    // Always start with an I/O context object.
    boost::asio::io_context ioc {};

    // Create an I/O object. Every Boost.Asio I/O object API needs an io_context
    // as the first parameter.
    tcp::socket socket {boost::asio::make_strand(ioc)};

    size_t nThreads {4};

    // Under the hood, socket.connect uses I/O context to talk to the socket
    // and get a response back. The response is saved in ec.
    boost::system::error_code ec {};
    tcp::resolver resolver {ioc};
    std::string url{"echo.websocket.org"};
    auto resolverIt {resolver.resolve(url, "80", ec)};
    if (ec) {   // Checking if resolver endpoint is valid
        Log(ec);
        return -1;
    }
    socket.connect(*resolverIt, ec);
    Log(ec, "TCP connection");
    
    // Tie the socket object to the WebSocket stream and attempt an handshake.
    websocket::stream<boost::beast::tcp_stream> ws {std::move(socket)};
    ws.handshake(url, "/", ec);
    Log(ec, "WebSocket handshake");
    
    // Tell the WebSocket object to exchange messages in text format.
    ws.text(true);

    std::string message{"Hello world!"};
    // Send a message to the connected WebSocket server.
    boost::asio::const_buffer wbuffer {message.c_str(), message.size()};
    ws.write(wbuffer, ec);
    Log(ec, "WebSocket write");

    // Read the echoed message back.
    boost::beast::flat_buffer rbuffer {};
    ws.read(rbuffer, ec);

    // Print the echoed message.
    std::cout << "ECHO: "
            << boost::beast::make_printable(rbuffer.data())
            << std::endl;

    return 0;
}