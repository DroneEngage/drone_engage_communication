
CXX=g++
CXXARM=/usr/bin/arm-linux-gnueabihf-g++
CXXARM_ZERO=/opt/cross-pi-gcc-10.2.0-0/bin/arm-linux-gnueabihf-g++
EXE=de_comm
EXE_ARM=de_comm
BIN=bin

INCLUDE= -I ~/TDisk/Boost/boost_1_76_0/ 
INCLUDE_ARM =  -I /home/pi/boost_1_76_0/ -I /usr/include -I /usr/include/arm-linux-gnueabihf  
INCLUDE_ARM_ZERO =  -I /home/pi/boost_1_76_0/  -I /usr/include/arm-linux-gnueabihf/  -I /usr/include 


LIBS=  -pthread   -lcurl  -lssl -lcrypto /home/mhefny/TDisk/Boost/boost_1_76_0/stage/lib/libboost_coroutine.a  /home/mhefny/TDisk/Boost/boost_1_76_0/stage/lib/libboost_thread.a /home/mhefny/TDisk/Boost/boost_1_76_0/stage/lib/libboost_filesystem.a  /home/mhefny/TDisk/Boost/boost_1_76_0/stage/lib/libboost_system.a /home/mhefny/TDisk/Boost/boost_1_76_0/stage/lib/libboost_chrono.a /home/mhefny/TDisk/Boost/boost_1_76_0/stage/lib/libboost_context.a
LIBS_ARM = -pthread   -lcurl  -lssl -lcrypto   /home/pi/boost_1_76_0/stage/lib/libboost_coroutine.a  /home/pi/boost_1_76_0/stage/lib/libboost_thread.a /home/pi/boost_1_76_0/stage/lib/libboost_filesystem.a  /home/pi/boost_1_76_0/stage/lib/libboost_system.a /home/pi/boost_1_76_0/stage/lib/libboost_chrono.a /home/pi/boost_1_76_0/stage/lib/libboost_context.a
LIBS_ARM_ZERO = -pthread   -lcurl  -lssl -lcrypto   /home/pi/boost_1_76_0/stage/lib/libboost_coroutine.a  /home/pi/boost_1_76_0/stage/lib/libboost_thread.a /home/pi/boost_1_76_0/stage/lib/libboost_filesystem.a  /home/pi/boost_1_76_0/stage/lib/libboost_system.a /home/pi/boost_1_76_0/stage/lib/libboost_chrono.a /home/pi/boost_1_76_0/stage/lib/libboost_context.a

CXXFLAGS =  -std=c++17
CXXFLAGS_RELEASE= $(CXXFLAGS) -DRELEASE -s   -Werror=unused-variable -Werror=unused-result -Werror=parentheses
CXXFLAGS_DEBUG= $(CXXFLAGS)  -DDEBUG -g   
BUILD = build

OBJS = $(BUILD)/main.o \
	   $(BUILD)/configFile.o \
	   $(BUILD)/andruav_comm_session.o \
	   $(BUILD)/andruav_comm_server.o \
	   $(BUILD)/andruav_facade.o \
	   $(BUILD)/uavos_modules_manager.o \
	   $(BUILD)/andruav_auth.o \
	   $(BUILD)/andruav_unit.o \
	   $(BUILD)/helpers.o \
	   $(BUILD)/util_rpi.o \
	   $(BUILD)/getopt_cpp.o \
	   $(BUILD)/gpio.o \
	   $(BUILD)/rpi_gpio.o \
	   $(BUILD)/notification.o \
	   $(BUILD)/leds.o \
	   $(BUILD)/buzzer.o \
	   $(BUILD)/udpCommunicator.o \
	   

SRCS = ../main.cpp \
	   ../configFile.cpp \
	   ../helpers/helpers.cpp \
	   ../helpers/util_rpi.cpp \
	   ../helpers/getopt_cpp.cpp \
	   ../comm_server/andruav_comm_session.cpp \
	   ../comm_server/andruav_comm_server.cpp \
	   ../comm_server/andruav_facade.cpp \
	   ../comm_server/andruav_auth.cpp \
	   ../comm_server/andruav_unit.cpp \
	   ../uavos/uavos_modules_manager.cpp \
	   ../hal/gpio.cpp \
	   ../hal_linux/rpi_gpio.cpp \
	   ../notification_module/notification.cpp \
	   ../notification_module/leds.cpp \
	   ../notification_module/buzzer.cpp \
	   ../udpCommunicator.cpp 
	   
	   


all: release





release: uavos.release
	$(CXX)   -O2 -o $(BIN)/$(EXE).so  $(OBJS)   $(LIBS)  ;
	@echo "building finished ..."; 
	@echo "DONE."

debug: uavos.debug
	$(CXX) -Og -o $(BIN)/$(EXE).so  $(OBJS)   $(LIBS) ;
	@echo "building finished ..."; 
	@echo "DONE."

arm_release: uavos.arm.release
	$(CXXARM)    -O2  -o $(BIN)/$(EXE_ARM).so   $(OBJS)   $(LIBS_ARM) ;
	@echo "building finished ..."; 
	@echo "DONE."

arm_debug: uavos.arm.debug
	$(CXXARM)    -Og -o $(BIN)/$(EXE_ARM).so   $(OBJS)   $(LIBS_ARM) ;
	@echo "building finished ..."; 
	@echo "DONE."

arm_release_zero: uavos.arm.release.zero
	$(CXXARM_ZERO)  -o $(BIN)/$(EXE_ARM).so   $(OBJS)   $(LIBS_ARM_ZERO) ;
	@echo "building finished ..."; 
	@echo "DONE."

uavos.release: copy
	mkdir -p $(BUILD); \
	cd $(BUILD); \
	$(CXX)   -DSIMULATE_GPIO $(CXXFLAGS_RELEASE)  -c   $(SRCS)  $(INCLUDE)  ; 
	cd .. ; 
	@echo "compliling finished ..."

uavos.debug: copy
	mkdir -p $(BUILD); \
	cd $(BUILD); \
	$(CXX)   -DSIMULATE_GPIO $(CXXFLAGS_DEBUG)  -c  $(SRCS)  $(INCLUDE);
	cd .. ; 
	@echo "compliling finished ..."


uavos.arm.release: copy
	mkdir -p $(BUILD); \
	cd $(BUILD); \
	$(CXXARM)   $(CXXFLAGS_RELEASE)  -c  $(SRCS)  $(INCLUDE_ARM)  ; 
	cd .. ; 
	@echo "compliling finished ..."

uavos.arm.debug: copy
	mkdir -p $(BUILD); \
	cd $(BUILD); \
	$(CXXARM)   $(CXXFLAGS_DEBUG) -c  $(SRCS)  $(INCLUDE_ARM)  ; 
	cd .. ; 
	@echo "compliling finished ..."



uavos.arm.release.zero: copy
	mkdir -p $(BUILD); \
	cd $(BUILD); \
	$(CXXARM_ZERO)   $(CXXFLAGS_RELEASE) -c  $(SRCS)  $(INCLUDE_ARM_ZERO)  ; 
	cd .. ; 
	@echo "compliling finished ..."

git_submodule:
	git submodule update --init --recursive


copy: clean
	mkdir -p $(BIN); \
	cp config.*.json $(BIN); 
	@echo "copying finished"

clean:
	rm -rf $(BIN); 
	rm -rf $(BUILD);
	@echo "cleaning finished"



	

