#ifndef SIM_H
#define SIM_H

#include <QHostAddress>
#include <QTcpSocket>
#include <QImage>
#include <QPixmap>
#include <QtCore>

#include "protocol.h"

/*
 * Hold the wireworld cell map, and allow translation from/to image
 */
class WireWorldMap {
	public:
		WireWorldMap ();
		~WireWorldMap ();

		/* Size of internal map
		 */
		QSize getSize (void) const;
	
		/* Get map in raw format.
		 * getRawMap returns a newly allocated buffer
		 */
		wireworld_message_t * getRawMap (void) const;
		quint32 getRawMapSize (void) const;

		/* Update rectangle with raw format
		 */
		void updateMap (QPoint topLeft, QPoint bottomRight, wireworld_message_t * data);

		/* Generates image from cell map.
		 * Or load from image.
		 */	
		bool fromImage (const QImage & image, int cellSize = 1);
		const QImage & toImage (void) const;

	private:
		/* Resets internalMap image.
		 */
		bool resetImage (QSize size);

		QImage internalMap;
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
		void redraw (const QImage & image);

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

