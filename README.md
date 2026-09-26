# 🚁 Drone Delivery Management System

A final-year / major-project grade **Drone Delivery Management System** where
**all business logic is implemented in C++ using classic Data Structures &
Algorithms**. HTML/CSS/JavaScript is used *only* as the presentation layer —
every decision (routing, drone assignment, queueing, undo, reports) is
computed by the C++ backend and served over a small built-in HTTP server.

> No local AI model is used. AI features call **Cloud AI APIs** (Google
> Gemini / OpenAI) and gracefully fall back to rule-based offline logic when
> no API key is configured, so the core system never depends on an internet
> connection to function.

---

## 1. Why this project is DSA-heavy, not just CRUD

| Data Structure | Class | Real purpose in this system |
|---|---|---|
| **Queue** (linked-list FIFO) | `LinkedQueue<T>` | Normal delivery requests wait in booking order |
| **Priority Queue** (binary heap) | `PriorityQueue<T,Compare>` | Urgent/medical deliveries jump ahead, with aging on ties |
| **Stack** (linked-list LIFO) | `LinkedStack<T>` | Undo the last cancel / assignment (admin panel "Undo" button) |
| **Linked List** | `LinkedList<T>` | Per-customer delivery history, chronological, reversible for "most recent first" |
| **Binary Search Tree** | `BinarySearchTree<K,V>` | Drone registry keyed by Drone ID — sorted listing + O(log n) ops |
| **Hash Map** (separate chaining, hand-built) | `HashMap<K,V>` | O(1) average customer login lookup, drone quick-lookup mirror, email index |
| **Graph + Dijkstra** | `Graph` | City network; shortest route + distance + ETA between pickup & destination |
| **Vector** | `std::vector` | Package-type catalog, route paths, report rows |
| **Sorting** | `Sorting.h` | Merge sort (stable, by date) and quick sort (by ID) for reports |
| **Searching** | `Searching.h` | Binary search (post-sort) + linear search fallback for fast record lookup |

Every structure above is **hand-implemented** (not a thin `typedef` over
`std::queue`/`std::stack`/etc.) specifically so the project demonstrates DSA
fundamentals, per the assignment brief.

---

## 2. Folder structure

```
DroneDeliverySystem/
├── cpp/
│   ├── include/          # all header files (.h)
│   │   ├── DataStructures.h   (Queue, Stack, LinkedList, BST, HashMap, PriorityQueue)
│   │   ├── Sorting.h          (merge sort, quick sort)
│   │   ├── Searching.h        (binary search, linear search)
│   │   ├── Graph.h            (city graph + Dijkstra)
│   │   ├── Drone.h / Customer.h / Delivery.h  (entity classes)
│   │   ├── DroneManager.h / CustomerManager.h / DeliveryManager.h / ReportManager.h
│   │   ├── HttpServer.h       (raw-socket HTTP server)
│   │   └── Utils.h            (hashing, JSON helpers, validation)
│   └── src/               # all implementation files (.cpp)
│       ├── main.cpp            (wiring + REST API routes)
│       ├── DroneManager.cpp
│       ├── CustomerManager.cpp
│       ├── DeliveryManager.cpp
│       ├── ReportManager.cpp
│       └── HttpServer.cpp
├── ai/
│   ├── AIService.h / AIService.cpp   (Gemini / OpenAI cloud calls + offline fallback)
├── frontend/
│   ├── index.html        (landing + login/register)
│   ├── dashboard.html    (live stats + Chart.js graphs)
│   ├── customer.html     (book / history / track / AI chat)
│   ├── admin.html        (drones / deliveries / customers / reports)
│   ├── css/style.css
│   └── js/ (api.js, customer.js, admin.js)
├── data/                 # runtime persistence (plain pipe-delimited .txt "tables")
├── docs/                 # this documentation set
├── Makefile
└── README.md
```

---

## 3. Build & Run

Requires: `g++` (C++17), Linux/WSL/macOS (uses POSIX sockets).

```bash
cd DroneDeliverySystem
make            # builds ./drone_server (offline AI fallback mode)
./drone_server
```

Then open **http://localhost:8080** in your browser.

### Enabling real Cloud AI (optional)

```bash
export GEMINI_API_KEY="your-gemini-key"      # or OPENAI_API_KEY
make clean && make AI=1                       # links libcurl, enables live calls
./drone_server
```

Without a key, every AI endpoint still returns a clearly-labelled
`[Offline ...]` computed answer — the system is 100% functional without any
AI provider.

### First run — sample data

On first launch the system seeds:
- An 8-city delivery network (`Hub, Downtown, Riverside, Airport, Hillview,
  Lakeside, Northgate, Eastpark`) with realistic distances.
- 6 sample drones of varying battery/speed/capacity spread across cities.

Data persists to `data/*.txt` between runs (simple flat-file storage, human
readable, easy to inspect/reset by deleting the folder).

---

## 4. API surface (used by the frontend)

| Method & Path | Purpose |
|---|---|
| `GET /api/dashboard` | Aggregate stats for the dashboard cards |
| `GET /api/cities` | All graph nodes |
| `GET /api/route?pickup=&destination=` | Dijkstra shortest path + distance |
| `GET/POST /api/drones`, `/api/drones/add`, `/remove`, `/update` | Drone CRUD |
| `POST /api/register`, `/api/login` | Customer auth |
| `GET /api/customers` | Admin customer list |
| `GET /api/deliveries`, `/api/deliveries/history?customerId=` | Delivery data |
| `POST /api/book`, `/api/cancel`, `/api/assign-next`, `/api/assign` | Delivery lifecycle |
| `POST /api/deliveries/in-transit`, `/delivered` | Status transitions |
| `POST /api/undo` | Pop the undo stack |
| `GET /api/reports?range=daily\|weekly\|monthly` | Sorted/filtered report rows |
| `GET /api/ai/status`, `POST /api/ai/predict-time`, `/route-tip`, `/battery-advice`, `/chat`, `GET /api/ai/smart-report`, `/optimize` | Cloud AI features |

---

## 5. Security & validation notes

- Passwords are never stored in plain text (hashed via `utils::simpleHash`,
  see `Utils.h` — for production use, swap in bcrypt/argon2).
- All booking input is validated server-side (weight bounds, valid cities,
  distinct pickup/destination, email format, password length).
- Admin routes (`role: "admin"` in the cancel endpoint, admin panel passcode
  client-side gate) demonstrate role separation; a production deployment
  should add signed session tokens.
- `HttpServer` sets `Access-Control-Allow-Origin: *` for local development
  convenience — restrict this in production.

## 6. Further documentation

See `docs/PROJECT_REPORT.md`, `docs/ER_Diagram.md`, `docs/ClassDiagram.md`,
`docs/Flowchart.md`, `docs/UseCaseDiagram.md`, `docs/DFD.md`, and
`docs/INSTALLATION.md`.
