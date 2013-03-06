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

QPixmap WireWorldMap::toImage (void) const {
	return QPixmap::fromImage (internalMap);
}

bool WireWorldMap::resetImage (QSize size) {
	if (size.width () == 0 || size.height () == 0)
		return false;

	internalMap = QImage (size, QImage::Format_RGB32);
	return true;
}

/* -------- PixmapBuffer ------- */
PixmapBuffer::PixmapBuffer () {
	QObject::connect (&timer, SIGNAL (timeout ()),
			this, SLOT (timerTicked ()));
}

void PixmapBuffer::reset (int maxCreditAllowed, int interval) {
	pixmapQueue.clear ();
	timer.setInterval (interval);
	maxCredits = maxCreditAllowed;
	isInStepMode = true;

	emit hasCredit (maxCreditAllowed);
}

bool PixmapBuffer::pixmapReady (QPixmap pixmap) {
	// Fail if maxCredits is not respected
	if (pixmapQueue.size () == maxCredits)
		return false;

	// Queue pixmap
	bool wasEmpty = pixmapQueue.isEmpty ();
	pixmapQueue.enqueue (pixmap);

	// If we were in free run mode and stopped because buffer empty, restart timer
	if (wasEmpty && not isInStepMode) {
		outputPixmap ();
		timer.start ();
	}

	return true;
}

void PixmapBuffer::start (void) {
	isInStepMode = false;
	timer.start ();
}

void PixmapBuffer::stop (void) {
	isInStepMode = true;
	timer.stop ();
}

void PixmapBuffer::step (void) {
	if (isInStepMode && not pixmapQueue.empty ())
		outputPixmap ();
}

void PixmapBuffer::timerTicked (void) {
	if (not pixmapQueue.isEmpty ())
		outputPixmap (); // If we have a pixmap, update screen
	else
		timer.stop (); // Otherwise we stop the timer until data arrives
}

void PixmapBuffer::outputPixmap (void) {
	emit canRedraw (pixmapQueue.dequeue ());
	emit hasCredit (1);
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
	QObject::connect (&mPixmapBuffer, SIGNAL (canRedraw (QPixmap)),
			this, SLOT (bufferSaidRedraw (QPixmap)));
	QObject::connect (&mPixmapBuffer, SIGNAL (hasCredit (int)),
			this, SLOT (sendFrameRequest (int)));
}

void ExecuteAndProcessOutput::init (
		QString host, int port,
		QString mapFile, int cellSize,
		int updateRate, int samplingRate) {
	// Load from file
	QImage image (mapFile);

	// Convert to suitable format
	if (image.format () != QImage::Format_RGB32)
		image = image.convertToFormat (QImage::Format_RGB32);

	// Check loading
	if (image.isNull ()) {
		abort (QString ("Unable to load file '%1'").arg (mapFile));
		return;
	}

	// Load image into buffer
	if (not mCellMap.fromImage (image, cellSize)) {
		abort ("Unable to extract a map from image");
		return;
	}

	// Save parameters for later initialization
	mUpdateRate = updateRate;
	mSamplingRate = samplingRate;

	// Start Tcp
	mSocket.connectToHost (host, port);
}

/* Execution control functions */
void ExecuteAndProcessOutput::start (void) {
	mPixmapBuffer.start ();
}

void ExecuteAndProcessOutput::pause (void) {
	mPixmapBuffer.stop ();
}

void ExecuteAndProcessOutput::step (void) {
	mPixmapBuffer.step ();
}

void ExecuteAndProcessOutput::stop (void) {
	mPixmapBuffer.stop ();
	mSocket.close ();
}

void ExecuteAndProcessOutput::onSocketError (void) {
	abort ("Socket error : " + mSocket.errorString ());
}

void ExecuteAndProcessOutput::onSocketDisconnected (void) {
	// Signal gui if connection has been closed
	emit connectionEnded ();
}

void ExecuteAndProcessOutput::bufferSaidRedraw (QPixmap pixmap) {
	// Propagate signal
	emit redraw (pixmap);
}

void ExecuteAndProcessOutput::sendFrameRequest (int nbRequests) {
	wireworld_message_t message = R_FRAME;
	for (int i = 0; i < nbRequests; ++i)
		writeInternal (&message, 1);
}

void ExecuteAndProcessOutput::hasConnected (void) {
	// If connected, send init request
	wireworld_message_t message[4];
	message[0] = R_INIT;
	message[1] = mCellMap.getSize ().width ();
	message[2] = mCellMap.getSize ().height ();
	message[3] = mSamplingRate;
	writeInternal (message, 4);

	// Send map data
	wireworld_message_t * data = mCellMap.getRawMap ();
	if (data == 0) {
		abort ("Alloc error");
		return;
	} else {
		writeInternal (data, mCellMap.getRawMapSize ());
		delete[] data;
	}

	// Correctly initialized, inform gui
	emit initialized ();

	// And force redraw of initial map state.
	emit redraw (mCellMap.toImage ());

	// Then start reception buffer with a buffer of size 5
	mPixmapBuffer.reset (5, mUpdateRate);
}

void ExecuteAndProcessOutput::canReadData (void) {
	// Add timer stuff TODO
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
			abort ("Write error : " + mSocket.errorString ());
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
			abort ("Read error : " + mSocket.errorString ());
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

void ExecuteAndProcessOutput::abort (QString error) {
	emit errored (error);
	mSocket.abort ();
	mPixmapBuffer.stop ();
}

