#pragma once

#include "ui_RPCS3UpdaterQt.h"

#include <QMainWindow>
#include <QProgressDialog>
#include <QTimer>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QFile>
#include <QTemporaryDir>
#include <QProcess>
#include <QDirIterator>

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <memory>

static const QStringList forbidden_directories =
{
	"dev_hdd0" , "dev_hdd1" , "data" , "dev_flash" , "dev_usb000" , "shaderlog"
};
static const QString deprecated_extension = "rpcs3-deprecated";
static const QString api = "https://update.rpcs3.net/?c=XXXXXXXX";

class RPCS3UpdaterQt : public QMainWindow
{
	Q_OBJECT

public:
	RPCS3UpdaterQt(QWidget *parent = Q_NULLPTR);

private:
	bool ReadJSON(QByteArray data);
	QString SaveFile(QNetworkReply *network_reply);
	void ShowDownloadProgress(const QString& message);
	void Extract(const QString& path);
	static QByteArray GetFileHash(QFile *file, QCryptographicHash::Algorithm algorithm = QCryptographicHash::Algorithm::Md5);
	static void CleanUp(const QDir& directory = QDir(qApp->applicationDirPath()));
	void UpdateFiles();

	Ui::RPCS3UpdaterQtClass ui;

	std::unique_ptr<QNetworkAccessManager> network_access_manager;
	std::unique_ptr<QTemporaryDir> extraction_directory, download_directory;
	std::unique_ptr<QProcess> extract_process;
	QNetworkReply *network_reply;

private slots:
	void OnAbout();
	void OnCancel();
	void OnDownload();
	void OnDownloadFinished();
	void OnUpdate();
	void OnUpdateFinished();
	void OnErrorOccured(QProcess::ProcessError error);
};
