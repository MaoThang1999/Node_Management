
CXX      = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2 -g
LDFLAGS  = -lws2_32            # Winsock (UDPConnector)


TARGET   = NodeManagement.exe


OBJDIR   = build


INCLUDES = -I./source -I./source/node_service -I./source/udp_connected_lib


SOURCES  = source/AppCtrl.cpp \
           source/IOHandler.cpp \
           source/NodeManagament.cpp \
           source/Utils.cpp \
           source/node_service/ControlPlaneNode.cpp \
           source/node_service/DataPlaneNode.cpp \
           source/node_service/NodeBase.cpp \
           source/node_service/OAMNode.cpp \
           source/udp_connected_lib/UDPConnector.cpp


OBJECTS  = $(patsubst source/%.cpp, $(OBJDIR)/%.o, $(SOURCES))


.PHONY: all clean

all: $(TARGET)


$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)


$(OBJDIR)/%.o: source/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@


clean:
	rm -rf $(OBJDIR) $(TARGET)