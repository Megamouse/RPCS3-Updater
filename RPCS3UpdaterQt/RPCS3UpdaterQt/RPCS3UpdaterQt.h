#pragma once

#include "ui_RPCS3UpdaterQt.h"

#include <QMainWindow>
#include <QProgressDialog>
#include <QTimer>

#include <memory>

class RPCS3UpdaterQt : public QMainWindow
{
	Q_OBJECT

public:
	RPCS3UpdaterQt(QWidget *parent = Q_NULLPTR);

private:
	void ReadJSON(QByteArray data);

	Ui::RPCS3UpdaterQtClass ui;

	std::unique_ptr<QTimer> progress_timer;
	std::unique_ptr<QProgressDialog> progress_dialog;

	QString api = "https://update.rpcs3.net/?c=XXXXXXXX";
	QString latest;

private slots:
	void OnAbout();
	void OnUpdate();
};
