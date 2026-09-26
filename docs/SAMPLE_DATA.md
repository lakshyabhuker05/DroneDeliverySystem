# Sample Seed Data

The system auto-seeds this data on first launch (when `data/` is empty).
It is documented here for grading / demo reference.

## City Network (Graph edges, km)

| From | To | Distance (km) |
|---|---|---|
| Hub | Downtown | 8 |
| Hub | Riverside | 12 |
| Hub | Airport | 15 |
| Downtown | Riverside | 6 |
| Downtown | Hillview | 10 |
| Riverside | Lakeside | 9 |
| Airport | Lakeside | 14 |
| Hillview | Lakeside | 7 |
| Hillview | Northgate | 11 |
| Lakeside | Northgate | 8 |
| Northgate | Eastpark | 13 |
| Airport | Eastpark | 20 |

## Sample Drone Fleet

| ID | Model | Battery | Speed (km/h) | Max Weight (kg) | Location |
|---|---|---|---|---|---|
| 1 | SkyHawk X1 | 100% | 45 | 5 | Hub |
| 2 | SkyHawk X2 | 95% | 50 | 8 | Downtown |
| 3 | FalconLite | 80% | 60 | 3 | Riverside |
| 4 | HeavyLift T4 | 90% | 35 | 20 | Airport |
| 5 | SwiftDrone | 60% | 55 | 4 | Hillview |
| 6 | SkyHawk X3 | 40% | 45 | 5 | Lakeside |

## Package Catalog

Documents, Electronics, Medicines, Groceries, Clothing, Spare Parts.

## Suggested Demo Script

1. Register a customer (`Ali Khan`, `ali@test.com`, password `pass123`).
2. Log in, book a **Normal** delivery from `Hub` to `Eastpark` — watch it
   auto-assign to the nearest capable drone and show a Dijkstra-computed
   route + ETA.
3. Book a **Critical/Medical** delivery from `Airport` to `Northgate` — note
   it jumps the priority queue ahead of any pending normal request.
4. In the Admin Panel, remove the assigned drone's charge by editing battery
   (or just observe `HeavyLift T4`'s lower speed affecting ETA on heavy
   packages > 8kg, since lighter drones can't carry them).
5. Cancel a delivery, then click **Undo Last Action** in the admin panel to
   demonstrate the Stack-based undo.
6. Open the Reports tab, generate a Daily report, and read the AI executive
   summary (falls back to an offline summary without an API key).
