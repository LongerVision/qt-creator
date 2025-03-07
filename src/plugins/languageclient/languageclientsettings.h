/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "languageclient_global.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QAbstractItemModel>
#include <QCoreApplication>
#include <QJsonObject>
#include <QLabel>
#include <QPointer>
#include <QUuid>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
QT_END_NAMESPACE

namespace Utils {
class CommandLine;
class FilePath;
class PathChooser;
class FancyLineEdit;
} // namespace Utils

namespace Core { class IDocument; }
namespace ProjectExplorer { class Project; }
namespace TextEditor { class BaseTextEditor; }

namespace LanguageClient {

class Client;
class BaseClientInterface;

class LANGUAGECLIENT_EXPORT LanguageFilter
{
public:
    QStringList mimeTypes;
    QStringList filePattern;
    bool isSupported(const Utils::FilePath &filePath, const QString &mimeType) const;
    bool isSupported(const Core::IDocument *document) const;
    bool operator==(const LanguageFilter &other) const;
    bool operator!=(const LanguageFilter &other) const;
};

class LANGUAGECLIENT_EXPORT BaseSettings
{
public:
    BaseSettings() = default;

    virtual ~BaseSettings() = default;

    enum StartBehavior {
        AlwaysOn = 0,
        RequiresFile,
        RequiresProject,
        LastSentinel
    };

    QString m_name = QString("New Language Server");
    QString m_id = QUuid::createUuid().toString();
    Utils::Id m_settingsTypeId;
    bool m_enabled = true;
    StartBehavior m_startBehavior = RequiresFile;
    LanguageFilter m_languageFilter;
    QString m_initializationOptions;
    QString m_configuration;

    QJsonObject initializationOptions() const;
    QJsonValue configuration() const;

    virtual bool applyFromSettingsWidget(QWidget *widget);
    virtual QWidget *createSettingsWidget(QWidget *parent = nullptr) const;
    virtual BaseSettings *copy() const { return new BaseSettings(*this); }
    virtual bool isValid() const;
    Client *createClient() const;
    Client *createClient(ProjectExplorer::Project *project) const;
    virtual QVariantMap toMap() const;
    virtual void fromMap(const QVariantMap &map);

protected:
    virtual BaseClientInterface *createInterface(ProjectExplorer::Project *) const;
    virtual Client *createClient(BaseClientInterface *interface) const;

    BaseSettings(const BaseSettings &other) = default;
    BaseSettings(BaseSettings &&other) = default;
    BaseSettings &operator=(const BaseSettings &other) = default;
    BaseSettings &operator=(BaseSettings &&other) = default;

private:
    bool canStart(QList<const Core::IDocument *> documents) const;
};

class LANGUAGECLIENT_EXPORT StdIOSettings : public BaseSettings
{
public:
    StdIOSettings() = default;
    ~StdIOSettings() override = default;

    Utils::FilePath m_executable;
    QString m_arguments;

    bool applyFromSettingsWidget(QWidget *widget) override;
    QWidget *createSettingsWidget(QWidget *parent = nullptr) const override;
    BaseSettings *copy() const override { return new StdIOSettings(*this); }
    bool isValid() const override;
    QVariantMap toMap() const override;
    void fromMap(const QVariantMap &map) override;
    QString arguments() const;
    Utils::CommandLine command() const;

protected:
    BaseClientInterface *createInterface(ProjectExplorer::Project *project) const override;

    StdIOSettings(const StdIOSettings &other) = default;
    StdIOSettings(StdIOSettings &&other) = default;
    StdIOSettings &operator=(const StdIOSettings &other) = default;
    StdIOSettings &operator=(StdIOSettings &&other) = default;
};

struct ClientType {
    Utils::Id id;
    QString name;
    using SettingsGenerator = std::function<BaseSettings*()>;
    SettingsGenerator generator = nullptr;
};

class LANGUAGECLIENT_EXPORT LanguageClientSettings
{
    Q_DECLARE_TR_FUNCTIONS(LanguageClientSettings)
public:
    static void init();
    static QList<BaseSettings *> fromSettings(QSettings *settings);
    static QList<BaseSettings *> pageSettings();
    static QList<BaseSettings *> changedSettings();

    /**
     * must be called before the delayed initialize phase
     * otherwise the settings are not loaded correctly
     */
    static void registerClientType(const ClientType &type);
    static void addSettings(BaseSettings *settings);
    static void enableSettings(const QString &id);
    static void toSettings(QSettings *settings, const QList<BaseSettings *> &languageClientSettings);

    static bool outlineComboBoxIsSorted();
    static void setOutlineComboBoxSorted(bool sorted);
};

class LANGUAGECLIENT_EXPORT BaseSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BaseSettingsWidget(const BaseSettings* settings, QWidget *parent = nullptr);
    ~BaseSettingsWidget() override = default;

    QString name() const;
    LanguageFilter filter() const;
    BaseSettings::StartBehavior startupBehavior() const;
    bool alwaysOn() const;
    bool requiresProject() const;
    QString initializationOptions() const;

private:
    void showAddMimeTypeDialog();

    QLineEdit *m_name = nullptr;
    QLabel *m_mimeTypes = nullptr;
    QLineEdit *m_filePattern = nullptr;
    QComboBox *m_startupBehavior = nullptr;
    Utils::FancyLineEdit *m_initializationOptions = nullptr;

    static constexpr char filterSeparator = ';';
};

class LANGUAGECLIENT_EXPORT StdIOSettingsWidget : public BaseSettingsWidget
{
    Q_OBJECT
public:
    explicit StdIOSettingsWidget(const StdIOSettings* settings, QWidget *parent = nullptr);
    ~StdIOSettingsWidget() override = default;

    Utils::FilePath executable() const;
    QString arguments() const;

private:
    Utils::PathChooser *m_executable = nullptr;
    QLineEdit *m_arguments = nullptr;
};

LANGUAGECLIENT_EXPORT TextEditor::BaseTextEditor *jsonEditor();

} // namespace LanguageClient
