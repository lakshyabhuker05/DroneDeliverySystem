# Use Case Diagram

```mermaid
flowchart LR
    Customer((Customer))
    Admin((Admin))
    CloudAI((Cloud AI<br/>Gemini/OpenAI))

    subgraph System [Drone Delivery Management System]
        UC1[Register Account]
        UC2[Login]
        UC3[Book Delivery]
        UC4[View Delivery History]
        UC5[Track Package]
        UC6[Cancel Delivery]
        UC7[AI Support Chat]

        UC8[View Dashboard]
        UC9[Add / Remove / Update Drone]
        UC10[Assign Deliveries]
        UC11[Manage Customers]
        UC12[Generate Reports]
        UC13[Undo Last Action]

        UC14[Predict Delivery Time]
        UC15[Suggest Best Route]
        UC16[Weather Advice]
        UC17[Battery Recommendation]
        UC18[Smart Report Summary]
        UC19[Optimization Suggestions]
    end

    Customer --> UC1
    Customer --> UC2
    Customer --> UC3
    Customer --> UC4
    Customer --> UC5
    Customer --> UC6
    Customer --> UC7

    Admin --> UC8
    Admin --> UC9
    Admin --> UC10
    Admin --> UC11
    Admin --> UC12
    Admin --> UC13

    UC3 -.includes.-> UC15
    UC3 -.includes.-> UC14
    UC10 -.includes.-> UC17
    UC12 -.includes.-> UC18
    UC8 -.includes.-> UC19
    UC7 -.includes.-> UC16

    UC7 --> CloudAI
    UC14 --> CloudAI
    UC15 --> CloudAI
    UC16 --> CloudAI
    UC17 --> CloudAI
    UC18 --> CloudAI
    UC19 --> CloudAI
```

All Cloud-AI use cases degrade to an offline, rule-based fallback computed
from the same C++ DSA-derived data when no `GEMINI_API_KEY` /
`OPENAI_API_KEY` is configured, so the system never depends on AI
availability for correctness.
