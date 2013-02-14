#include "simulator.h"

static QRgb wireworldColors[] = {
	qRgb (0x30, 0x30, 0x30),
	qRgb (0xA0, 0x50, 0x00),
	qRgb (0xFF, 0xFF, 0xFF),
	qRgb (0x00, 0x00, 0xA0)
};

static wireworld_message_t getNearestState (QRgb color) {
	int minDist = 10000;
	int minIndex = -1;
	for (int i = 0; i < 4; ++i) {
		QRgb rgb = wireworldColors[i];
		int dist = qAbs (qRed (rgb) - qRed (color)) +
			qAbs (qGreen (rgb) - qGreen (color)) +
			qAbs (qBlue (rgb) - qBlue (color));
		if (dist < minDist) {
			minDist = dist;
			minIndex = i;
		}
	}
	return minIndex;
}

/* ------ WireWorldMap ------ */
WireWorldMap::WireWorldMap () : internalCellMap (0) {
}

WireWorldMap::~WireWorldMap () {
	if (internalCellMap != 0) {
		delete[] internalCellMap;
		internalCellMap = 0;
	}
}

bool WireWorldMap::setSize (QSize size) {
	imageSize = size;
	if (internalCellMap != 0) {
		delete[] internalCellMap;
		internalCellMap = 0;
	}

	if (size.width () == 0 || size.height () == 0)
		return false;

	// Compute message size of cell map
	messageSize = imageSize.width () * imageSize.height () * C_BIT_SIZE / M_BIT_SIZE + 1;
	internalCellMap = new wireworld_message_t [messageSize];
	return true;
}

QSize WireWorldMap::getSize (void) const { return imageSize; }
wireworld_message_t * WireWorldMap::getCellMapContainer (void) { return internalCellMap; }
quint32 WireWorldMap::getCellMapMessageSize (void) const { return messageSize; }

bool WireWorldMap::fromImage (QImage & image, int cellSize) {
	// Check size is valid
	if (not setSize (QSize (image.width () / cellSize, image.height () / cellSize)))
		return false;

	// Extract map from image by matching colors
	quint32 messageIndex = 0;
	int bitIndex = 0;
	
	internalCellMap[0] = 0;
	for (int i = 0; i < imageSize.height (); ++i) {
		const QRgb * lineColors = (const QRgb *) image.constScanLine (i);
		for (int j = 0; j < imageSize.width (); ++j) {
			// Set value
			internalCellMap[messageIndex] |=
				getNearestState (lineColors[j]) <<
				(C_BIT_SIZE * bitIndex);

			// Update counters, and init next word
			bitIndex++;
			if (bitIndex > 15) {
				messageIndex++;
				internalCellMap[messageIndex] = 0;
				bitIndex = 0;
			}
		}
	}

	return true;
}

QImage WireWorldMap::toImage (void) const {
	if (internalCellMap == 0)
		return QImage ();

	// Create new image
	QImage newImage (imageSize, QImage::Format_RGB32);

	// Fill it
	quint32 messageIndex = 0;
	int bitIndex = 0;
	for (int i = 0; i < imageSize.height (); ++i) {
		QRgb * lineColors = (QRgb *) newImage.scanLine (i);
		for (int j = 0; j < imageSize.width (); ++j) {
			// Set value
			wireworld_message_t cellValue = C_BIT_MASK & 
				(internalCellMap[messageIndex] >> (C_BIT_SIZE * bitIndex));
			lineColors[j] = wireworldColors[cellValue];
			// Update counters
			bitIndex++;
			if (bitIndex > 15) {
				messageIndex++;
				bitIndex = 0;
			}
		}
	}

	return newImage;
}

/* ------ ExecuteAndProcessOutput ------ */
ExecuteAndProcessOutput::ExecuteAndProcessOutput () {
	QObject::connect (&mSocket, SIGNAL (error (QAbstractSocket::SocketError)),
			this, SLOT (onSocketError ()));
	QObject::connect (&mSocket, SIGNAL (connected ()),
			this, SLOT (hasConnected ()));
	QObject::connect (&mSocket, SIGNAL (readyRead ()),
			this, SLOT (canReadData ()));
	QObject::connect (&mSocket, SIGNAL (disconnected ()),
			this, SLOT (onSocketDisconnected ()));
}

