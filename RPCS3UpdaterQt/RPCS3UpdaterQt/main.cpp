#include "RPCS3UpdaterQt.h"
#include <QtWidgets/QApplication>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char *argv[])
{
#ifdef _WIN32
	// use this instead of SetProcessDPIAware if Qt ever fully supports this on windows
	// at the moment it can't display QCombobox frames for example
	// I think there was an issue with gsframe if I recall correctly, so look out for that
	//QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	SetProcessDPIAware();
#else
	qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
#endif

	QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
	QCoreApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);

	QApplication a(argc, argv);
	a.setWindowIcon(QIcon(":/RPCS3UpdaterQt/rpcs3.ico"));
	RPCS3UpdaterQt w;
	w.show();
	return a.exec();
}
