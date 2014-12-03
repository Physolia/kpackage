/*
 *   Copyright 2010 by Ryan Rix <ry@n.rix.si>
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

#ifndef KPACKAGE_LOADER_H
#define KPACKAGE_LOADER_H

#include <kpackage/package.h>

#include <kpackage/package_export.h>

namespace KPackage
{

class PackageLoaderPrivate;

/**
 * This is an abstract base class which defines an interface to which Plasma's
 * Applet Loading logic can communicate with a parent application. The plugin loader
 * must be set before any plugins are loaded, otherwise (for safety reasons), the
 * default PackageLoader implementation will be used. The reimplemented version should
 * not do more than simply returning a loaded plugin. It should not init() it, and it should not
 * hang on to it. The associated methods will be called only when a component of Plasma
 * needs to load a _new_ plugin. (e.g. DataEngine does its own caching).
 *
 * @author Ryan Rix <ry@n.rix.si>
 * @since 4.6
 **/
class PACKAGE_EXPORT PackageLoader
{
public:
    /**
     * Load a Package plugin.
     *
     * @param packageFormat the format of the package to load
     * @param packagePath the package name: the path of the package relative to the
     *        packageFormat root path. If not specified it will have to be set manually
     *        with Package::setPath() by the caller.
     *
     * @return a Package object matching name, or an invalid package on failure
     **/
    Package loadPackage(const QString &packageFormat, const QString &packagePath = QString());

    QList<KPluginMetaData> listPackages(const QString &packageFormat, const QString &packageRoot = QString());

    KPackage::PackageStructure *loadPackageStructure(const QString &packageFormat);

    /**
     * Set the plugin loader which will be queried for all loads.
     *
     * @param loader A subclass of PackageLoader which will be supplied
     * by the application
     **/
    static void setPackageLoader(PackageLoader *loader);

    /**
     * Return the active plugin loader
     **/
    static PackageLoader *self();

protected:

    /**
     * A re-implementable method that allows subclasses to override
     * the default behaviour of loadPackage. If the service requested is not recognized,
     * then the implementation should return an empty and invalid Package(). 
     * This method is called
     * by loadPackage prior to attempting to load a Package using the standard
     * plugin mechanisms.
     *
     * @param packageFormat the format of the package to load
     *
     * @return a Package instance with the proper PackageStructure
     **/
    virtual Package internalLoadPackage(const QString &packageFormat);

    PackageLoader();
    virtual ~PackageLoader();

private:
    PackageLoaderPrivate *const d;
};

}

Q_DECLARE_METATYPE(KPackage::PackageLoader *)

#endif