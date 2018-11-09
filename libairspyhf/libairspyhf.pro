#--------------------------------------------------------
#
# Pro file for Android and Windows builds with Qt Creator
#
#--------------------------------------------------------

QT += core

TEMPLATE = lib
TARGET = libairspyhf

CONFIG(MINGW32):LIBAIRSPYHFSRC = "C:\softs\airspyhf\libairspyhf"
CONFIG(MINGW64):LIBAIRSPYHFSRC = "C:\softs\airspyhf\libairspyhf"
CONFIG(MSVC):LIBAIRSPYHFSRC = "C:\softs\airspyhf\libairspyhf"

INCLUDEPATH += $$LIBAIRSPYHFSRC/src

CONFIG(MINGW32):INCLUDEPATH += "C:\softs\libusb-1.0.22\include\libusb-1.0"
CONFIG(MINGW64):INCLUDEPATH += "C:\softs\libusb-1.0.22\include\libusb-1.0"
CONFIG(MSVC):INCLUDEPATH += "C:\softs\libusb-1.0.22\include\libusb-1.0"
CONFIG(MSVC):INCLUDEPATH += "C:\softs\pthreads-w32\include"

SOURCES = $$LIBAIRSPYHFSRC/src/airspyhf.c\
  $$LIBAIRSPYHFSRC/src/iqbalancer.c

HEADERS = $$LIBAIRSPYHFSRC/src/airspyhf.h\
  $$LIBAIRSPYHFSRC/src/airspyhf_commands.h\
  $$LIBAIRSPYHFSRC/src/iqbalancer.h

CONFIG(MINGW32):LIBS += -LC:\softs\libusb-1.0.22\MinGW32\dll -llibusb-1.0
CONFIG(MINGW64):LIBS += -LC:\softs\libusb-1.0.22\MinGW64\dll -llibusb-1.0
CONFIG(MSVC):LIBS += -LC:\softs\libusb-1.0.22\MS64\dll -llibusb-1.0
CONFIG(MSVC):LIBS += -LC:\softs\pthreads-w32\lib\x64 -lpthreadVC2

CONFIG(ANDROID):CONFIG += mobility
CONFIG(ANDROID):MOBILITY =
