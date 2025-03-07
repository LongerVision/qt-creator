/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "taskfile.h"

#include "projectexplorer.h"
#include "session.h"
#include "taskhub.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>

#include <QAction>
#include <QMessageBox>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// TaskFile
// --------------------------------------------------------------------------

QList<TaskFile *> TaskFile::openFiles;

TaskFile::TaskFile(QObject *parent) : Core::IDocument(parent)
{
    setId("TaskList.TaskFile");
}

Core::IDocument::ReloadBehavior TaskFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool TaskFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(flag)

    if (type == TypeRemoved) {
        deleteLater();
        return true;
    }
    return load(errorString, filePath());
}

static Task::TaskType typeFrom(const QString &typeName)
{
    QString tmp = typeName.toLower();
    if (tmp.startsWith("warn"))
        return Task::Warning;
    if (tmp.startsWith("err"))
        return Task::Error;
    return Task::Unknown;
}

static QStringList parseRawLine(const QByteArray &raw)
{
    QStringList result;
    QString line = QString::fromUtf8(raw.constData());
    if (line.startsWith('#'))
        return result;

    return line.split('\t');
}

static QString unescape(const QString &input)
{
    QString result;
    for (int i = 0; i < input.count(); ++i) {
        if (input.at(i) == '\\') {
            if (i == input.count() - 1)
                continue;
            if (input.at(i + 1) == 'n') {
                result.append('\n');
                ++i;
                continue;
            } else if (input.at(i + 1) == 't') {
                result.append('\t');
                ++i;
                continue;
            } else if (input.at(i + 1) == '\\') {
                result.append('\\');
                ++i;
                continue;
            }
            continue;
        }
        result.append(input.at(i));
    }
    return result;
}

static bool parseTaskFile(QString *errorString, const FilePath &name)
{
    QFile tf(name.toString());
    if (!tf.open(QIODevice::ReadOnly)) {
        *errorString = ProjectExplorerPlugin::tr("Cannot open task file %1: %2").arg(
                name.toUserOutput(), tf.errorString());
        return false;
    }

    const FilePath parentDir = name.parentDir();
    while (!tf.atEnd()) {
        QStringList chunks = parseRawLine(tf.readLine());
        if (chunks.isEmpty())
            continue;

        QString description;
        QString file;
        Task::TaskType type = Task::Unknown;
        int line = -1;

        if (chunks.count() == 1) {
            description = chunks.at(0);
        } else if (chunks.count() == 2) {
            type = typeFrom(chunks.at(0));
            description = chunks.at(1);
        } else if (chunks.count() == 3) {
            file = chunks.at(0);
            type = typeFrom(chunks.at(1));
            description = chunks.at(2);
        } else if (chunks.count() >= 4) {
            file = chunks.at(0);
            bool ok;
            line = chunks.at(1).toInt(&ok);
            if (!ok)
                line = -1;
            type = typeFrom(chunks.at(2));
            description = chunks.at(3);
        }
        if (!file.isEmpty()) {
            file = QDir::fromNativeSeparators(file);
            QFileInfo fi(file);
            if (fi.isRelative())
                file = parentDir.pathAppended(file).toString();
        }
        description = unescape(description);

        TaskHub::addTask(Task(type, description, FilePath::fromUserInput(file), line,
                              Constants::TASK_CATEGORY_TASKLIST_ID));
    }
    return true;
}

static void clearTasks()
{
    TaskHub::clearTasks(Constants::TASK_CATEGORY_TASKLIST_ID);
}

void TaskFile::stopMonitoring()
{
    SessionManager::setValue(Constants::SESSION_TASKFILE_KEY, {});

    for (TaskFile *document : qAsConst(openFiles))
        document->deleteLater();
    openFiles.clear();
}

bool TaskFile::load(QString *errorString, const FilePath &fileName)
{
    setFilePath(fileName);
    clearTasks();

    bool result = parseTaskFile(errorString, fileName);
    if (result) {
        if (!SessionManager::isDefaultSession(SessionManager::activeSession()))
            SessionManager::setValue(Constants::SESSION_TASKFILE_KEY, fileName.toVariant());
    } else {
        stopMonitoring();
    }

    return result;
}

TaskFile *TaskFile::openTasks(const FilePath &filePath)
{
    TaskFile *file = Utils::findOrDefault(openFiles, Utils::equal(&TaskFile::filePath, filePath));
    QString errorString;
    if (file) {
        file->load(&errorString, filePath);
    } else {
        file = new TaskFile(ProjectExplorerPlugin::instance());

        if (!file->load(&errorString, filePath)) {
            QMessageBox::critical(Core::ICore::dialogParent(), tr("File Error"), errorString);
            delete file;
            return nullptr;
        }

        openFiles.append(file);

        // Register with filemanager:
        Core::DocumentManager::addDocument(file);
    }
    return file;
}

bool StopMonitoringHandler::canHandle(const ProjectExplorer::Task &task) const
{
    return task.category == Constants::TASK_CATEGORY_TASKLIST_ID;
}

void StopMonitoringHandler::handle(const ProjectExplorer::Task &task)
{
    QTC_ASSERT(canHandle(task), return);
    Q_UNUSED(task)
    TaskFile::stopMonitoring();
}

QAction *StopMonitoringHandler::createAction(QObject *parent) const
{
    const QString text =
            QCoreApplication::translate("TaskList::Internal::StopMonitoringHandler",
                                        "Stop Monitoring");
    const QString toolTip =
            QCoreApplication::translate("TaskList::Internal::StopMonitoringHandler",
                                        "Stop monitoring task files.");
    auto stopMonitoringAction = new QAction(text, parent);
    stopMonitoringAction->setToolTip(toolTip);
    return stopMonitoringAction;
}

} // namespace Internal
} // namespace ProjectExplorer
