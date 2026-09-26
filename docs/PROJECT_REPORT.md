# Project Report — Drone Delivery Management System

## 1. Abstract

The Drone Delivery Management System is a full-stack academic major project
that simulates the operations of a commercial drone-delivery company:
customer onboarding, delivery booking, fleet management, route optimization,
and reporting. The distinguishing design goal of this project is that **all
decision-making logic is implemented in native C++ using hand-built Data
Structures and Algorithms**, rather than relying on a scripting-language
backend or off-the-shelf frameworks. The web frontend (HTML/CSS/JavaScript)
is a thin presentation layer that communicates with the C++ program over a
lightweight, purpose-built HTTP server, and Cloud AI (Gemini/OpenAI) is used
strictly as an optional enhancement layer, never as a replacement for the
core algorithmic logic.

## 2. Objectives

1. Demonstrate practical, non-trivial use of core DSA topics (Queue, Stack,
   Linked List, BST, Hash Map, Graph, Priority Queue, Sorting, Searching).
2. Model a realistic multi-actor business domain (Customer / Admin / Drone
   Fleet / Delivery Lifecycle) using clean OOP design.
3. Provide a genuinely usable, modern web interface without leaking business
   logic into JavaScript.
4. Integrate optional Cloud AI features without creating a hard dependency
   on any AI provider.
5. Persist data across runs using simple, auditable flat files (no external
   database dependency), while keeping the design open to swapping in a real
   database later.

## 3. System Architecture

```
┌─────────────────────┐      HTTP/JSON      ┌──────────────────────────────┐
│   Frontend (SPA-ish) │ ───────────────────▶│        C++ HttpServer         │
│  HTML + CSS + JS     │◀───────────────────  │  (raw POSIX sockets, no deps) │
└─────────────────────┘                      └───────────────┬──────────────┘
                                                              │
                                              ┌───────────────┴───────────────┐
                                              │        Manager Layer          │
                                              │ DroneManager | CustomerManager│
                                              │ DeliveryManager | ReportManager│
                                              └───────────────┬───────────────┘
                                                              │
                              ┌───────────────────────────────┼───────────────────────────────┐
                              │                               │                               │
                      ┌───────┴────────┐             ┌────────┴────────┐             ┌────────┴────────┐
                      │ Data Structures │             │      Graph      │             │    AIService     │
                      │ Queue / Stack /  │             │  (Dijkstra)     │             │ Gemini / OpenAI  │
                      │ BST / HashMap /  │             │                 │             │ (optional, cloud)│
                      │ LinkedList / PQ  │             └─────────────────┘             └──────────────────┘
                      └──────────────────┘
                              │
                      ┌───────┴────────┐
                      │  Flat-file I/O  │
                      │  data/*.txt     │
                      └────────────────┘
```

## 4. Modules Implemented

### 4.1 Customer Module
- Register (server-side validated: email format, password length, duplicate
  email rejection via `HashMap` email index).
- Login (email → id via `HashMap`, password compared as salted hash).
- Book Delivery (creates a `Delivery`, computes route via Dijkstra, enqueues
  into the FIFO `Queue` or the `Priority Queue` depending on urgency).
- View Delivery History (`LinkedList` per customer, most-recent-first).
- Track Package (live status/ETA/drone lookup).
- Cancel Delivery (releases the assigned drone if any; pushed onto the
  `Stack` for undo).

### 4.2 Admin Module
- Dashboard (aggregate stats computed from the manager layer).
- Add / Remove / Update Drone (`BinarySearchTree` + `HashMap` kept in sync).
- Assign Deliveries (pop next request from Priority Queue / Queue, run
  Dijkstra, pick best available drone, or override with a specific drone).
- Manage Customers (list, backed by the customer `HashMap`).
- Reports (merge-sorted, range-filtered, exportable as CSV/JSON).
- Undo (pop the `Stack` to reverse the last cancel/assign/book action).

