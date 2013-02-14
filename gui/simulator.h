#ifndef SIM_H
#define SIM_H

#include <QHostAddress>
#include <QTcpSocket>
#include <QtCore>

#include "../protocol.h"

class ExecuteAndProcessOutput : public QObject {
	Q_OBJECT

	public:
		ExecuteAndProcessOutput () {}
	
		QString init (QString program, int port, QString mapFile, int cellSize);

		void start (void);
		void pause (void);
		void step (void);
		void stop (void);

	signals:
		void redraw (QPixmap & pixmap);

	private:
		QTcpSocket mSocket;
		QImage blah
};

#endif

