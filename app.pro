TARGET = Offroad
SOURCES = App.cpp \
    btBulletWorldImporter.cpp \
    debug-drawer/GLDebugDrawer.cpp \
    debug-drawer/GLDebugFont.cpp \
    BulletFileLoader/bChunk.cpp \
    BulletFileLoader/bDNA.cpp \
    BulletFileLoader/bFile.cpp \
    BulletFileLoader/btBulletFile.cpp \
    vehicle/btRaycastVehicle.cpp \
    vehicle/btWheelInfo.cpp
HEADERS = debug-drawer/GLDebugDrawer.h \
    debug-drawer/GLDebugFont.h \
    App.h
INCLUDEPATH += /home/matej/college/grafika/bullet/src
INCLUDEPATH += /home/matej/college/grafika/bullet/Extras/Serialize/BulletWorldImporter
CONFIG += qt \
    warn_on \
    release
LIBS += -lGLEW -lBulletDynamics -lBulletCollision -lLinearMath
QT += opengl \
    xml
