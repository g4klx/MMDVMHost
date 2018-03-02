cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(APP_NAME "MMDVMHost")
project(${APP_NAME})

option(ENABLE_OLED "Enable OLED support" OFF)
option(ENABLE_HD44780 "Enable HD44780 support" OFF)
option(ENABLE_ADAFRUIT "Enable Adafruit RGB LCD Shield support" OFF)
option(ENABLE_PCF8574 "Enable PCF8574 display support" OFF)
option(ENABLE_RASPBERRY_PI "Enable RASPBERRY_PI (Null display??)" OFF)

file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

if(GIT_VERSION)
  set(GIT_HASH ${GIT_VERSION})
else()
  find_package(Git)
  if(GIT_FOUND AND EXISTS ${CMAKE_SOURCE_DIR}/.git)
    execute_process(COMMAND ${GIT_EXECUTABLE} log -n 1 --pretty=format:"%h" HEAD
                    OUTPUT_VARIABLE GIT_HASH
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    string(REPLACE "\"" "" GIT_HASH ${GIT_HASH})
  else()
    set(GIT_HASH "0000000000000000000000000000000000000000")
  endif()
endif()
file(WRITE ${CMAKE_SOURCE_DIR}/GitVersion.h "const char *gitversion = \"${GIT_HASH}\";")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -Wall -std=c++0x $ENV{CXXFLAGS}")
set(DEPLIBS "")

find_package(Threads REQUIRED QUIET)
list(APPEND DEPLIBS ${CMAKE_THREAD_LIBS_INIT})

if(ENABLE_OLED)
  find_package(ArduiPi_OLED REQUIRED)
  include_directories(${ARDUIPI_OLED_INCLUDE_DIRS})
  list(APPEND DEPLIBS ${ARDUIPI_OLED_LIBRARIES})

  find_package(I2c)

  if(I2C_FOUND)
    include_directories(${I2C_INCLUDE_DIRS})
    list(APPEND DEPLIBS ${I2C_LIBRARIES})
  else()
    # raspbian wiringPi nonsense!
    find_package(WiringPi REQUIRED)
    find_package(WiringPiDev REQUIRED)

    include_directories(${WIRINGPI_INCLUDE_DIRS})
    list(APPEND DEPLIBS ${WIRINGPI_LIBRARIES} ${WIRINGPIDEV_LIBRARIES})
  endif()
  add_definitions(-DOLED)
else()
  list(REMOVE_ITEM SOURCES ${CMAKE_SOURCE_DIR}/OLED.cpp)
  list(REMOVE_ITEM HEADERS ${CMAKE_SOURCE_DIR}/OLED.h)
endif()

if(ENABLE_HD44780)
  find_package(WiringPi REQUIRED)
  find_package(WiringPiDev REQUIRED)

  include_directories(${WIRINGPI_INCLUDE_DIRS})
  list(APPEND DEPLIBS ${WIRINGPI_LIBRARIES} ${WIRINGPIDEV_LIBRARIES})
  add_definitions(-DHD44780)
endif()

if(ENABLE_ADAFRUIT)
  find_package(WiringPi REQUIRED)
  find_package(WiringPiDev REQUIRED)

  include_directories(${WIRINGPI_INCLUDE_DIRS})
  list(APPEND DEPLIBS ${WIRINGPI_LIBRARIES} ${WIRINGPIDEV_LIBRARIES})
  add_definitions(-DHD44780 -DADAFRUIT_DISPLAY)
endif()

if(ENABLE_PCF8574)
  find_package(WiringPi REQUIRED)
  find_package(WiringPiDev REQUIRED)

  include_directories(${WIRINGPI_INCLUDE_DIRS})
  list(APPEND DEPLIBS ${WIRINGPI_LIBRARIES} ${WIRINGPIDEV_LIBRARIES})
  add_definitions(-DHD44780 -DPCF8574_DISPLAY)
endif()

if(ENABLE_RASPBERRY_PI)
  find_package(WiringPi REQUIRED)
  find_package(WiringPiDev REQUIRED)

  include_directories(${WIRINGPI_INCLUDE_DIRS})
  list(APPEND DEPLIBS ${WIRINGPI_LIBRARIES} ${WIRINGPIDEV_LIBRARIES})
  add_definitions(-DRASPBERRY_PI)
endif()

if(NOT ENABLE_HD44780 AND NOT ENABLE_ADAFRUIT AND NOT ENABLE_PCF8574)
  list(REMOVE_ITEM SOURCES ${CMAKE_SOURCE_DIR}/HD44780.cpp)
  list(REMOVE_ITEM HEADERS ${CMAKE_SOURCE_DIR}/HD44780.h)
endif()

add_executable(${APP_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${APP_NAME} ${DEPLIBS})
