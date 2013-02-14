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
	
	// TODO remove, just for test
	emit redraw (mCellMap.toImage ());
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
}

void ExecuteAndProcessOutput::writeInternal (
		wireworld_message_t * messages, quint32 nbMessages) {
	uchar * buffer = new uchar[nbMessages * sizeof (wireworld_message_t)];
	
	// Convert endianness
	for (quint32 i = 0; i < nbMessages; ++i)
		qToBigEndian (messages[i], &buffer[i * sizeof (wireworld_message_t)]);

	// Send all of it (qt should not block, it buffers instead)
	const char * it = (const char *) buffer;
	qint64 bytesToSend = nbMessages * sizeof (wireworld_message_t);
	while (bytesToSend > 0) {
		qint64 sent = mSocket.write (it, bytesToSend);

		// On error, abort everything
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

