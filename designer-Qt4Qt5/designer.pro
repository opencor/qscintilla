# This is the qmake file for the QScintilla plugin for Qt Designer.


TEMPLATE = lib
TARGET = qscintillaplugin

CONFIG += plugin qscintilla2

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += designer

    # Work around QTBUG-39300.
    CONFIG -= android_install
} else {
    CONFIG += designer
}

macx {
    QMAKE_POST_LINK = install_name_tool -change libqscintilla2_qt$${QT_MAJOR_VERSION}.13.dylib $$[QT_INSTALL_LIBS]/libqscintilla2_qt$${QT_MAJOR_VERSION}.13.dylib $(TARGET)
}

HEADERS = qscintillaplugin.h
SOURCES = qscintillaplugin.cpp

target.path = $$[QT_INSTALL_PLUGINS]/designer
INSTALLS += target
