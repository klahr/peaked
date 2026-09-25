# The name of your application
TARGET = peaked

CONFIG += sailfishapp
QT += dbus network

SOURCES += src/peaked.cpp \
    src/advisor.cpp \
    src/bloodalcohol.cpp \
    src/drinklog.cpp \
    src/monotone.cpp \
    src/moodlog.cpp \
    src/presetstore.cpp \
    src/productsearch.cpp \
    src/profile.cpp \
    src/sessionlog.cpp

HEADERS += src/advisor.h \
    src/bloodalcohol.h \
    src/drinklog.h \
    src/monotone.h \
    src/moodlog.h \
    src/presetstore.h \
    src/productsearch.h \
    src/profile.h \
    src/sessionlog.h

DISTFILES += qml/peaked.qml \
    qml/display.js \
    qml/cover/CoverPage.qml \
    qml/pages/BloodAlcoholGraph.qml \
    qml/pages/CameraPage.qml \
    qml/pages/DrinkDialog.qml \
    qml/pages/DrinkPhoto.qml \
    qml/pages/MainPage.qml \
    qml/pages/MoodButtons.qml \
    qml/pages/PresetDialog.qml \
    qml/pages/PresetsPage.qml \
    qml/pages/ProductSearchPage.qml \
    qml/pages/SettingsDialog.qml \
    rpm/peaked.changes \
    rpm/peaked.spec \
    translations/*.ts \
    peaked.desktop \
    LICENSE \
    README.md \
    icons/peaked.svg

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

# Keeps translations/peaked.ts up to date as the source for translators.
# Add translations/peaked-<lang>.ts files to TRANSLATIONS to ship them.
CONFIG += sailfishapp_i18n
