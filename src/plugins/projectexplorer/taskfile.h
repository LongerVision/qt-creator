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

#pragma once

#include "itaskhandler.h"

#include <coreplugin/idocument.h>

namespace ProjectExplorer {
namespace Internal {

class StopMonitoringHandler : public ITaskHandler
{
public:
    bool canHandle(const ProjectExplorer::Task &) const override;
    void handle(const ProjectExplorer::Task &) override;
    QAction *createAction(QObject *parent) const override;
};

class TaskFile : public Core::IDocument
{
public:
    TaskFile(QObject *parent);

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;

    bool load(QString *errorString, const Utils::FilePath &fileName);

    static TaskFile *openTasks(const Utils::FilePath &filePath);
    static void stopMonitoring();

private:
    static QList<TaskFile *> openFiles;
};

} // namespace Internal
} // namespace ProjectExplorer
