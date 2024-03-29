win32:CONFIG += win
win64:CONFIG += win
CONFIG(release, debug|release): DEFINES += NDEBUG
DEFINES += "FREEGLUT_STATIC=1"

# needed by freeglut: -lwinmm  -lgdi32
win:LIBS += -lfreeglut_static -lopengl32 -lglu32 -lwinmm  -lgdi32

QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3
QMAKE_LFLAGS_RELEASE -= -O1


INCLUDEPATH += ./ ./src
INCLUDEPATH += C:/Qt/Tools/mingw730_64/x86_64-w64-mingw32/include

SOURCES += \
    $$files(src/*.cpp) \
    $$files(src/topology/*.cpp)

HEADERS += \
    $$files(src/*.hpp) \
    $$files(src/topology/*.hpp)

CONFIG += c++14
