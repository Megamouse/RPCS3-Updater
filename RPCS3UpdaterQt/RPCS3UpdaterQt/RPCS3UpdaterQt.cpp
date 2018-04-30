#include "RPCS3UpdaterQt.h"

#include <QMessageBox>
#include <QPushButton>
#include <QLabel>

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

RPCS3UpdaterQt::RPCS3UpdaterQt(QWidget *parent)
	: QMainWindow(parent)
{
	//================================================================================
	// General
	//================================================================================

	ui.setupUi(this);

	//================================================================================
	// Menu
	//================================================================================

	// connect 'Help->About' to the about dialog
	connect(ui.actionAbout, &QAction::triggered, this, &RPCS3UpdaterQt::OnAbout);

	// connect 'Help->About Qt' to the pre shipped Qt about dialog
	connect(ui.actionAbout_Qt, &QAction::triggered, qApp, &QApplication::aboutQt);

	//================================================================================
	// Buttons
	//================================================================================

	connect(ui.updateButton, &QPushButton::clicked, this, &RPCS3UpdaterQt::OnUpdate);
}

void RPCS3UpdaterQt::OnAbout()
{
}

void RPCS3UpdaterQt::OnUpdate()
{
	// Check for SSL and abort in case it is not supported
	if (QSslSocket::supportsSsl() == false)
	{
		QMessageBox::critical(nullptr, tr("Warning!"), tr("Can not retrieve the Update! Please make sure your system supports SSL."));
		return;
	}

	// Send network request and wait for response
	QNetworkAccessManager *network_access_manager = new QNetworkAccessManager();
	QNetworkRequest network_request = QNetworkRequest(QUrl(api + file));
	QNetworkReply *network_reply = network_access_manager->get(network_request);

	// Show Progress
	progress_dialog.reset(new QProgressDialog(tr(".Please wait."), tr("Abort"), 0, 100));
	progress_dialog->setWindowTitle(tr("Downloading Update"));
	progress_dialog->setFixedWidth(QLabel("This is the very length of the progressbar due to hidpi reasons.").sizeHint().width());
	progress_dialog->setValue(0);
	progress_dialog->show();

	// Animate progress dialog a bit
	int timer_count = 0;
	progress_timer.reset(new QTimer(this));
	connect(progress_timer.get(), &QTimer::timeout, [&]()
	{
		switch (++timer_count % 3)
		{
		case 0:
			timer_count = 0;
			progress_dialog->setLabelText(tr(".Please wait."));
			break;
		case 1:
			progress_dialog->setLabelText(tr("..Please wait.."));
			break;
		default:
			progress_dialog->setLabelText(tr("...Please wait..."));
			break;
		}
	});
	progress_timer->start(500);

	// Handle new progress
	connect(network_reply, &QNetworkReply::downloadProgress, [&](qint64 bytesReceived, qint64 bytesTotal)
	{
		progress_dialog->setMaximum(bytesTotal);
		progress_dialog->setValue(bytesReceived);
	});

	// Handle abort
	connect(progress_dialog.get(), &QProgressDialog::rejected, network_reply, &QNetworkReply::abort);

	// Handle response according to its contents
	connect(network_reply, &QNetworkReply::finished, [=]()
	{
		// Clean up Progress Dialog
		if (progress_dialog)
		{
			progress_dialog->close();
		}

		// Clean up Progress Timer
		if (progress_timer)
		{
			progress_timer->stop();
		}

		// Handle Errors
		if (network_reply->error() != QNetworkReply::NoError)
		{
			// We failed to retrieve a new update
			QMessageBox::critical(nullptr, tr("Error!"), network_reply->errorString());
			return;
		}

		// Read data from network reply
		ReadJSON(network_reply->readAll());

		// Clean up network reply
		network_reply->deleteLater();
	});
}

void RPCS3UpdaterQt::ReadJSON(QByteArray data)
{
	// Read JSON data
	QJsonObject json_data = QJsonDocument::fromJson(data).object();

	int return_code = json_data["return_code"].toInt();

	if (return_code < 0)
	{
		// We failed to retrieve a new update

		QString error_message;

		switch (return_code)
		{
		case -1:
			error_message = tr("Server Error - Internal Error");
			break;
		case -2:
			error_message = tr("Server Error - Maintenance Mode");
			break;
		default:
			error_message = tr("Server Error - Unknown Error");
			break;
		}

		QMessageBox::critical(nullptr, tr("Error code %0!").arg(return_code), error_message + "\n\n" + api + file);
		return;
	}

	// Check for appveyor node
	if (!json_data["appveyor"].isObject())
	{
		QMessageBox::critical(nullptr, tr("Error!"), tr("No appveyor found!"));
		return;
	}

	// String retrieved from JSON
	QString result = json_data["appveyor"].toString();
	QMessageBox::information(nullptr, tr("Error!"), tr("The response is: %0").arg(result));
}
