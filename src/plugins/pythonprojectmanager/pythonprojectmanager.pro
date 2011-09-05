TEMPLATE = lib
TARGET = PythonProjectManager
include(../../qtcreatorplugin.pri)
include(pythonprojectmanager_dependencies.pri)
HEADERS = pythonproject.h \
    pythonprojectplugin.h \
    pythontarget.h \
    pythonprojectmanager.h \
    pythonprojectconstants.h \
    pythonprojectnodes.h \
    pythonprojectwizard.h \
    pythonprojectfileseditor.h \
    pkgconfigtool.h \
    pythonmakestep.h \
    pythonbuildconfiguration.h \
    selectablefilesmodel.h \
    filesselectionwizardpage.h
SOURCES = pythonproject.cpp \
    pythonprojectplugin.cpp \
    pythontarget.cpp \
    pythonprojectmanager.cpp \
    pythonprojectnodes.cpp \
    pythonprojectwizard.cpp \
    pythonprojectfileseditor.cpp \
    pkgconfigtool.cpp \
    pythonmakestep.cpp \
    pythonbuildconfiguration.cpp \
    selectablefilesmodel.cpp \
    filesselectionwizardpage.cpp
RESOURCES += pythonproject.qrc
FORMS += pythonmakestep.ui
