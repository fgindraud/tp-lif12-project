#ifndef SIM_H
#define SIM_H

#include <QHostAddress>
#include <QTcpSocket>
#include <QImage>
#include <QPixmap>
#include <QtCore>

#include "../protocol.h"

class ExecuteAndProcessOutput : public QObject {
	Q_OBJECT

	public:

		ExecuteAndProcessOutput ();
	
		void init (QString program, int port, QString mapFile, int cellSize);

		void start (void);
		void pause (void);
		void step (void);
		void stop (void);

	signals:
		// Called if initialization succedeed.
		void initialized (void);
		// Called if error happened. In this case the connection will be closed.
		void errored (QString error);

		// Called when a new frame is available
		void redraw (QPixmap & pixmap);

	private slots:
		void onSocketError (void);
		void hasConnected (void);
		void canSendData (void);
		void canReadData (void);

	private:
		QTcpSocket mSocket;
		QImage loadedImage;
};

#endif

