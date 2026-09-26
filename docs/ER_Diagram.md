# ER Diagram

Rendered with [Mermaid](https://mermaid.js.org/) — view on GitHub, VS Code
(Mermaid preview extension), or https://mermaid.live by pasting the block
below.

```mermaid
erDiagram
    CUSTOMER ||--o{ DELIVERY : books
    DRONE ||--o{ DELIVERY : fulfills
    CITY ||--o{ DELIVERY : "pickup / destination"
    CITY ||--o{ ROUTE : connects

    CUSTOMER {
        int id PK
        string name
        string email UK
        string passwordHash
        string address
        string phone
    }

    DRONE {
        int id PK
        string model
        string status
        double batteryPercent
        double speedKmph
        double maxWeightKg
        string currentLocation FK
        bool availability
    }

    DELIVERY {
        int id PK
        int customerId FK
        string packageName
        double weightKg
        string pickupCity FK
        string destinationCity FK
        int droneId FK
        string status
        double estimatedMinutes
        double distanceKm
        int priority
        long createdAt
    }

    CITY {
        string name PK
    }

    ROUTE {
        string fromCity FK
        string toCity FK
        double distanceKm
    }
```

## Notes
- `CITY` and `ROUTE` model the in-memory `Graph` (adjacency list); they are
  not persisted as a separate flat file in this build, but are shown here
  because they are first-class entities in the domain model.
- `DELIVERY.droneId` is nullable-equivalent (`-1`) until a drone is assigned.
