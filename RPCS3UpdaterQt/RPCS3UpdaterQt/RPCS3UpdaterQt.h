#pragma once

#include "ui_RPCS3UpdaterQt.h"

#include <QMainWindow>
#include <QProgressDialog>
#include <QTimer>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QFile>

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <memory>

class RPCS3UpdaterQt : public QMainWindow
{
	Q_OBJECT

public:
	RPCS3UpdaterQt(QWidget *parent = Q_NULLPTR);

private:
	bool ReadJSON(QByteArray data);
	void SaveFile(QNetworkReply *network_reply);
	void ShowProgress(QString message);

	Ui::RPCS3UpdaterQtClass ui;

	std::unique_ptr<QNetworkAccessManager> network_access_manager;
	QNetworkReply *network_reply;

	QString api = "https://update.rpcs3.net/?c=XXXXXXXX";

private slots:
	void OnAbout();
	void OnCancel();
	void OnDownload();
	void OnDownloadFinished();
	void OnUpdate();
	void OnUpdateFinished();
};
