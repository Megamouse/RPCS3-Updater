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
	~RPCS3UpdaterQt();

private:
	bool ReadJSON(QByteArray data);
	void Download();
	void SaveFile(QNetworkReply *network_reply);
	void ShowProgress(QString message);

	Ui::RPCS3UpdaterQtClass ui;

	std::unique_ptr<QTimer> progress_timer;
	std::unique_ptr<QProgressDialog> progress_dialog;
	QNetworkReply *network_reply;

	QString api = "https://update.rpcs3.net/?c=XXXXXXXX";
	QString latest;

private slots:
	void OnAbout();
	void OnUpdate();
};
