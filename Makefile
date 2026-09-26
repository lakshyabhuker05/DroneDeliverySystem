# =============================================================================
# Drone Delivery Management System - Makefile
#
# Build (offline / no AI network calls, still fully functional with fallback
# responses for every AI feature):
#     make
#
# Build WITH real Cloud AI (Gemini/OpenAI) support (requires libcurl-dev):
#     make AI=1
#
# Run (must run from the project root so relative paths `frontend/` and
# `data/` resolve correctly):
#     ./drone_server
# =============================================================================

CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Icpp/include -Iai
LDFLAGS := -lpthread

SRC := cpp/src/main.cpp \
       cpp/src/DroneManager.cpp \
       cpp/src/CustomerManager.cpp \
       cpp/src/DeliveryManager.cpp \
       cpp/src/ReportManager.cpp \
       cpp/src/HttpServer.cpp \
       ai/AIService.cpp

TARGET := drone_server

ifeq ($(AI),1)
CXXFLAGS += -DUSE_CURL
LDFLAGS += -lcurl
endif

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
