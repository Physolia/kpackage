/*
 *   Copyright 2008 Aaron Seigo <aseigo@kde.org>
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2,
 *   or (at your option) any later version.
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

/**
 * kpackagetool5 exit codes used in this program

    0 No error

    1 Unspecified error
    2 Plugin is not installed
    3 Plugin or package invalid
    4 Installation failed, see stderr for reason
    5 Could not find a suitable installer for package type
    6 No install option given
    7 Conflicting arguments supplied
    8 Uninstallation failed, see stderr for reason
    9 Failed to generate package hash

*/

#include <klocalizedstring.h>
#include <qcommandlineparser.h>
#include <qcommandlineoption.h>

#include "kpackagetool.h"

int main(int argc, char **argv)
{
    QCommandLineParser parser;
    KPackage::PackageTool app(argc, argv, &parser);

    const QString description = i18n("KPackage Manager");
    const char version[] = "2.0";

    app.setApplicationVersion(version);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.setApplicationDescription(description);
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("hash"), i18nc("Do not translate <path>", "Generate a SHA1 hash for the package at <path>"), QStringLiteral("path")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("g") << QStringLiteral("global"), i18n("For install or remove, operates on packages installed for all users.")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("t") << QStringLiteral("type"),
                                        i18nc("theme, wallpaper, etc. are keywords, but they may be translated, as both versions "
                                                "are recognized by the application "
                                                "(if translated, should be same as messages with 'package type' context below)",
                                                "The type of package, e.g. theme, wallpaper, plasmoid, dataengine, runner, layout-template, etc."),
                                        QStringLiteral("type"), QStringLiteral("KPackage/Generic")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("i") << QStringLiteral("install"), i18nc("Do not translate <path>", "Install the package at <path>"), QStringLiteral("path")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("s") << QStringLiteral("show"), i18nc("Do not translate <name>", "Show information of package <name>"), QStringLiteral("name")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("u") << QStringLiteral("upgrade"), i18nc("Do not translate <path>", "Upgrade the package at <path>"), QStringLiteral("path")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("l") << QStringLiteral("list"), i18n("List installed packages")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("list-types"), i18n("List all known package types that can be installed")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("r") << QStringLiteral("remove"), i18nc("Do not translate <name>", "Remove the package named <name>"), QStringLiteral("name")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("p") << QStringLiteral("packageroot"), i18n("Absolute path to the package root. If not supplied, then the standard data directories for this KDE session will be searched instead."), QStringLiteral("path")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("generate-index"), i18n("Recreate the plugin index. To be used in conjunction with either the option -t or -g. Recreates the index for the given type or package root. Operates in the user directory, unless -g is used")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("appstream-metainfo"), i18nc("Do not translate <path>", "Outputs the metadata for the package <path>"), QStringLiteral("path")));

    parser.process(app);

    //at least one operation should be specified
    if (!parser.isSet(QStringLiteral("hash")) && !parser.isSet(QStringLiteral("g")) &&
        !parser.isSet(QStringLiteral("i")) && !parser.isSet(QStringLiteral("s")) && !parser.isSet(QStringLiteral("appstream-metainfo")) &&
        !parser.isSet(QStringLiteral("u")) && !parser.isSet(QStringLiteral("l")) &&
        !parser.isSet(QStringLiteral("list-types")) && !parser.isSet(QStringLiteral("r")) &&
        !parser.isSet(QStringLiteral("generate-index"))) {
        parser.showHelp(0);
    }
    return app.exec();
}

