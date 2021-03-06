#include "RPCS3UpdaterQt.h"

RPCS3UpdaterQt::RPCS3UpdaterQt(QWidget *parent)
	: QMainWindow(parent)
{
	//================================================================================
	// General
	//================================================================================

	ui.setupUi(this);

	network_access_manager.reset(new QNetworkAccessManager());
	network_access_manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

	CleanUp(qApp->applicationDirPath());

	//================================================================================
	// Menu
	//================================================================================

	// connect 'File->Exit' to quit the application
	connect(ui.actionExit, &QAction::triggered, this, &RPCS3UpdaterQt::close);

	// connect 'Help->About' to the about dialog
	connect(ui.actionAbout, &QAction::triggered, this, &RPCS3UpdaterQt::OnAbout);

	// connect 'Help->About Qt' to the pre shipped Qt about dialog
	connect(ui.actionAbout_Qt, &QAction::triggered, qApp, &QApplication::aboutQt);

	//================================================================================
	// Buttons
	//================================================================================

	connect(ui.updateButton, &QPushButton::clicked, this, &RPCS3UpdaterQt::OnUpdate);
	connect(ui.downloadButton, &QPushButton::clicked, this, &RPCS3UpdaterQt::OnDownload);
	connect(ui.cancelButton, &QPushButton::clicked, this, &RPCS3UpdaterQt::OnCancel);
}

void RPCS3UpdaterQt::OnAbout()
{
}

void RPCS3UpdaterQt::OnCancel()
{
	if (!network_reply)
	{
		return;
	}

	network_reply->abort();
}

void RPCS3UpdaterQt::OnUpdate()
{
	// Check for SSL and abort in case it is not supported
	if (!QSslSocket::supportsSsl())
	{
		QMessageBox::critical(nullptr, tr("Warning!"), tr("Can not retrieve the Update! Please make sure your system supports SSL."));
		return;
	}

	ui.cancelButton->setEnabled(true);
	ui.updateButton->setEnabled(false);
	ui.downloadButton->setEnabled(false);

	// Send network request and wait for response
	QNetworkRequest network_request = QNetworkRequest(QUrl(api));
	network_reply = network_access_manager->get(network_request);

	// Initialise and show progress bar
	ShowDownloadProgress(tr("Downloading build info."));

	// Handle response according to its contents
	connect(network_reply, &QNetworkReply::finished, this, &RPCS3UpdaterQt::OnUpdateFinished);
}

void RPCS3UpdaterQt::OnUpdateFinished()
{
	if (!network_reply)
	{
		return;
	}

	// Handle Errors
	switch (network_reply->error())
	{
	case QNetworkReply::NoError:
		ReadJSON(network_reply->readAll());
		ui.progressLabel->setText(tr("Build info retrieved"));
		break;
	case QNetworkReply::OperationCanceledError:
		ui.progressLabel->setText(tr("Build info canceled"));
		break;
	default:
		ui.progressLabel->setText(tr("Build info error"));
		QMessageBox::critical(nullptr, tr("Error!"), network_reply->errorString());
		break;
	}

	// Clean up network reply
	network_reply->deleteLater();

	ui.cancelButton->setEnabled(false);
	ui.updateButton->setEnabled(true);
	ui.downloadButton->setEnabled(network_reply->error() == QNetworkReply::NoError);
}

void RPCS3UpdaterQt::OnDownload()
{
	const QUrl latest(ui.download_data->text());

	if (latest.isValid() == false)
	{
		return;
	}

	ui.cancelButton->setEnabled(true);
	ui.updateButton->setEnabled(false);
	ui.downloadButton->setEnabled(false);

	QNetworkRequest network_request = QNetworkRequest(latest);
	network_reply = network_access_manager->get(network_request);

	// Initialise and show progress bar
	ShowDownloadProgress(tr("Downloading latest build."));

	// Handle response according to its contents
	connect(network_reply, &QNetworkReply::finished, this, &RPCS3UpdaterQt::OnDownloadFinished);
}

