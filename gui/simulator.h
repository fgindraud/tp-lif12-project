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

		/* Borders of internal map
		 */
		QRect getRect (void) const;
	
		/* Get map in raw format.
		 * getRawMap returns a newly allocated buffer
		 */
		wireworld_message_t * getRawMap (void) const;
		quint32 getRawMapSize (void) const;

		/* Update rectangle with raw format
		 */
		void updateMap (QPoint topLeft, QPoint bottomRight, wireworld_message_t * data);

		/* Generates a new pixmap from stored map.
		 * Or load initial map from an image.
		 */	
		bool fromImage (const QImage & image, int cellSize = 1);
		QPixmap toImage (void) const;

	private:
		/* Resets internalMap image.
		 */
		bool resetImage (QSize size);

		QImage internalMap;
};


/*
 * 
 */
class PixmapBuffer : public QObject {
	Q_OBJECT

	public:
		PixmapBuffer ();

		void reset (int maxCreditAllowed, int interval);
		bool pixmapReady (QPixmap pixmap);
		void start (void);
		void stop (void);
		void step (void);

	signals:
		void canRedraw (QPixmap pixmap);
		void hasCredit (int credit);

	private slots:
		void timerTicked (void);

	private:
		void outputPixmap (void);

		QQueue< QPixmap > pixmapQueue;
		QTimer timer;

		int maxCredits;
		bool isInStepMode;
};

/*
 * Network / Timing interaction
 */
class ExecuteAndProcessOutput : public QObject {
	Q_OBJECT

	public:

		ExecuteAndProcessOutput ();
	
		void init (QString host, int port,
				QString mapFile, int cellSize,
				int updateRate, int samplingRate);

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
		void redraw (QPixmap pixmap);

	private slots:
		void onSocketError (void);
		void hasConnected (void);
		void canReadData (void);
		void onSocketDisconnected (void);
		void bufferSaidRedraw (QPixmap pixmap);
		void sendFrameRequest (int nbRequests);

	private:
		void writeInternal (const wireworld_message_t * messages, quint32 nbMessages);
		void readInternal (wireworld_message_t * messages, quint32 nbMessages);
		void abort (QString error);

		QTcpSocket mSocket;
		WireWorldMap mCellMap;
		PixmapBuffer mPixmapBuffer;

		// Temporarily store parameters
		int mUpdateRate;
		int mSamplingRate;

		/* Store message decoding step
		 */
		enum DecodingStep {
			WaitingHeader, RectUpdateWaitingPos, RectUpdateWaitingData
		};

		// Step we are in, and size of data needed to go further
		DecodingStep mDecodingStep;
		quint32 mRequestedDataSize;

		// Specific data
		QPoint mPos1, mPos2;
};

#endif

