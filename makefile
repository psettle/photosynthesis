#config
DEBUG=0

#setup
SOURCES=
LIBS=
INCLUDE=

#header includes
INCLUDE += src/agent/mcts
INCLUDE += src/agent/neural_mcts
INCLUDE += src/engine
INCLUDE += src/test
INCLUDE += src/util
INCLUDE += src/neural

#source includes
SOURCES += src/main.cpp
SOURCES += src/engine/engine_GameState.cpp
#SOURCES += src/agent/neural_mcts/train_network_main.cpp
#SOURCES += src/create_training_data_main.cpp

#more setup
EXECUTABLE=out/photo.exe

ifeq ($(DEBUG), 1)
	FLAG_BUILD_MODE=-O0 -g
else
	FLAG_BUILD_MODE=-O3
endif

LDFLAGS=-Wall $(FLAG_BUILD_MODE)
CC=g++
CFLAGS=-c -MMD -Wall $(FLAG_BUILD_MODE)
OBJECTS=$(SOURCES:%.cpp=out/%.o)
DEPENDENCIES=$(OBJECTS_FINAL:.o=.d)

INCLUDE_FORMATTED=$(addprefix -I, $(INCLUDE))

#targets
.PHONY: all
all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	@$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@
	@echo $@

$(OBJECTS): out/%.o : %.cpp
	@mkdir -p out/$(dir $<)
	@$(CC) $(CFLAGS) $(INCLUDE_FORMATTED) $< -o $@
	@echo $<


.PHONY: clean
clean:
	@rm -rf out/*

-include $(DEPENDENCIES)