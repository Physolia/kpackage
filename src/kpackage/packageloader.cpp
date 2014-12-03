/*
 *   Copyright 2010 Ryan Rix <ry@n.rix.si>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "packageloader.h"

#include <QStandardPaths>
#include <QDirIterator>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCoreApplication>
#include <QPointer>

#include <QDebug>
#include <KLocalizedString>
#include <KPluginLoader>
#include <KPluginFactory>
#include <KDesktopFile>
#include <KConfigGroup>

#include "config-package.h"



#include "private/packages_p.h"
#include "package.h"
#include "packagestructure.h"

namespace KPackage
{

static PackageLoader *s_packageTrader = 0;

class PackageLoaderPrivate
{
public:
    PackageLoaderPrivate()
        : isDefaultLoader(false),
          packageStructurePluginDir("kpackage/packagestructure"),
          packageRE("[^a-zA-Z0-9\\-_]")
    {
    }

    static QSet<QString> knownCategories();
    static QString parentAppConstraint(const QString &parentApp = QString());

    static QSet<QString> s_customCategories;
    QHash<QString, QPointer<PackageStructure> > structures;
    bool isDefaultLoader;
    QString packageStructurePluginDir;
    QRegExp packageRE;
};

QSet<QString> PackageLoaderPrivate::s_customCategories;

QSet<QString> PackageLoaderPrivate::knownCategories()
{
    // this is to trick the tranlsation tools into making the correct
    // strings for translation
    QSet<QString> categories = s_customCategories;
    categories << QString(I18N_NOOP("Accessibility")).toLower()
               << QString(I18N_NOOP("Application Launchers")).toLower()
               << QString(I18N_NOOP("Astronomy")).toLower()
               << QString(I18N_NOOP("Date and Time")).toLower()
               << QString(I18N_NOOP("Development Tools")).toLower()
               << QString(I18N_NOOP("Education")).toLower()
               << QString(I18N_NOOP("Environment and Weather")).toLower()
               << QString(I18N_NOOP("Examples")).toLower()
               << QString(I18N_NOOP("File System")).toLower()
               << QString(I18N_NOOP("Fun and Games")).toLower()
               << QString(I18N_NOOP("Graphics")).toLower()
               << QString(I18N_NOOP("Language")).toLower()
               << QString(I18N_NOOP("Mapping")).toLower()
               << QString(I18N_NOOP("Miscellaneous")).toLower()
               << QString(I18N_NOOP("Multimedia")).toLower()
               << QString(I18N_NOOP("Online Services")).toLower()
               << QString(I18N_NOOP("Productivity")).toLower()
               << QString(I18N_NOOP("System Information")).toLower()
               << QString(I18N_NOOP("Utilities")).toLower()
               << QString(I18N_NOOP("Windows and Tasks")).toLower();
    return categories;
}

QString PackageLoaderPrivate::parentAppConstraint(const QString &parentApp)
{
    if (parentApp.isEmpty()) {
        const QCoreApplication *app = QCoreApplication::instance();
        if (!app) {
            return QString();
        }

        return QString("((not exist [X-KDE-ParentApp] or [X-KDE-ParentApp] == '') or [X-KDE-ParentApp] == '%1')")
               .arg(app->applicationName());
    }

    return QString("[X-KDE-ParentApp] == '%1'").arg(parentApp);
}

PackageLoader::PackageLoader()
    : d(new PackageLoaderPrivate)
{
}

PackageLoader::~PackageLoader()
{
    foreach (auto wp, d->structures) {
        delete wp.data();
    }
    delete d;
}

void PackageLoader::setPackageLoader(PackageLoader *loader)
{
    if (!s_packageTrader) {
        s_packageTrader = loader;
    } else {
#ifndef NDEBUG
        // qDebug() << "Cannot set packageTrader, already set!" << s_packageTrader;
#endif
    }
}

PackageLoader *PackageLoader::self()
{
    if (!s_packageTrader) {
        // we have been called before any PackageLoader was set, so just use the default
        // implementation. this prevents plugins from nefariously injecting their own
        // plugin loader if the app doesn't
        s_packageTrader = new PackageLoader;
        s_packageTrader->d->isDefaultLoader = true;
    }

    return s_packageTrader;
}

Package PackageLoader::loadPackage(const QString &packageFormat, const QString &packagePath)
{
    if (!d->isDefaultLoader) {
        Package p = internalLoadPackage(packageFormat);
        if (p.hasValidStructure()) {
            if (!packagePath.isEmpty()) {
                p.setPath(packagePath);
            }
            return p;
        }
    }

    if (packageFormat.isEmpty()) {
        return Package();
    }

    PackageStructure *structure = loadPackageStructure(packageFormat);

    if (structure) {
        Package p(structure);
        if (!packagePath.isEmpty()) {
            p.setPath(packagePath);
        }
        return p;
    }

#ifndef NDEBUG
        // qDebug() << "Couldn't load Package for" << packageFormat << "! reason given: " << error;
#endif

    return Package();
}

QList<KPluginMetaData> PackageLoader::listPackages(const QString &packageFormat, const QString &packageRoot)
{
    QList<KPluginMetaData> lst;

    //has been a root specified?
    QString actualRoot = packageRoot;

    //try to take it from the ackage structure
    if (actualRoot.isEmpty()) {
        PackageStructure *structure = d->structures.value(packageFormat).data();
        if (!structure) {
            if (packageFormat == QStringLiteral("KPackage/Generic")) {
                structure = new GenericPackage();
            }
        }

        if (!structure) {
            structure = loadPackageStructure(packageFormat);
        }

        if (structure) {
            d->structures.insert(packageFormat, structure);
            Package p(structure);
            actualRoot = p.defaultPackageRoot();

        }
    }

    if (actualRoot.isEmpty()) {
        actualRoot = packageFormat;
    }

    //TODO: case in which defaultpackageroot is absolute
    for (auto datadir : QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
        const QString plugindir = datadir + '/' + actualRoot;

        const QString &_ixfile = plugindir + QStringLiteral("kpluginindex.json");
        QFile indexFile(_ixfile);
        //qDebug() << "indexfile: " << _ixfile << indexFile.exists();
        if (indexFile.exists()) {
            indexFile.open(QIODevice::ReadOnly);
            QJsonDocument jdoc = QJsonDocument::fromBinaryData(indexFile.readAll());
            indexFile.close();


            QJsonArray plugins = jdoc.array();

            for (QJsonArray::const_iterator iter = plugins.constBegin(); iter != plugins.constEnd(); ++iter) {
                const QJsonObject &obj = QJsonValue(*iter).toObject();
                const QString &pluginFileName = obj.value(QStringLiteral("FileName")).toString();
                const KPluginMetaData m(obj, QString(), pluginFileName);
                if (m.isValid()) {
                    lst << m;
                }
            }

        } else {
            //qDebug() << "Not cached";
            // If there's no cache file, fall back to listing the directory
            const QDirIterator::IteratorFlags flags = QDirIterator::Subdirectories;
            const QStringList nameFilters = QStringList(QStringLiteral("metadata.desktop"));

            QDirIterator it(plugindir, nameFilters, QDir::Files, flags);
            while (it.hasNext()) {
                it.next();
                const QString metadataPath = it.fileInfo().absoluteFilePath();

                const KPluginMetaData info(metadataPath);

                if (!info.isValid()) {
                    continue;
                }

                if (packageFormat.isEmpty() || info.serviceTypes().contains(packageFormat)) {
                    lst << info;
                }
            }
        }
    }

    return lst;
}

KPackage::PackageStructure *PackageLoader::loadPackageStructure(const QString &packageFormat)
{
    PackageStructure *structure = d->structures.value(packageFormat).data();
    if (!structure) {
        if (packageFormat == QStringLiteral("KPackage/Generic")) {
            structure = new GenericPackage();
        }
    }

    QStringList libraryPaths;

    const QString subDirectory = "kpackage/packagestructure";

    Q_FOREACH (const QString &dir, QCoreApplication::libraryPaths()) {
        QString d = dir + QDir::separator() + subDirectory;
        if (!d.endsWith(QDir::separator())) {
            d += QDir::separator();
        }
        libraryPaths << d;
    }


    QString pluginFileName;

    Q_FOREACH (const QString &plugindir, libraryPaths) {
        const QString &_ixfile = plugindir + QStringLiteral("kpluginindex.json");
        QFile indexFile(_ixfile);
        if (indexFile.exists()) {
            indexFile.open(QIODevice::ReadOnly);
            QJsonDocument jdoc = QJsonDocument::fromBinaryData(indexFile.readAll());
            indexFile.close();


            QJsonArray plugins = jdoc.array();

            for (QJsonArray::const_iterator iter = plugins.constBegin(); iter != plugins.constEnd(); ++iter) {
                const QJsonObject &obj = QJsonValue(*iter).toObject();
                const QString &candidate = obj.value(QStringLiteral("FileName")).toString();
                const KPluginMetaData m(obj, candidate);
                if (m.pluginId() == packageFormat) {
                    pluginFileName = candidate;
                    break;
                }
            }
        } else {
            QVector<KPluginMetaData> plugins = KPluginLoader::findPlugins(plugindir);
            QVectorIterator<KPluginMetaData> iter(plugins);
            while (iter.hasNext()) {
                auto md = iter.next();
                if (md.pluginId() == packageFormat) {
                    pluginFileName = md.fileName();
                    break;
                }
            }
        }
    }

    QString error;
    if (!pluginFileName.isEmpty()) {
        KPluginLoader loader(pluginFileName);
        const QVariantList argsWithMetaData = QVariantList() << loader.metaData().toVariantMap();
        KPluginFactory *factory = loader.factory();
        if (factory) {
            structure = factory->create<PackageStructure>(0, argsWithMetaData);
            if (!structure) {
                error = QCoreApplication::translate("", "No service matching the requirements was found");
            }
        }
    }

    if (structure && !error.isEmpty()) {
        qWarning() << i18n("Could not load installer for package of type %1. Error reported was: %2",
                            packageFormat, error);
    }

    d->structures.insert(packageFormat, structure);

    return structure;
}

Package PackageLoader::internalLoadPackage(const QString &name)
{
    Q_UNUSED(name);
    return Package();
}

} // Plasma Namespace
