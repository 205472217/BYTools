#pragma once

#include <QObject>
#include <QString>

#include "NamePreviewModel.h"
#include "NameService.h"
#include "TextConverter.h"

class BatchNameController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString rootPath READ rootPath WRITE setRootPath NOTIFY rootPathChanged)
    Q_PROPERTY(int targetType READ targetType WRITE setTargetType NOTIFY targetTypeChanged)
    Q_PROPERTY(bool recursive READ recursive WRITE setRecursive NOTIFY recursiveChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool hasRecords READ hasRecords NOTIFY hasRecordsChanged)
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)

public:
    explicit BatchNameController(QObject *parent = nullptr);

    QString rootPath() const;
    void setRootPath(const QString &rootPath);

    int targetType() const;
    void setTargetType(int targetType);

    bool recursive() const;
    void setRecursive(bool recursive);

    QString statusMessage() const;
    bool hasRecords() const;
    bool isProcessing() const;
    Q_INVOKABLE QObject* previewModel();

    Q_INVOKABLE void buildPreview();
    Q_INVOKABLE void executeRename();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void restoreRecord(int row);
    Q_INVOKABLE void clearRecords();
    Q_INVOKABLE void reset();

signals:
    void rootPathChanged();
    void targetTypeChanged();
    void recursiveChanged();
    void statusMessageChanged();
    void hasRecordsChanged();
    void isProcessingChanged();

private:
    NameService::TargetType currentTargetType() const;
    void setStatusMessage(const QString &message);

    void setIsProcessing(bool processing);

    QString m_rootPath;
    int m_targetType = 2;
    bool m_recursive = false;
    bool m_isProcessing = false;
    QString m_statusMessage;
    ChineseTextConverter m_converter;
    NameService m_service;
    NamePreviewModel m_previewModel;
};