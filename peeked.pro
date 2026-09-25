# The name of your application
TARGET = peeked

CONFIG += sailfishapp
QT += dbus network

SOURCES += src/peeked.cpp \
    src/advisor.cpp \
    src/bloodalcohol.cpp \
    src/drinklog.cpp \
    src/moodlog.cpp \
    src/presetstore.cpp \
    src/productsearch.cpp \
    src/profile.cpp

HEADERS += src/advisor.h \
    src/bloodalcohol.h \
    src/drinklog.h \
    src/moodlog.h \
    src/presetstore.h \
    src/productsearch.h \
    src/profile.h

DISTFILES += qml/peeked.qml \
    qml/display.js \
    qml/cover/CoverPage.qml \
    qml/pages/BloodAlcoholGraph.qml \
    qml/pages/CameraPage.qml \
    qml/pages/DrinkDialog.qml \
    qml/pages/DrinkPhoto.qml \
    qml/pages/MainPage.qml \
    qml/pages/PresetDialog.qml \
    qml/pages/PresetsPage.qml \
    qml/pages/ProductSearchPage.qml \
    qml/pages/SettingsDialog.qml \
    rpm/peeked.changes \
    rpm/peeked.spec \
    translations/*.ts \
    peeked.desktop \
    LICENSE \
    README.md \
    icons/peeked.svg

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

# Keeps translations/peeked.ts up to date as the source for translators.
# Add translations/peeked-<lang>.ts files to TRANSLATIONS to ship them.
CONFIG += sailfishapp_i18n