void RPCS3UpdaterQt::OnDownloadFinished()
{
	if (!network_reply)
	{
		return;
	}

	// Handle Errors
	switch (network_reply->error())
	{
	case QNetworkReply::NoError:
		Extract(SaveFile(network_reply));
		ui.progressLabel->setText(tr("Download finished"));
		break;
	case QNetworkReply::OperationCanceledError:
		ui.progressLabel->setText(tr("Download canceled"));
		break;
	default:
		ui.progressLabel->setText(tr("Download error"));
		QMessageBox::critical(nullptr, tr("Error!"), network_reply->errorString());
		break;
	}

	ui.cancelButton->setEnabled(false);
	ui.updateButton->setEnabled(true);
	ui.downloadButton->setEnabled(true);

	// Clean up network reply
	network_reply->deleteLater();
}

bool RPCS3UpdaterQt::ReadJSON(QByteArray data)
{
	// Read JSON data
	const QJsonObject json_data = QJsonDocument::fromJson(data).object();
	const int return_code = json_data["return_code"].toInt();

	if (return_code < -1/*0*/)
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

	const QJsonObject latest_build = json_data["latest_build"].toObject();

#ifdef _WIN32
	const QJsonObject os = latest_build["windows"].toObject();
#elif __linux__
	const QJsonObject os = latest_build["linux"].toObject();
#endif

	ui.pr_data->setText(latest_build.value("pr").toString());
	ui.datetime_data->setText(os.value("datetime").toString());
	ui.download_data->setText(os.value("download").toString());

	return true;
}

QString RPCS3UpdaterQt::SaveFile(QNetworkReply *network_reply)
{
	if (!network_reply)
	{
		return "";
	}

	download_directory.reset(new QTemporaryDir());

	const QString filename = download_directory->path() + "/" + network_reply->url().fileName();
	QFile file(filename);

	if (!file.open(QIODevice::WriteOnly))
	{
		QMessageBox::critical(nullptr, tr("Error!"), tr("Could not open %0 for writing: %1").arg(filename).arg(file.errorString()));
		return nullptr;
	}

	file.write(network_reply->readAll());
	file.close();

	return filename;
}

void RPCS3UpdaterQt::Extract(const QString& path)
{
	if (path.isEmpty())
	{
		return;
	}

	extraction_directory.reset(new QTemporaryDir());
	QProcess extract_process;

	connect(&extract_process, &QProcess::errorOccurred, this, &RPCS3UpdaterQt::OnErrorOccured);
	connect(&extract_process, &QProcess::readyReadStandardOutput, [&extract_process]()
	{
		if (extract_process.canReadLine())
		{
			qDebug() << extract_process.readLine();
		}
	});

#ifdef _WIN32
	const QString file = QString(R"(./7za.exe x -aoa -o"%1" "%2")").arg(extraction_directory->path(), path);
	qDebug() << "Extraction file: " << file;
	qDebug() << "Extraction dir: " << extraction_directory->path();
	extract_process.start(file);

	connect(&extract_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=, &extract_process](int exit_code, QProcess::ExitStatus exit_status)
	{
		qDebug() << "Extraction Finished";

		switch (exit_status)
		{
		case QProcess::NormalExit:
		{
			qDebug() << "Extraction exit code: " << exit_code;
			qDebug() << extract_process.readAll();
			UpdateFiles();
			break;
		}
		case QProcess::CrashExit:
		default:
			// TODO: Handle error
			qDebug() << "Extraction Error: " << exit_status;
			break;
		}

		extract_process.close();

		if (download_directory->isValid() && !download_directory->remove())
		{
			qDebug() << "Could not remove download_directory!";
		}

		if (extraction_directory->isValid() && !extraction_directory->remove())
		{
			qDebug() << "Could not remove extraction_directory!";
		}
	});

#elif __linux__
#endif
}

