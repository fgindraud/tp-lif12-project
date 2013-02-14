#ifndef SIM_H
#define SIM_H

#include <QHostAddress>
#include <QTcpSocket>
#include <QImage>
#include <QPixmap>
#include <QtCore>

#include "../protocol.h"

/*
 * Translator class for cell array <-> image
 */
class WireWorldMap {
	public:
		WireWorldMap ();
		~WireWorldMap ();

		/* Image size to which will be interpreted raw data
		 * Setting it will reallocate internalCellMap
		 * Returns false if size is not valid, true if it worked
		 */
		bool setSize (QSize size);
		QSize getSize (void) const;

		/*	Internal cell map (native byte order)
		 * Will store cell data.
		 * getCellMapMessageSize : size of internal map is number of wireworld_message_t.
		 */
		wireworld_message_t * getCellMapContainer (void);
		quint32 getCellMapMessageSize (void) const;
	
		/* Generates image from cell map
		 * Or load from image (and set size and data)
		 */	
		bool fromImage (QImage & image, int cellSize = 1);
		QImage toImage (void) const;

	private:
		QSize imageSize;
		wireworld_message_t * internalCellMap;
		quint32 messageSize;
};

/*
 * Network and image generation.
 */
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
		// End of connection
		void connectionEnded (void);

		// Called when a new frame is available
		void redraw (QImage image);

	private slots:
		void onSocketError (void);
		void hasConnected (void);
		void canReadData (void);
		void onSocketDisconnected (void);

	private:
		void writeInternal (const wireworld_message_t * messages, quint32 nbMessages);
		void readInternal (wireworld_message_t * messages, quint32 nbMessages);

		QTcpSocket mSocket;
		WireWorldMap mCellMap;
};

#endif

