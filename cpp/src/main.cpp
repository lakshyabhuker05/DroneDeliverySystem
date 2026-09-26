/*******************************************************************************
 * main.cpp
 * Drone Delivery Management System -- entry point.
 *
 * Wires together all C++ business-logic managers (each backed by a specific
 * Data Structure -- see comments on each class) and exposes them to the
 * HTML/CSS/JS frontend through a small built-in HTTP server. ALL decision
 * making (routing, assignment, sorting, searching, undo, priority handling)
 * happens here in C++; the frontend only renders what this program returns.
 ******************************************************************************/
#include "Graph.h"
#include "DroneManager.h"
#include "CustomerManager.h"
#include "DeliveryManager.h"
#include "ReportManager.h"
#include "HttpServer.h"
#include "Utils.h"
#include "../../ai/AIService.h"

#include <iostream>
#include <sstream>
#include <cstdlib>

static Graph g_graph;
static DroneManager* g_droneMgr = nullptr;
static CustomerManager* g_customerMgr = nullptr;
static DeliveryManager* g_deliveryMgr = nullptr;
static ReportManager* g_reportMgr = nullptr;
static AIService* g_ai = nullptr;

// ---------------------------------------------------------------------------
// Seed sample data (cities/routes graph + a few drones) the FIRST time the
// program runs (i.e. if the data files do not yet contain anything).
// ---------------------------------------------------------------------------
static void seedGraph() {
    // City network -- distances in km (sample regional delivery zone).
    g_graph.addRoute("Hub", "Downtown", 8);
    g_graph.addRoute("Hub", "Riverside", 12);
    g_graph.addRoute("Hub", "Airport", 15);
    g_graph.addRoute("Downtown", "Riverside", 6);
    g_graph.addRoute("Downtown", "Hillview", 10);
    g_graph.addRoute("Riverside", "Lakeside", 9);
    g_graph.addRoute("Airport", "Lakeside", 14);
    g_graph.addRoute("Hillview", "Lakeside", 7);
    g_graph.addRoute("Hillview", "Northgate", 11);
    g_graph.addRoute("Lakeside", "Northgate", 8);
    g_graph.addRoute("Northgate", "Eastpark", 13);
    g_graph.addRoute("Airport", "Eastpark", 20);
}

static void seedDronesIfEmpty() {
    if (g_droneMgr->totalDrones() > 0) return;
    g_droneMgr->addDrone("SkyHawk X1", 100, 45, 5, "Hub");
    g_droneMgr->addDrone("SkyHawk X2", 95, 50, 8, "Downtown");
    g_droneMgr->addDrone("FalconLite", 80, 60, 3, "Riverside");
    g_droneMgr->addDrone("HeavyLift T4", 90, 35, 20, "Airport");
    g_droneMgr->addDrone("SwiftDrone", 60, 55, 4, "Hillview");
    g_droneMgr->addDrone("SkyHawk X3", 40, 45, 5, "Lakeside");
}

// ---------------------------------------------------------------------------
// Small helpers to build consistent JSON responses.
// ---------------------------------------------------------------------------
static HttpResponse jsonOk(const std::string& body) {
    HttpResponse r; r.statusCode = 200; r.contentType = "application/json"; r.body = body; return r;
}
static HttpResponse jsonError(int code, const std::string& msg) {
    HttpResponse r; r.statusCode = code; r.contentType = "application/json";
    r.body = "{\"success\":false,\"message\":\"" + utils::jsonEscape(msg) + "\"}";
    return r;
}