void RPCS3UpdaterQt::UpdateFiles()
{
	QDirIterator it(extraction_directory->path(), QDirIterator::Subdirectories);
	while (it.hasNext())
	{
		const QFileInfo info = QFileInfo(it.next());
		if (!info.exists())
		{
			continue;
		}

		const QString old_path = info.absoluteFilePath().replace(extraction_directory->path(), qApp->applicationDirPath());

		if (info.isFile())
		{
			QFile new_file(info.absoluteFilePath());
			QFile old_file(old_path);

			if (GetFileHash(&new_file) != GetFileHash(&old_file))
			{
				old_file.rename(old_file.fileName() + "." + deprecated_extension);
				new_file.rename(old_path);
			}

			continue;
		}

		if (info.isDir())
		{
			QDir dir(old_path);
			if (!dir.exists() && !dir.mkpath(old_path))
			{
				qDebug() << "Update error: " << old_path;
				// TODO: Handle error
				// Cleanup(extraction_directory->path());
				// return;
			}
		}
	}
}

QByteArray RPCS3UpdaterQt::GetFileHash(QFile *file, QCryptographicHash::Algorithm algorithm)
{
	if (!file || !file->exists())
	{
		return QByteArray();
	}

	file->open(QIODevice::ReadOnly);
	QByteArray hash = QCryptographicHash::hash(file->readAll(), algorithm);
	file->close();

	return hash;
}

void RPCS3UpdaterQt::CleanUp(const QDir& directory)
{
	const QFileInfoList files = directory.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);

	for (const QFileInfo& file_info : files)
	{
		if (forbidden_directories.contains(file_info.fileName()) || !file_info.exists())
		{
			continue;
		}

		if (file_info.isDir()) 
		{
			CleanUp(QDir(file_info.absoluteFilePath()));
			continue;
		}

		if (file_info.suffix().compare(deprecated_extension) == 0)
		{
			QFile deprecated_file(file_info.absoluteFilePath());

			if (deprecated_file.remove())
			{
				qDebug() << "Successfully deleted deprecated file: " << deprecated_file;
			}
			else
			{
				qDebug() << "Error occured while deleting deprecated file... Check your privilege!";
				qDebug() << "Error message: " << deprecated_file.errorString();
				qDebug() << "Offending file: " << deprecated_file;
				// TODO: Handle error?
			}
		}
	}
}

void RPCS3UpdaterQt::ShowDownloadProgress(const QString& message)
{
	ui.progressBar->setValue(0);
	ui.progressBar->setEnabled(true);

	ui.progressLabel->setText(message + tr(" Please wait..."));

	// Handle new progress
	connect(network_reply, &QNetworkReply::downloadProgress, [this](qint64 bytesReceived, qint64 bytesTotal)
	{
		ui.progressBar->setMaximum(bytesTotal);
		ui.progressBar->setValue(bytesReceived);
	});

	// Clean Up
	connect(network_reply, &QNetworkReply::finished, [this]()
	{
		ui.progressBar->setEnabled(false);

		if (network_reply && network_reply->error() == QNetworkReply::NoError)
		{
			QApplication::beep();
		}
	});
}

void RPCS3UpdaterQt::OnErrorOccured(QProcess::ProcessError error)
{
	QString error_message;
	switch (error)
	{
	case QProcess::FailedToStart:
		error_message = tr("Failed To Start");
		break;
	case QProcess::Crashed:
		error_message = tr("Crashed");
		break;
	case QProcess::Timedout:
		error_message = tr("Timed Out");
		break;
	case QProcess::ReadError:
		error_message = tr("Read Error");
		break;
	case QProcess::WriteError:
		error_message = tr("Write Error");
		break;
	case QProcess::UnknownError:
		error_message = tr("Unknown Error");
		break;
	default:
		error_message = tr("Unknown Error: %0").arg(error);
		break;
	}
	QMessageBox::critical(nullptr, tr("Error!"), error_message);
}
