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
WireWorldMap::WireWorldMap () {}
WireWorldMap::~WireWorldMap () {}

QSize WireWorldMap::getSize (void) const { return internalMap.size (); }

quint32 WireWorldMap::getRawMapSize (void) const {
	return internalMap.width () * internalMap.height () * C_BIT_SIZE / M_BIT_SIZE + 1;
}

wireworld_message_t * WireWorldMap::getRawMap (void) const {
	wireworld_message_t * rawMap = new wireworld_message_t [getRawMapSize ()];
	if (rawMap != 0) {
		// Extract map from image by matching colors with the palette
		
		// Iterators over bit-packed structure
		quint32 messageIndex = 0;
		int bitIndex = 0;

		rawMap[0] = 0;
		for (int i = 0; i < internalMap.height (); ++i) {
			const QRgb * lineColors = (const QRgb *) internalMap.constScanLine (i);
			for (int j = 0; j < internalMap.width (); ++j) {
				// Set value
				rawMap[messageIndex] |=
					getNearestState (lineColors[j]) <<
					(C_BIT_SIZE * bitIndex);

				// Update iterators, and init next word if needed as we use or-ing
				bitIndex++;
				if (bitIndex > 15) {
					messageIndex++;
					rawMap[messageIndex] = 0;
					bitIndex = 0;
				}
			}
		}
	}
	return rawMap;
}

void WireWorldMap::updateMap (QPoint topLeft, QPoint bottomRight, wireworld_message_t * data) {
	// Iterators in bit-packed structure
	quint32 messageIndex = 0;
	int bitIndex = 0;

	// Update only rectangle
	for (int i = topLeft.y (); i < bottomRight.y (); ++i) {
		QRgb * lineColors = (QRgb *) internalMap.scanLine (i);
		for (int j = topLeft.x (); j < bottomRight.x (); ++j) {
			// Set value
			wireworld_message_t cellValue = C_BIT_MASK & 
				(data[messageIndex] >> (C_BIT_SIZE * bitIndex));
			lineColors[j] = wireworldColors[cellValue];
			
			// Update counters
			bitIndex++;
			if (bitIndex > 15) {
				messageIndex++;
				bitIndex = 0;
			}
		}
	}
}

bool WireWorldMap::fromImage (const QImage & image, int cellSize) {
	// Check size is valid
	if (not resetImage (QSize (image.width () / cellSize, image.height () / cellSize)))
		return false;

	// Fill image with the sampled content of the source image (sampling factor : cellSize)
	// Also format colors to the 4 color used
	for (int i = 0; i < internalMap.height (); ++i) {
		QRgb * toLineColors = (QRgb *) internalMap.scanLine (i);
		const QRgb * fromLineColors = (const QRgb *) image.constScanLine (i * cellSize);
		for (int j = 0; j < internalMap.width (); ++j) {
			toLineColors[j] = wireworldColors[getNearestState (fromLineColors[j * cellSize])];
		}
	}

	return true;
}

const QImage & WireWorldMap::toImage (void) const {
	return internalMap;
}

bool WireWorldMap::resetImage (QSize size) {
	if (size.width () == 0 || size.height () == 0)
		return false;

	internalMap = QImage (size, QImage::Format_RGB32);
	return true;
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
	//wireworld_message_t message = R_START;
	//writeInternal (&message, 1);
}

void ExecuteAndProcessOutput::pause (void) {
	//wireworld_message_t message = R_PAUSE;
	//writeInternal (&message, 1);
}

void ExecuteAndProcessOutput::step (void) {
	//wireworld_message_t message = R_STEP;
	//writeInternal (&message, 1);
}

void ExecuteAndProcessOutput::stop (void) {
	//wireworld_message_t message = R_STOP;
	//writeInternal (&message, 1);
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
	wireworld_message_t * data = mCellMap.getRawMap ();
	if (data == 0) {
		emit errored ("Alloc error");
		mSocket.abort ();
	} else {
		writeInternal (data, mCellMap.getRawMapSize ());
		delete[] data;
	}

	// Correctly initialized, inform gui
	emit initialized ();

	// And force redraw of initial map state.
	emit redraw (mCellMap.toImage ());
}

void ExecuteAndProcessOutput::canReadData (void) {
	// Add timer stuff TODO
	
	// Wait for a complete frame
	quint32 frameSize = mCellMap.getRawMapSize () * sizeof (wireworld_message_t);

	// Read all pending frames
	//while (mSocket.bytesAvailable () >= frameSize) {
	//	readInternal (mCellMap.getCellMapContainer (), mCellMap.getCellMapMessageSize ());
	//}

	// Only show the last one
	emit redraw (mCellMap.toImage ());
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
