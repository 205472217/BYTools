#pragma once

#include <QObject>
#include <QString>
#include <QThread>

#include "NamePreviewModel.h"
#include "NameService.h"
#include "TextConverter.h"

class PluginLogger;
class NameConverterSettings;

class NameConverterController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool hasRecords READ hasRecords NOTIFY hasRecordsChanged)
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)
    Q_PROPERTY(NamePreviewModel* previewModel READ previewModel CONSTANT)

    // === Config properties (delegated to NameConverterSettings) ===
    Q_PROPERTY(QString rootPath READ rootPath WRITE setRootPath NOTIFY rootPathChanged)
    Q_PROPERTY(int targetType READ targetType WRITE setTargetType NOTIFY targetTypeChanged)
    Q_PROPERTY(bool recursive READ recursive WRITE setRecursive NOTIFY recursiveChanged)

public:
    explicit NameConverterController(PluginLogger *logger, NameConverterSettings *settings, QObject *parent = nullptr);
    ~NameConverterController() override;

    QString statusMessage() const;
    bool hasRecords() const;
    bool isProcessing() const;
    NamePreviewModel* previewModel() const;

    QString rootPath() const;
    int targetType() const;
    bool recursive() const;

    void setRootPath(const QString &path);
    void setTargetType(int type);
    void setRecursive(bool rec);

    Q_INVOKABLE void buildPreview();
    Q_INVOKABLE void executeRename();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void restoreRecord(int row);
    Q_INVOKABLE void restoreAllRecords();
    Q_INVOKABLE void clearRecords();
    Q_INVOKABLE void reset();

signals:
    void statusMessageChanged();
    void hasRecordsChanged();
    void isProcessingChanged();
    void rootPathChanged();
    void targetTypeChanged();
    void recursiveChanged();
    void logMessage(const QString &message);

private:
    NameService::TargetType currentTargetType() const;

    void setStatusMessage(const QString &message);
    void setIsProcessing(bool processing);
    bool m_isProcessing = false;
    QString m_statusMessage;
    ChineseTextConverter m_converter;
    PluginLogger *m_logger = nullptr;
    NameService m_service;
    NamePreviewModel *m_previewModel = nullptr;
    NameConverterSettings *m_settings = nullptr;

    QThread m_workerThread;
    bool m_workerRunning = false;
};
