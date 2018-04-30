#include "RPCS3UpdaterQt.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	RPCS3UpdaterQt w;
	w.show();
	return a.exec();
}
