#include <network-monitor/websocket-client.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/test/unit_test.hpp>

#include <string>
#include <filesystem>

using boost::asio::ip::tcp;
using NetworkMonitor::BoostWebSocketClient;
namespace ssl = boost::asio::ssl;

bool CheckResponse(const std::string& response)
{
    // We do not parse the whole message. We only check that it contains some
    // expected items.
    bool ok {true};
    ok &= response.find("ERROR") != std::string::npos;
    ok &= response.find("ValidationInvalidAuth") != std::string::npos;

    std::cout << response << std::endl;
    return ok;
}

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_CASE(class_WebSocketClient)
{
    // Connection targets
    const std::string url {"ltnm.learncppthroughprojects.com"};
    const std::string endpoint {"/network-events"};
    const std::string port {"443"};
    std::string response_msg;
    
    // STOMP frame
    const std::string username {"fake_username"};
    const std::string password {"fake_password"};
    std::stringstream ss {};
    ss << "STOMP" << std::endl
       << "accept-version:1.2" << std::endl
       << "host:transportforlondon.com" << std::endl
       << "login:" << username << std::endl
       << "passcode:" << password << std::endl
       << std::endl // Headers need to be followed by a blank line.
       << '\0'; // The body (even if absent) must be followed by a NULL octet.
    const std::string message {ss.str()};

    // Always start with an I/O context object.
    boost::asio::io_context ioc {};

    ssl::context ctx(ssl::context::tlsv12_client);
    ctx.load_verify_file(TESTS_CACERT_PEM); // TESTS_CACERT_PEM is defined in CMake

    // The class under test
    BoostWebSocketClient client {url, endpoint, port, ioc, ctx};

    // We use these flags to check that the connection, send, receive functions
    // work as expected.
    bool connected {false};
    bool messageSent {false};
    bool messageReceived {false};
    bool messageMatches {false};
    bool disconnected {false};

    // Our own callbacks
    auto onSend {[&messageSent](auto ec) {
        messageSent = !ec;
    }};
    auto onConnect {[&client, &connected, &onSend, &message](auto ec) {
        connected = !ec;
        if (!ec) {
            client.Send(message, onSend);
        }
    }};
    auto onClose {[&disconnected](auto ec) {
        disconnected = !ec;
    }};
    auto onReceive {[&client,
                      &onClose,
                      &messageReceived,
                      &messageMatches,
                      &message,
                      &response_msg](auto ec, auto received) {
        messageReceived = !ec;
        messageMatches = message == received;
        response_msg = std::move(received);
        client.Close(onClose);
    }};

    // We must call io_context::run for asynchronous callbacks to run.
    client.Connect(onConnect, onReceive);
    ioc.run();

    // When we get here, the io_context::run function has run out of work to do.
    BOOST_CHECK(connected);
    BOOST_CHECK(messageSent);
    BOOST_CHECK(messageReceived);
    BOOST_CHECK(disconnected);
    BOOST_CHECK(CheckResponse(response_msg));
}

BOOST_AUTO_TEST_CASE(cacert_pem)
{
    BOOST_CHECK(std::filesystem::exists(TESTS_CACERT_PEM)); 
}

BOOST_AUTO_TEST_SUITE_END();