void ExecuteAndProcessOutput::init (
		QString program, int port,
		QString mapFile, int cellSize) {
	// Load from file
	QImage image (mapFile);

	// Convert to suitable format
	if (image.format () != QImage::Format_RGB32)
		image = image.convertToFormat (QImage::Format_RGB32);

	// Check loading
	if (image.isNull ()) {
		QString errorText = QString ("Unable to load file '%1'").arg (mapFile);
		emit errored (errorText);
		return;
	}

	// Load image into buffer
	if (not mCellMap.fromImage (image, cellSize)) {
		emit errored ("Unable to extract a map from image");
		return;
	}

	// Start Tcp
	mSocket.connectToHost (program, port);
}

/* Little messages */
void ExecuteAndProcessOutput::start (void) {
	wireworld_message_t message = R_START;
	writeInternal (&message, 1);
}

void ExecuteAndProcessOutput::pause (void) {
	wireworld_message_t message = R_PAUSE;
	writeInternal (&message, 1);
}

void ExecuteAndProcessOutput::step (void) {
	wireworld_message_t message = R_STEP;
	writeInternal (&message, 1);
}

void ExecuteAndProcessOutput::stop (void) {
	wireworld_message_t message = R_STOP;
	writeInternal (&message, 1);
	mSocket.close ();
}

void ExecuteAndProcessOutput::onSocketError (void) {
	emit errored ("Socket error : " + mSocket.errorString ());
	mSocket.abort ();
}

void ExecuteAndProcessOutput::hasConnected (void) {
	// If connected, send init request
	wireworld_message_t message[3];
	message[0] = R_INIT;
	message[1] = mCellMap.getSize ().width ();
	message[2] = mCellMap.getSize ().height ();
	writeInternal (message, 3);

	// Send map data
	writeInternal (mCellMap.getCellMapContainer (), mCellMap.getCellMapMessageSize ());
	
	emit initialized ();
}

void ExecuteAndProcessOutput::canReadData (void) {
	// Wait for a complete frame
	quint32 frameSize = mCellMap.getCellMapMessageSize () * sizeof (wireworld_message_t);
	
	// Read all pending frames
	while (mSocket.bytesAvailable () >= frameSize) {
		readInternal (mCellMap.getCellMapContainer (), mCellMap.getCellMapMessageSize ());
	}

	// Only show the last one
	emit redraw (mCellMap.toImage ());

	/* We do not control the timing, so we flush all frames we can.
	 * The server should ensure that frames are sent at the right timing.
	 */
}

void ExecuteAndProcessOutput::onSocketDisconnected (void) {
	// Signal gui if connection has been closed
	emit connectionEnded ();
}

/* Internal read/write */
void ExecuteAndProcessOutput::writeInternal (
		const wireworld_message_t * messages, quint32 nbMessages) {
	wireworld_message_t * buffer = new wireworld_message_t[nbMessages];
	
	// Convert endianness
	for (quint32 i = 0; i < nbMessages; ++i)
		buffer[i] = qToBigEndian (messages[i]);

	// Send all of it (qt should not block, it buffers instead)
	const char * it = (const char *) buffer;
	qint64 bytesToSend = nbMessages * sizeof (wireworld_message_t);
	while (bytesToSend > 0) {
		qint64 sent = mSocket.write (it, bytesToSend);

		// On error, abort connection.
		if (sent == -1) {
			emit errored ("Write error : " + mSocket.errorString ());
			mSocket.abort ();
			return;
		}

		// Update counters
		bytesToSend -= sent;
		it += sent;
	}

	delete[] buffer;
}

void ExecuteAndProcessOutput::readInternal (
		wireworld_message_t * messages, quint32 nbMaxMessages) {
	wireworld_message_t * buffer = new wireworld_message_t[nbMaxMessages];
	
	// Send all of it (qt should not block, it buffers instead)
	char * it = (char *) buffer;
	qint64 bytesToRead = nbMaxMessages * sizeof (wireworld_message_t);
	while (bytesToRead > 0) {
		qint64 read = mSocket.read (it, bytesToRead);
		if (read == -1) {
			// On error, abort connection
			emit errored ("Read error : " + mSocket.errorString ());
			mSocket.abort ();
			return;
		} else if (read == 0) {
			// If no more data, terminate connection peacefully
			emit connectionEnded ();
			mSocket.close ();	
		}

		// Update counters
		bytesToRead -= read;
		it += read;
	}

	// Convert endianness
	for (quint32 i = 0; i < nbMaxMessages; ++i)
		messages[i] = qFromBigEndian (buffer[i]);

	delete[] buffer;
}
