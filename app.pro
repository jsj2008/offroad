TARGET = Offroad
SOURCES = App.cpp \
    debug-drawer/GLDebugDrawer.cpp \
    debug-drawer/GLDebugFont.cpp
HEADERS = debug-drawer/GLDebugDrawer.h \
    debug-drawer/GLDebugFont.h \
    App.h
INCLUDEPATH = /data/Dropbox/college/grafika/research/bullet/bullet-2.77/src
CONFIG += qt \
    warn_on \
    release
LIBS += -lGLEW -lBulletDynamics -lBulletCollision -lLinearMath
QT += opengl \
    xml
