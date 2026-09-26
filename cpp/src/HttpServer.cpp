#include "HttpServer.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

HttpServer::HttpServer(int port_, std::string staticRoot_) : serverFd(-1), port(port_), staticRoot(std::move(staticRoot_)) {}

void HttpServer::addRoute(const std::string& method, const std::string& path, RouteHandler handler) {
    routes[method + " " + path] = std::move(handler);
}

std::string HttpServer::urlDecode(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            int value = std::stoi(s.substr(i + 1, 2), nullptr, 16);
            out.push_back(static_cast<char>(value));
            i += 2;
        } else if (s[i] == '+') {
            out.push_back(' ');
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

std::map<std::string, std::string> HttpServer::parseQuery(const std::string& qs) {
    std::map<std::string, std::string> out;
    std::stringstream ss(qs);
    std::string pair;
    while (std::getline(ss, pair, '&')) {
        auto eq = pair.find('=');
        if (eq == std::string::npos) continue;
        std::string key = urlDecode(pair.substr(0, eq));
        std::string val = urlDecode(pair.substr(eq + 1));
        out[key] = val;
    }
    return out;
}

static std::string contentTypeFor(const std::string& path) {
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".html") return "text/html";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".css") return "text/css";
    if (path.size() >= 3 && path.substr(path.size() - 3) == ".js") return "application/javascript";
    if (path.size() >= 5 && path.substr(path.size() - 5) == ".json") return "application/json";
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".png") return "image/png";
    return "text/plain";
}

HttpResponse HttpServer::serveStaticFile(const std::string& reqPath) const {
    std::string path = reqPath;
    if (path == "/" || path.empty()) path = "/index.html";
    std::string fullPath = staticRoot + path;

    std::ifstream file(fullPath, std::ios::binary);
    HttpResponse resp;
    if (!file.is_open()) {
        resp.statusCode = 404;
        resp.contentType = "text/html";
        resp.body = "<h1>404 Not Found</h1><p>" + reqPath + "</p>";
        return resp;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    resp.body = ss.str();
    resp.contentType = contentTypeFor(fullPath);
    resp.statusCode = 200;
    return resp;
}

void HttpServer::handleClient(int clientFd) {
    char buffer[8192];
    std::string raw;
    ssize_t n;
    // Read until we've got the header block; for simplicity we do a bounded
    // number of reads (sufficient for typical small JSON bodies in this demo).
    while ((n = recv(clientFd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = '\0';
        raw += buffer;
        if (raw.find("\r\n\r\n") != std::string::npos) {
            // if Content-Length present and body not fully read, loop again
            auto headerEnd = raw.find("\r\n\r\n");
            auto clPos = raw.find("Content-Length:");
            if (clPos != std::string::npos && clPos < headerEnd) {
                size_t lenStart = clPos + strlen("Content-Length:");
                size_t lenEnd = raw.find("\r\n", lenStart);
                int contentLength = std::stoi(raw.substr(lenStart, lenEnd - lenStart));
                size_t bodyLenSoFar = raw.size() - (headerEnd + 4);
                if (static_cast<int>(bodyLenSoFar) >= contentLength) break;
                else continue;
            }
            break;
        }
        if (static_cast<size_t>(n) < sizeof(buffer) - 1) break;
    }

    if (raw.empty()) { close(clientFd); return; }

    std::istringstream stream(raw);
    std::string requestLine;
    std::getline(stream, requestLine);
    if (!requestLine.empty() && requestLine.back() == '\r') requestLine.pop_back();

    std::istringstream lineStream(requestLine);
    HttpRequest req;
    std::string fullPath;
    lineStream >> req.method >> fullPath;

    auto qmark = fullPath.find('?');
    if (qmark != std::string::npos) {
        req.path = fullPath.substr(0, qmark);
        req.query = parseQuery(fullPath.substr(qmark + 1));
    } else {
        req.path = fullPath;
    }

    auto headerEnd = raw.find("\r\n\r\n");
    if (headerEnd != std::string::npos) req.body = raw.substr(headerEnd + 4);

    HttpResponse resp;
    auto it = routes.find(req.method + " " + req.path);
    if (it != routes.end()) {
        resp = it->second(req);
    } else if (req.method == "GET") {
        resp = serveStaticFile(req.path);
    } else {
        resp.statusCode = 404;
        resp.contentType = "application/json";
        resp.body = "{\"error\":\"route not found\"}";
    }

    std::ostringstream out;
    out << "HTTP/1.1 " << resp.statusCode << (resp.statusCode == 200 ? " OK" : " ERROR") << "\r\n";
    out << "Content-Type: " << resp.contentType << "\r\n";
    out << "Content-Length: " << resp.body.size() << "\r\n";
    out << "Access-Control-Allow-Origin: *\r\n";
    out << "Connection: close\r\n\r\n";
    out << resp.body;

    std::string full = out.str();
    send(clientFd, full.c_str(), full.size(), 0);
    close(clientFd);
}

void HttpServer::run() {
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) { std::cerr << "Failed to create socket\n"; return; }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed on port " << port << "\n";
        return;
    }
    if (listen(serverFd, 32) < 0) {
        std::cerr << "Listen failed\n";
        return;
    }

    std::cout << "HTTP server listening on http://localhost:" << port << "\n";

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientFd < 0) continue;
        handleClient(clientFd); // sequential handling (sufficient for demo/course-project load)
    }
}
