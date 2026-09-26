#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>
#include <map>
#include <functional>
#include <vector>

/*******************************************************************************
 * HttpServer
 * A deliberately minimal, dependency-free HTTP/1.1 server built directly on
 * POSIX sockets (Linux). It exists purely as a thin transport layer so the
 * HTML/CSS/JS frontend can talk to the C++ business logic over HTTP -- ALL
 * business logic (drones, deliveries, routing, reports, DSA) lives in the
 * manager classes; this class only parses request lines, routes them, and
 * writes back a response.
 *
 * Supported: GET and POST, static file serving, query-string parsing, and a
 * simple route table keyed by "METHOD path".
 ******************************************************************************/
struct HttpRequest {
    std::string method;
    std::string path;                       // path without query string
    std::map<std::string, std::string> query;
    std::string body;
};

struct HttpResponse {
    int statusCode = 200;
    std::string contentType = "application/json";
    std::string body;
};

using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

class HttpServer {
private:
    int serverFd;
    int port;
    std::string staticRoot;
    std::map<std::string, RouteHandler> routes; // key = "GET /api/drones"

    static std::map<std::string, std::string> parseQuery(const std::string& qs);
    static std::string urlDecode(const std::string& s);
    HttpResponse serveStaticFile(const std::string& path) const;
    void handleClient(int clientFd);

public:
    explicit HttpServer(int port_, std::string staticRoot_ = "frontend");
    void addRoute(const std::string& method, const std::string& path, RouteHandler handler);
    void run(); // blocking loop
};

#endif // HTTP_SERVER_H
