/*
PluginPy
Copyright (C) 2018 Samson Wang

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "PluginMgr.h"
#include <pybind11/embed.h>
#include <pybind11/eval.h>
#include <pybind11/pytypes.h>
#include "ExportPyPlugin.h"
#include "PluginWrapper.h"
#include "InputData.h"

namespace py = pybind11;

namespace pluginpy {

pluginpy::PluginMgr& PluginMgr::instance() {
    static PluginMgr s_mgr;
    return s_mgr;
}

launchy::PluginInterface* PluginMgr::loadPlugin(const QString& pluginName, const QString& pluginPath) {
    qDebug() << "pluginpy::PluginMgr::loadPlugin, name:" << pluginName
        << "path:" << pluginPath;

    uint pluginId = qHash(pluginName);
    if (m_pluginInterface.contains(pluginId)) {
        qDebug() << "pluginpy::PluginMgr::loadPlugin, plugin alread loaded, name:" << pluginName
            << "path:" << pluginPath;
        return m_pluginInterface[pluginId];
    }

    if (!m_pluginObject.contains(pluginId)) {
//         QString pluginFileName = pluginPath + "/" + pluginName + ".py";
//         pluginFileName = QDir::toNativeSeparators(pluginFileName);
//         std::string fileName = pluginFileName.toLocal8Bit().data();

        // add dir to os path
        // Make .py files in the working directory available by default
        //module::import("sys").attr("path").cast<list>().append(".");
        //std::string path = QDir::toNativeSeparators(pluginPath).toStdString();
        py::list pathObj = py::module::import("sys").attr("path").cast<py::list>();
        pathObj.append(qPrintable(QDir::toNativeSeparators(pluginPath)));
        py::object mod = py::module::import(qPrintable(pluginName));
        py::object pluginClass = mod.attr("getPlugin")();

        m_pluginObject.insert(pluginId, pluginClass());
    }

    py::object& pluginObject = m_pluginObject[pluginId];

    if (py::isinstance<exportpy::Plugin>(pluginObject)) {
        qDebug() << "PluginMgr::loadPlugin, plugin load succeed";
        exportpy::Plugin* pluginPtr = pluginObject.cast<exportpy::Plugin*>();
        if (pluginPtr) {
            std::string name = pluginPtr->getName();
            qDebug() << "exportpy::registerPlugin, plugin name:" << name.c_str();
            launchy::PluginInterface* intf = new pluginpy::PluginWrapper(pluginPtr);
            // store this pointer in manager
            m_pluginInterface.insert(pluginId, intf);
            return intf;
        }
    }

    return nullptr;
}

bool PluginMgr::unloadPlugin(uint pluginId) {
    qDebug() << "pluginpy::PluginMgr::unloadPlugin, id:" << pluginId;

    launchy::PluginInterface* plugin = m_pluginInterface[pluginId];
    if (plugin) {
        delete plugin;
        plugin = nullptr;
    }
    m_pluginInterface.remove(pluginId);

    return true;
}

void PluginMgr::registerPlugin(py::object pluginClass) {
    qDebug() << "PluginMgr::registerPlugin, register plugin called";
    m_pluginClass.push_back(pluginClass);

}

PluginMgr::PluginMgr() {
    py::initialize_interpreter();

    QString pythonLibPath = qApp->applicationDirPath() + "/python";
    py::list pathObj = py::module::import("sys").attr("path").cast<py::list>();
    pathObj.append(qPrintable(QDir::toNativeSeparators(pythonLibPath)));
}

PluginMgr::~PluginMgr() {
    QHashIterator<uint, launchy::PluginInterface*> it1(m_pluginInterface);
    while (it1.hasNext()) {
        it1.next();
        delete it1.value();
    }
    m_pluginInterface.clear();
    m_pluginObject.clear();

    py::finalize_interpreter();
}

}