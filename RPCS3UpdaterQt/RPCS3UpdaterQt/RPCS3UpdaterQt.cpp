#include "RPCS3UpdaterQt.h"

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

RPCS3UpdaterQt::~RPCS3UpdaterQt()
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
	QNetworkRequest network_request = QNetworkRequest(QUrl(api));
	network_reply = network_access_manager->get(network_request);

	// Initialise and show progress bar
	ShowProgress(tr("Downloading update information"));

	// Handle response according to its contents
	connect(network_reply, &QNetworkReply::finished, [=]()
	{
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

		// Download the latest build
		Download();
	});
}

bool RPCS3UpdaterQt::ReadJSON(QByteArray data)
{
	// Read JSON data
	QJsonObject json_data = QJsonDocument::fromJson(data).object();

	int return_code = json_data["return_code"].toInt();

	if (return_code < -1)
	{
		// We failed to retrieve a new update
		QString error_message;

		switch (return_code)
		{
		//case -1:
		//	error_message = tr("Server Error - Internal Error");
		//	break;
		case -2:
			error_message = tr("Server Error - Maintenance Mode");
			break;
		default:
			error_message = tr("Server Error - Unknown Error");
			break;
		}

		QMessageBox::critical(nullptr, tr("Error code %0!").arg(return_code), error_message + "\n\n" + api);
		return false;
	}

	// Check for latest_build node
	if (!json_data["latest_build"].isObject())
	{
		QMessageBox::critical(nullptr, tr("Error!"), tr("No latest build found!"));
		return false;
	}

	QJsonObject latest_build = json_data["latest_build"].toObject();

#ifdef _WIN32
	QJsonObject os = latest_build["windows"].toObject();
#elif __linux__
	QJsonObject os = latest_build["linux"].toObject();
#endif

	QString pr = latest_build.value("pr").toString();
	QString datetime = os.value("datetime").toString();
	latest = os.value("download").toString();

	QMessageBox::information(nullptr, tr("Success!"), tr("PR: %0\nDatetime: %1\nDownload: %2\n").arg(pr).arg(datetime).arg(latest));
	return true;
}

void RPCS3UpdaterQt::Download()
{
	QNetworkAccessManager *network_access_manager = new QNetworkAccessManager();
	QNetworkRequest network_request = QNetworkRequest(QUrl(latest));
	network_reply = network_access_manager->get(network_request);

	// Initialise and show progress bar
	ShowProgress(tr("Downloading latest build"));

	// Handle response according to its contents
	connect(network_reply, &QNetworkReply::finished, [=]()
	{
		// Handle Errors
		if (network_reply->error() != QNetworkReply::NoError)
		{
			// We failed to retrieve a new update
			QMessageBox::critical(nullptr, tr("Error!"), network_reply->errorString());
			return;
		}

		// Read data from network reply
		SaveFile(network_reply);

		// Clean up network reply
		network_reply->deleteLater();
	});
}

void RPCS3UpdaterQt::SaveFile(QNetworkReply *network_reply)
{
	QString filename = network_reply->url().fileName();
	QFile file(filename);

	if (!file.open(QIODevice::WriteOnly))
	{
		QMessageBox::critical(nullptr, tr("Error!"), tr("Could not open %0 for writing: %1").arg(filename).arg(file.errorString()));
		return;
	}

	file.write(network_reply->readAll());
	file.close();
}

void RPCS3UpdaterQt::ShowProgress(QString message)
{
	// Show Progress
	progress_dialog.reset(new QProgressDialog(tr(".Please wait."), tr("Abort"), 0, 100));
	progress_dialog->setWindowTitle(message);
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
	connect(network_reply, &QNetworkReply::downloadProgress, [this](qint64 bytesReceived, qint64 bytesTotal)
	{
		progress_dialog->setMaximum(bytesTotal);
		progress_dialog->setValue(bytesReceived);
	});

	// Handle abort
	connect(progress_dialog.get(), &QProgressDialog::rejected, network_reply, &QNetworkReply::abort);

	// Clean Up
	connect(network_reply, &QNetworkReply::finished, [this]()
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
	});
}
