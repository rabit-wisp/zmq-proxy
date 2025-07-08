#include <zmq.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <cassert>
#include "docopt.h"

static const char USAGE[] =
R"(ZMQ Pub/Sub Proxy.

Usage:
  zmq-proxy --sub=SUBSCRIBER... --pub=PUBLISHER...
  zmq-proxy --stdin --pub=PUBLISHER...
  zmq-proxy --sub=SUBSCRIBER... (--stdout|--stderr) [--subscriptions=TAG1,TAG2,...] [--sub-method=(bind|connect)]

Options:
  --pub=SUBSCRIBER...            Publisher endpoint (e.g., connect:tcp://*:5555, bind:ipc://stdout).
  --sub=PUBLISHER...             Subscriber endpoint (e.g., connect:tcp://*:5556, bind:tcp://123:23).
  --stdin                        Take input from stdin and send over publisher endpoint
  --stdout                       Output subscriber data to stdout
  --stderr                       Output subscriber data to stderr
  --subscriptions=TAG1,TAG2,...  Subscribe to specific tags - uses XSUB (all messages) if none specified

Note: when dumping data from stdin, use part boundaries are null character delimited, and multi-part message terminators
are double null character delimited.
)";


enum class zmq_method {
    bind,
    connect
};

std::pair<zmq_method, std::string> parse_endpoint(const std::string& endpoint) {
    constexpr auto bind_prefix = "bind:";
    constexpr auto connect_prefix = "connect:";

    if (endpoint.starts_with(bind_prefix)) {
        return {zmq_method::bind, endpoint.substr(std::strlen(bind_prefix))};
    }
    else if (endpoint.starts_with(connect_prefix)) {
        return {zmq_method::connect, endpoint.substr(std::strlen(connect_prefix))};
    }

    throw std::invalid_argument("Invalid endpoint format: must start with 'bind:' or 'connect:'");
}

void connect_endpoint(void* socket, const std::vector<std::string>& endpoints)
{
    for( auto& e: endpoints) {
        auto [method, endpoint] = parse_endpoint(e);

        if (method == zmq_method::connect){
            std::cout << "  connect " << endpoint << std::endl;
            zmq_connect(socket, endpoint.c_str());
        }
        else if (method == zmq_method::bind){
            std::cout << "  bind    " << endpoint << std::endl;
            zmq_bind(socket, endpoint.c_str());
        }
    }
}

void stream_in_handler(void* ctx, const std::vector<std::string>& endpoints)
{
    void* socket = zmq_socket(ctx, ZMQ_PUB);
    connect_endpoint(socket, endpoints);

    std::string buffer;
    while (std::getline(std::cin, buffer, '\0'))
    {
        if (buffer.empty()) {
            zmq_send(socket, "", 0, 0);  ///< Final empty frame.
        } else {

            zmq_send(socket, buffer.data(), buffer.size(), ZMQ_SNDMORE);

        }
    }
    zmq_send(socket, "", 0, 0);  ///< Final empty frame.
    zmq_close(socket);
}

void stream_out_handler(void* ctx,
                        std::ostream& stream,
                        const std::vector<std::string>& endpoints,
                        const std::vector<std::string>& subscriptions)
{
    /* this handler essentially acts as a ZMQ->pipe bridge. It captures all traffic
     * (i.e. subscribing to all traffic)
     *
     */

    void* socket = zmq_socket(ctx, ZMQ_SUB);
    connect_endpoint(socket, endpoints);

    if (subscriptions.empty())
        zmq_setsockopt(socket, ZMQ_SUBSCRIBE, "", 0);
    else
        for (auto& tag : subscriptions) {
            zmq_setsockopt(socket, ZMQ_SUBSCRIBE, tag.data(), tag.size());
        }

    while (true) {
        int64_t more;
        size_t more_size = sizeof more;

        do {
            zmq_msg_t part;
            int rc = zmq_msg_init(&part);
            assert(rc == 0);

            /* Block until a message is available to be received from socket */
            rc = zmq_recvmsg(socket, &part, 0);
            assert(rc != -1);

            stream.write((char*)zmq_msg_data(&part), zmq_msg_size(&part));
            stream.put('\0');

            /* Determine if more message parts are to follow */
            rc = zmq_getsockopt (socket, ZMQ_RCVMORE, &more, &more_size);
            assert (rc == 0);

            zmq_msg_close (&part);
        } while (more);

        stream.put('\0');
        stream.put('\n');
        stream.flush();
    }

    zmq_close(socket);
}

void zmq_proxy_handler(void* ctx,
                       const std::vector<std::string>& pub_endpoints,
                       const std::vector<std::string>& sub_endpoints) {

    void* xsub = zmq_socket(ctx, ZMQ_XSUB);
    void* xpub = zmq_socket(ctx, ZMQ_XPUB);

    std::cout << "initializing SUB sockets" << std::endl;
    connect_endpoint(xsub, sub_endpoints);

    std::cout << "initializing PUB sockets" << std::endl;
    connect_endpoint(xpub, pub_endpoints);

    std::cout << "starting proxy" << std::endl;
    zmq_proxy(xpub, xsub, nullptr);

    zmq_close(xsub);
    zmq_close(xpub);
}

int main(int argc, const char* argv[]) {
    auto args = docopt::docopt(USAGE, {argv + 1, argv + argc});

    std::vector<std::string> pub = args["--pub"] ? args["--pub"].asStringList() : std::vector<std::string>{};
    std::vector<std::string> sub = args["--sub"] ? args["--sub"].asStringList() : std::vector<std::string>{};
    bool from_stdin = args["--stdin"].asBool();
    bool to_stdout = args["--stdout"].asBool();
    bool to_stderr = args["--stderr"].asBool();

    std::vector<std::string> subscriptions;
    if (args["--subscriptions"])
        subscriptions = args["--subscriptions"].asStringList();

    void* ctx = zmq_ctx_new();

    if (from_stdin)
        stream_in_handler(ctx, pub);
    else if (to_stdout)
        stream_out_handler(ctx, std::cout, sub, subscriptions);
    else if (to_stderr)
        stream_out_handler(ctx, std::cerr, sub, subscriptions);
    else
        zmq_proxy_handler(ctx, pub, sub);

    zmq_ctx_term(ctx);

}
