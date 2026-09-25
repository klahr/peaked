#include <QDBusConnection>
#include <QGuiApplication>
#include <QQuickView>
#include <QtQml>

#include <sailfishapp.h>

#include "advisor.h"
#include "bloodalcohol.h"
#include "drinklog.h"
#include "moodlog.h"
#include "presetstore.h"
#include "productsearch.h"
#include "profile.h"

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));

    qmlRegisterUncreatableType<Profile>("rs.r8.peaked", 1, 0, "Profile",
                                        QStringLiteral("The profile is a context property"));
    qmlRegisterUncreatableType<MoodLog>("rs.r8.peaked", 1, 0, "MoodLog",
                                        QStringLiteral("The mood log is a context property"));
    qmlRegisterType<ProductSearch>("rs.r8.peaked", 1, 0, "ProductSearch");
    qmlRegisterUncreatableType<Advisor>("rs.r8.peaked", 1, 0, "Advisor",
                                        QStringLiteral("The advisor is a context property"));

    Profile profile;
    PresetStore presetStore;
    DrinkLog drinkLog;
    BloodAlcohol bloodAlcohol(&profile, &drinkLog);
    MoodLog moodLog(&bloodAlcohol);
    Advisor advisor(&bloodAlcohol, &drinkLog, &moodLog);

    // The mood notification buttons call in here. Sailjail lets the app own
    // its OrganizationName.ApplicationName.
    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.registerService(QStringLiteral("rs.r8.peaked"));
    bus.registerObject(QStringLiteral("/rs/r8/peaked"), &moodLog, QDBusConnection::ExportScriptableSlots);

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->rootContext()->setContextProperty(QStringLiteral("profile"), &profile);
    view->rootContext()->setContextProperty(QStringLiteral("presetStore"), &presetStore);
    view->rootContext()->setContextProperty(QStringLiteral("drinkLog"), &drinkLog);
    view->rootContext()->setContextProperty(QStringLiteral("bloodAlcohol"), &bloodAlcohol);
    view->rootContext()->setContextProperty(QStringLiteral("moodLog"), &moodLog);
    view->rootContext()->setContextProperty(QStringLiteral("advisor"), &advisor);
    view->setSource(SailfishApp::pathToMainQml());
    view->show();

    return app->exec();
}