static std::string dronesToJsonArray(const std::vector<Drone>& drones) {
    std::ostringstream os; os << "[";
    for (size_t i = 0; i < drones.size(); ++i) { os << drones[i].toJSON(); if (i + 1 < drones.size()) os << ","; }
    os << "]"; return os.str();
}
static std::string deliveriesToJsonArray(const std::vector<Delivery>& d) {
    std::ostringstream os; os << "[";
    for (size_t i = 0; i < d.size(); ++i) { os << d[i].toJSON(); if (i + 1 < d.size()) os << ","; }
    os << "]"; return os.str();
}
static std::string customersToJsonArray(const std::vector<Customer>& c) {
    std::ostringstream os; os << "[";
    for (size_t i = 0; i < c.size(); ++i) { os << c[i].toJSON(); if (i + 1 < c.size()) os << ","; }
    os << "]"; return os.str();
}

// ---------------------------------------------------------------------------
// Route registration
// ---------------------------------------------------------------------------
static void registerRoutes(HttpServer& server) {

    // ---------------- Dashboard ----------------
    server.addRoute("GET", "/api/dashboard", [](const HttpRequest&) {
        auto s = g_reportMgr->dashboardStats();
        std::ostringstream os;
        os << "{"
           << "\"totalDeliveries\":" << s.totalDeliveries << ","
           << "\"activeDrones\":" << s.activeDrones << ","
           << "\"totalDrones\":" << s.totalDrones << ","
           << "\"availableDrones\":" << s.availableDrones << ","
           << "\"avgBattery\":" << s.avgBattery << ","
           << "\"pendingDeliveries\":" << s.pendingDeliveries << ","
           << "\"completedDeliveries\":" << s.completedDeliveries
           << "}";
        return jsonOk(os.str());
    });

    // ---------------- Cities / Graph ----------------
    server.addRoute("GET", "/api/cities", [](const HttpRequest&) {
        std::ostringstream os; os << "[";
        auto cities = g_graph.allCities();
        for (size_t i = 0; i < cities.size(); ++i) { os << "\"" << cities[i] << "\""; if (i + 1 < cities.size()) os << ","; }
        os << "]";
        return jsonOk(os.str());
    });

    server.addRoute("GET", "/api/route", [](const HttpRequest& req) {
        std::string pickup = req.query.count("pickup") ? req.query.at("pickup") : "";
        std::string dest = req.query.count("destination") ? req.query.at("destination") : "";
        auto result = g_graph.shortestRoute(pickup, dest);
        if (!result.found) return jsonError(404, "No route found between the given cities");
        std::ostringstream os;
        os << "{\"found\":true,\"distanceKm\":" << result.distanceKm << ",\"path\":[";
        for (size_t i = 0; i < result.path.size(); ++i) { os << "\"" << result.path[i] << "\""; if (i + 1 < result.path.size()) os << ","; }
        os << "]}";
        return jsonOk(os.str());
    });

    // ---------------- Drones (Admin) ----------------
    server.addRoute("GET", "/api/drones", [](const HttpRequest&) {
        return jsonOk(dronesToJsonArray(g_droneMgr->allDronesSortedById()));
    });

    server.addRoute("POST", "/api/drones/add", [](const HttpRequest& req) {
        std::string model = utils::extractJsonString(req.body, "model");
        double battery = utils::extractJsonNumber(req.body, "battery", 100);
        double speed = utils::extractJsonNumber(req.body, "speed", 40);
        double maxWeight = utils::extractJsonNumber(req.body, "maxWeight", 5);
        std::string location = utils::extractJsonString(req.body, "location");
        if (model.empty() || location.empty()) return jsonError(400, "model and location are required");
        int id = g_droneMgr->addDrone(model, battery, speed, maxWeight, location);
        return jsonOk("{\"success\":true,\"droneId\":" + std::to_string(id) + "}");
    });

    server.addRoute("POST", "/api/drones/remove", [](const HttpRequest& req) {
        int id = static_cast<int>(utils::extractJsonNumber(req.body, "id", -1));
        bool ok = g_droneMgr->removeDrone(id);
        return jsonOk(std::string("{\"success\":") + (ok ? "true" : "false") + "}");
    });

    server.addRoute("POST", "/api/drones/update", [](const HttpRequest& req) {
        int id = static_cast<int>(utils::extractJsonNumber(req.body, "id", -1));
        Drone d;
        if (!g_droneMgr->getDrone(id, d)) return jsonError(404, "Drone not found");
        std::string model = utils::extractJsonString(req.body, "model");
        std::string location = utils::extractJsonString(req.body, "location");
        double battery = utils::extractJsonNumber(req.body, "battery", d.batteryPercent);
        double speed = utils::extractJsonNumber(req.body, "speed", d.speedKmph);
        double maxWeight = utils::extractJsonNumber(req.body, "maxWeight", d.maxWeightKg);
        if (!model.empty()) d.model = model;
        if (!location.empty()) d.currentLocation = location;
        d.batteryPercent = battery; d.speedKmph = speed; d.maxWeightKg = maxWeight;
        g_droneMgr->updateDrone(id, d);
        return jsonOk("{\"success\":true}");
    });

    // ---------------- Customers ----------------
    server.addRoute("POST", "/api/register", [](const HttpRequest& req) {
        std::string name = utils::extractJsonString(req.body, "name");
        std::string email = utils::extractJsonString(req.body, "email");
        std::string password = utils::extractJsonString(req.body, "password");
        std::string address = utils::extractJsonString(req.body, "address");
        std::string phone = utils::extractJsonString(req.body, "phone");
        int id = g_customerMgr->registerCustomer(name, email, password, address, phone);
        if (id == -1) return jsonError(400, "Registration failed: invalid input or email already registered");
        return jsonOk("{\"success\":true,\"customerId\":" + std::to_string(id) + "}");
    });

    server.addRoute("POST", "/api/login", [](const HttpRequest& req) {
        std::string email = utils::extractJsonString(req.body, "email");
        std::string password = utils::extractJsonString(req.body, "password");
        int id = g_customerMgr->login(email, password);
        if (id == -1) return jsonError(401, "Invalid email or password");
        Customer c; g_customerMgr->getCustomer(id, c);
        return jsonOk("{\"success\":true,\"customer\":" + c.toJSON() + "}");
    });

    server.addRoute("GET", "/api/customers", [](const HttpRequest&) {
        return jsonOk(customersToJsonArray(g_customerMgr->allCustomers()));
    });

    // ---------------- Deliveries ----------------
    server.addRoute("GET", "/api/deliveries", [](const HttpRequest&) {
        return jsonOk(deliveriesToJsonArray(g_deliveryMgr->allDeliveries()));
    });

    server.addRoute("GET", "/api/deliveries/history", [](const HttpRequest& req) {
        int customerId = req.query.count("customerId") ? std::stoi(req.query.at("customerId")) : -1;
        return jsonOk(deliveriesToJsonArray(g_deliveryMgr->historyForCustomer(customerId)));
    });

    server.addRoute("POST", "/api/book", [](const HttpRequest& req) {
        int customerId = static_cast<int>(utils::extractJsonNumber(req.body, "customerId", -1));
        std::string package = utils::extractJsonString(req.body, "package");
        double weight = utils::extractJsonNumber(req.body, "weight", 0);
        std::string pickup = utils::extractJsonString(req.body, "pickup");
        std::string destination = utils::extractJsonString(req.body, "destination");
        int priority = static_cast<int>(utils::extractJsonNumber(req.body, "priority", 1));

        int id = g_deliveryMgr->bookDelivery(customerId, package, weight, pickup, destination, priority);
        if (id == -1) return jsonError(400, "Booking failed: check weight/cities/input");

        // Attempt immediate auto-assignment so the customer sees a drone/ETA right away.
        g_deliveryMgr->assignNextDelivery();
        g_droneMgr->savePersisted();

        Delivery d; g_deliveryMgr->getDelivery(id, d);
        return jsonOk("{\"success\":true,\"delivery\":" + d.toJSON() + "}");
    });

    server.addRoute("POST", "/api/cancel", [](const HttpRequest& req) {
        int deliveryId = static_cast<int>(utils::extractJsonNumber(req.body, "deliveryId", -1));
        int customerId = static_cast<int>(utils::extractJsonNumber(req.body, "customerId", -1));
        bool isAdmin = utils::extractJsonString(req.body, "role") == "admin";
        bool ok = g_deliveryMgr->cancelDelivery(deliveryId, customerId, isAdmin);
        return jsonOk(std::string("{\"success\":") + (ok ? "true" : "false") + "}");
    });

    server.addRoute("POST", "/api/assign-next", [](const HttpRequest&) {
        bool ok = g_deliveryMgr->assignNextDelivery();
        return jsonOk(std::string("{\"success\":") + (ok ? "true" : "false") + "}");
    });

    server.addRoute("POST", "/api/assign", [](const HttpRequest& req) {
        int deliveryId = static_cast<int>(utils::extractJsonNumber(req.body, "deliveryId", -1));
        int droneId = static_cast<int>(utils::extractJsonNumber(req.body, "droneId", -1));
        bool ok = g_deliveryMgr->assignSpecific(deliveryId, droneId);
        return jsonOk(std::string("{\"success\":") + (ok ? "true" : "false") + "}");
    });

    server.addRoute("POST", "/api/deliveries/in-transit", [](const HttpRequest& req) {
        int deliveryId = static_cast<int>(utils::extractJsonNumber(req.body, "deliveryId", -1));
        bool ok = g_deliveryMgr->markInTransit(deliveryId);
        return jsonOk(std::string("{\"success\":") + (ok ? "true" : "false") + "}");
    });

    server.addRoute("POST", "/api/deliveries/delivered", [](const HttpRequest& req) {
        int deliveryId = static_cast<int>(utils::extractJsonNumber(req.body, "deliveryId", -1));
        bool ok = g_deliveryMgr->markDelivered(deliveryId);
        return jsonOk(std::string("{\"success\":") + (ok ? "true" : "false") + "}");
    });

    server.addRoute("POST", "/api/undo", [](const HttpRequest&) {
        bool ok = g_deliveryMgr->undoLastAction();
        return jsonOk(std::string("{\"success\":") + (ok ? "true" : "false") + "}");
    });

    // ---------------- Reports ----------------
    server.addRoute("GET", "/api/reports", [](const HttpRequest& req) {
        std::string range = req.query.count("range") ? req.query.at("range") : "daily";
        ReportRange r = range == "weekly" ? ReportRange::WEEKLY : range == "monthly" ? ReportRange::MONTHLY : ReportRange::DAILY;
        auto rows = g_reportMgr->generate(r);
        return jsonOk(g_reportMgr->toJSON(rows));
    });

    // ---------------- AI (Cloud AI: Gemini / OpenAI) ----------------
    server.addRoute("GET", "/api/ai/status", [](const HttpRequest&) {
        bool avail = g_ai->isCloudAIAvailable();
        std::string provider = g_ai->activeProvider() == AIService::Provider::GEMINI ? "gemini" :
                                g_ai->activeProvider() == AIService::Provider::OPENAI ? "openai" : "offline";
        return jsonOk("{\"available\":" + std::string(avail ? "true" : "false") + ",\"provider\":\"" + provider + "\"}");
    });

    server.addRoute("POST", "/api/ai/predict-time", [](const HttpRequest& req) {
        double distance = utils::extractJsonNumber(req.body, "distance", 0);
        double speed = utils::extractJsonNumber(req.body, "speed", 40);
        std::string weather = utils::extractJsonString(req.body, "weather");
        if (weather.empty()) weather = "clear";
        std::string resp = g_ai->predictDeliveryTime(distance, speed, weather);
        return jsonOk("{\"result\":\"" + utils::jsonEscape(resp) + "\"}");
    });

    server.addRoute("POST", "/api/ai/route-tip", [](const HttpRequest& req) {
        std::string pickup = utils::extractJsonString(req.body, "pickup");
        std::string destination = utils::extractJsonString(req.body, "destination");
        double distance = utils::extractJsonNumber(req.body, "distance", 0);
        std::string resp = g_ai->suggestBestRoute(pickup, destination, distance);
        return jsonOk("{\"result\":\"" + utils::jsonEscape(resp) + "\"}");
    });

    server.addRoute("GET", "/api/ai/weather", [](const HttpRequest& req) {
        std::string city = req.query.count("city") ? req.query.at("city") : "Hub";
        std::string resp = g_ai->weatherAdvice(city);
        return jsonOk("{\"result\":\"" + utils::jsonEscape(resp) + "\"}");
    });

    server.addRoute("POST", "/api/ai/battery-advice", [](const HttpRequest& req) {
        double battery = utils::extractJsonNumber(req.body, "battery", 100);
        double distance = utils::extractJsonNumber(req.body, "distance", 0);
        std::string resp = g_ai->batteryRecommendation(battery, distance);
        return jsonOk("{\"result\":\"" + utils::jsonEscape(resp) + "\"}");
    });

    server.addRoute("POST", "/api/ai/chat", [](const HttpRequest& req) {
        std::string message = utils::extractJsonString(req.body, "message");
        std::string resp = g_ai->chatbotResponse(message);
        return jsonOk("{\"result\":\"" + utils::jsonEscape(resp) + "\"}");
    });

    server.addRoute("GET", "/api/ai/smart-report", [](const HttpRequest&) {
        auto rows = g_reportMgr->generate(ReportRange::WEEKLY);
        std::string csv = g_reportMgr->toCSV(rows);
        std::string resp = g_ai->generateSmartReportSummary(csv);
        return jsonOk("{\"result\":\"" + utils::jsonEscape(resp) + "\"}");
    });

    server.addRoute("GET", "/api/ai/optimize", [](const HttpRequest&) {
        auto stats = g_reportMgr->dashboardStats();
        std::string resp = g_ai->optimizationSuggestions(stats.pendingDeliveries, stats.availableDrones);
        return jsonOk("{\"result\":\"" + utils::jsonEscape(resp) + "\"}");
    });
}

