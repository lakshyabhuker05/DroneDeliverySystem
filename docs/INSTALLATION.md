# Installation Guide

## 1. Prerequisites

| Requirement | Notes |
|---|---|
| Linux, macOS, or WSL2 on Windows | The bundled `HttpServer` uses POSIX sockets (`sys/socket.h`, `arpa/inet.h`), so native Windows (MSVC) is not supported without WSL or a sockets shim. |
| `g++` supporting C++17 | Tested with GCC 11+. |
| `make` | Optional but recommended — a Makefile is provided. |
| `libcurl4-openssl-dev` (optional) | Only needed if you enable live Cloud AI calls with `make AI=1`. |

Install prerequisites on Ubuntu/Debian:

```bash
sudo apt update
sudo apt install build-essential libcurl4-openssl-dev
```

## 2. Getting the project running

```bash
cd DroneDeliverySystem

# Offline-fallback build (no AI network dependency, still fully functional):
make

# Run (must be run from the project root, so the relative "frontend/" and
# "data/" paths resolve correctly):
./drone_server
```

Open your browser to **http://localhost:8080**.

## 3. Enabling live Cloud AI (optional)

```bash
export GEMINI_API_KEY="your_key_here"      # get one from https://aistudio.google.com
# -- or --
export OPENAI_API_KEY="your_key_here"

make clean
make AI=1
./drone_server
```

The console will print which provider was detected:
```
[AI] Cloud AI provider: Gemini
```
or, with no key set:
```
[AI] Cloud AI provider: OFFLINE (no API key set -- fallback mode)
```

## 4. Resetting sample data

All persisted data lives under `data/` as plain pipe-delimited `.txt` files.
To reset to a clean, freshly-seeded state:

```bash
rm -rf data
./drone_server   # re-seeds 8 cities + 6 sample drones on next launch
```

## 5. Using the app

1. Open `http://localhost:8080` → **Register** a customer account, then
   **Login**.
2. You'll land on the **Customer Portal** — book a delivery, preview the
   Dijkstra-computed route, and watch the AI route-tip / ETA prediction load.
3. Open `http://localhost:8080/admin.html` in another tab — passcode
   `admin123` (change `ADMIN_PASSCODE` in `frontend/js/admin.js` for your own
   deployment) — to manage drones, assign the queue, and generate reports.
4. Open `http://localhost:8080/dashboard.html` for the live charts.

## 6. Common issues

| Symptom | Fix |
|---|---|
| `Bind failed on port 8080` | Another process is already using port 8080 — stop it, or change the port in `main()` (`HttpServer server(8080, "frontend");`). |
| Blank pages / 404 for CSS/JS | Make sure you launched `./drone_server` **from the project root** (`DroneDeliverySystem/`), not from inside `cpp/` or `frontend/`. |
| `curl: command not found` when testing manually | Install curl (`sudo apt install curl`) — not required for normal browser use. |
| AI endpoints always show `[Offline ...]` | Confirm `GEMINI_API_KEY`/`OPENAI_API_KEY` is exported **before** running `make AI=1` and before launching `./drone_server`, and that you rebuilt after setting `AI=1`. |
