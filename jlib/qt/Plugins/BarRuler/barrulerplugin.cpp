#include "barruler.h"
#include "barrulerplugin.h"

#include <QtPlugin>

BarRulerPlugin::BarRulerPlugin(QObject *parent)
    : QObject(parent)
{
    m_initialized = false;
}

void BarRulerPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool BarRulerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *BarRulerPlugin::createWidget(QWidget *parent)
{
    return new BarRuler(parent);
}

QString BarRulerPlugin::name() const
{
    return QLatin1String("BarRuler");
}

QString BarRulerPlugin::group() const
{
    return QLatin1String("");
}

QIcon BarRulerPlugin::icon() const
{
    return QIcon();
}

QString BarRulerPlugin::toolTip() const
{
    return QLatin1String("");
}

QString BarRulerPlugin::whatsThis() const
{
    return QLatin1String("");
}

bool BarRulerPlugin::isContainer() const
{
    return false;
}

QString BarRulerPlugin::domXml() const
{
    return QLatin1String("<widget class=\"BarRuler\" name=\"barRuler\">\n</widget>\n");
}

QString BarRulerPlugin::includeFile() const
{
    return QLatin1String("barruler.h");
}
#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(barrulerplugin, BarRulerPlugin)
#endif // QT_VERSION < 0x050000