### 4.3 Drone Management
Each `Drone` stores ID, model, status (`AVAILABLE / ASSIGNED / CHARGING /
MAINTENANCE / OFFLINE`), battery %, speed (km/h), max carry weight (kg),
current location (graph city), and availability flag.

### 4.4 Delivery Management
Each `Delivery` stores ID, customer ID, package name & weight, pickup &
destination city, assigned drone ID, status
(`PENDING/ASSIGNED/IN_TRANSIT/DELIVERED/CANCELLED`), computed distance (km),
computed ETA (minutes), priority level, and creation timestamp.

### 4.5 Route Planning
`Graph` represents cities as nodes and direct corridors as weighted,
undirected edges. `shortestRoute()` implements **Dijkstra's Algorithm** with
a binary min-heap (`std::priority_queue`) to return the shortest path,
total distance, and the ordered list of intermediate cities. ETA is derived
as `distance / drone.speed * 60` minutes.

### 4.6 AI Features (Cloud-only)
`AIService` calls Gemini/OpenAI's chat/completions endpoint via `libcurl`
for: delivery time prediction, best-route commentary, weather advisories,
battery-usage recommendations, a customer-support chatbot, AI-written
report summaries, and fleet optimization suggestions. If no API key is
present, each method returns a clearly-labelled `[Offline ...]` answer
computed from the same underlying DSA-derived numbers, so the system never
silently breaks.

## 5. Data Structure Design Decisions

- **Queue vs Priority Queue split**: normal bookings are strictly FIFO
  (fairness), while urgent/medical bookings preempt via a max-heap
  comparator that also ages older requests to prevent starvation.
- **BST + HashMap for Drones**: the BST gives a naturally sorted admin
  listing (by ID) with O(log n) mutation; the HashMap mirror gives O(1)
  average lookup for the hot path (`findBestDroneForPickup` runs on every
  booking).
- **Stack for Undo**: every mutating delivery operation (book/cancel/assign)
  pushes an `UndoAction` capturing the pre-mutation state, giving simple,
  correct LIFO undo semantics without a full command-pattern framework.
- **Graph + Dijkstra over Floyd-Warshall**: since routes are queried
  point-to-point on demand (not all-pairs), Dijkstra's O((V+E) log V) per
  query is the appropriate complexity class for this workload.
- **Merge Sort for reports**: stability matters when sorting by date while
  preserving insertion order for same-timestamp entries; merge sort's
  guaranteed O(n log n) and stability make it the right tool. **Quick Sort**
  is used where average-case speed matters more than stability (sorting by
  numeric ID before a binary search).

## 6. Testing Performed

- Compiled with `g++ -std=c++17 -Wall -Wextra` — zero warnings.
- Manual endpoint testing via `curl` for: city listing, Dijkstra route
  query, registration, login, booking (with automatic drone assignment),
  drone status transition verification, undo, dashboard stats, and all AI
  endpoints in offline-fallback mode.
- Verified BST/HashMap consistency after add/remove/update drone cycles.
- Verified Queue vs Priority Queue ordering with mixed-priority bookings.

## 7. Limitations & Future Work

- Flat-file persistence is simple and human-readable but not transactional;
  a production version should migrate to SQLite/PostgreSQL.
- The bundled HTTP server is intentionally minimal (sequential request
  handling) — sufficient for a demo/course project, but a production
  deployment should use a hardened server or a thread/async pool.
- Password hashing uses `std::hash` with a salt for demonstration; a real
  deployment must use bcrypt/argon2/scrypt.
- AI JSON parsing from the raw Cloud API response is intentionally minimal
  (no external JSON library dependency) — a production build should adopt
  `nlohmann/json` for robust parsing.

## 8. Conclusion

This project demonstrates that a genuinely interactive, modern-looking web
application can be powered end-to-end by classical Data Structures and
Algorithms implemented in C++, with the web stack strictly limited to
presentation duties — fulfilling both the academic DSA requirement and the
"looks like a real industry application" requirement of the brief.