int main() {
    std::cout << "=============================================\n";
    std::cout << " Drone Delivery Management System (C++ DSA) \n";
    std::cout << "=============================================\n";

    int sysRet = system("mkdir -p data"); // ensure persistence directory exists
    (void)sysRet;

    seedGraph();
    g_droneMgr = new DroneManager("data/drones.txt");
    g_customerMgr = new CustomerManager("data/customers.txt");
    g_deliveryMgr = new DeliveryManager(&g_graph, g_droneMgr, "data/deliveries.txt");
    g_reportMgr = new ReportManager(g_deliveryMgr, g_droneMgr);
    g_ai = new AIService();

    seedDronesIfEmpty();

    std::cout << "[AI] Cloud AI provider: "
              << (g_ai->activeProvider() == AIService::Provider::GEMINI ? "Gemini" :
                  g_ai->activeProvider() == AIService::Provider::OPENAI ? "OpenAI" : "OFFLINE (no API key set -- fallback mode)")
              << "\n";
    std::cout << "[Data] Drones: " << g_droneMgr->totalDrones()
              << " | Customers: " << g_customerMgr->totalCustomers()
              << " | Deliveries: " << g_deliveryMgr->totalCount() << "\n";

    HttpServer server(8080, "frontend");
    registerRoutes(server);

    std::cout << "Open http://localhost:8080 in your browser.\n";
    server.run();

    delete g_ai;
    delete g_reportMgr;
    delete g_deliveryMgr;
    delete g_customerMgr;
    delete g_droneMgr;
    return 0;
}
