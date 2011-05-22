TARGET = Offroad
SOURCES = App.cpp \
    btBulletWorldImporter.cpp \
    BulletFileLoader/bChunk.cpp \
    BulletFileLoader/bDNA.cpp \
    BulletFileLoader/bFile.cpp \
    BulletFileLoader/btBulletFile.cpp \
    vehicle/btRaycastVehicle.cpp \
    vehicle/btWheelInfo.cpp
HEADERS = App.h
INCLUDEPATH += /home/matej/college/grafika/bullet/src
INCLUDEPATH += /home/matej/college/grafika/bullet/Extras/Serialize/BulletWorldImporter
QMAKE_LIBDIR += /home/matej/college/grafika/app
CONFIG += qt \
    warn_on \
    release
LIBS += -lGLEW -lBulletDynamics -lBulletCollision -lLinearMath
QT += opengl \
    xml
