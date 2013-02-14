#include "simulator.h"

static QRgb wireworldColors[] = {
	qRgb (0x30, 0x30, 0x30),
	qRgb (0xA0, 0x50, 0x00),
	qRgb (0xFF, 0xFF, 0xFF),
	qRgb (0x00, 0x00, 0xA0)
};

ExecuteAndProcessOutput::ExecuteAndProcessOutput () {
	QObject::connect (&mSocket, SIGNAL (error (QAbstractSocket::SocketError)),
			this, SLOT (onSocketError ()));
}

void ExecuteAndProcessOutput::init (
		QString program, int port,
		QString mapFile, int cellSize) {
	// Load from file
	loadedImage.load (mapFile);

	// Convert to suitable format
	if (loadedImage.format () != QImage::Format_RGB32)
		loadedImage = loadedImage.convertToFormat (QImage::Format_RGB32);

	// Check loading
	if (loadedImage.isNull ()) {
		QString errorText = QString ("Unable to load file '%1'").arg (mapFile);
		emit errored (errorText);
		return;
	}

	// Start Tcp
	mSocket.connectToHost (program, port);
}

void ExecuteAndProcessOutput::start (void) {
}

void ExecuteAndProcessOutput::pause (void) {
}

void ExecuteAndProcessOutput::step (void) {
}

void ExecuteAndProcessOutput::stop (void) {
}

void ExecuteAndProcessOutput::onSocketError (void) {
	emit errored ("Socket error : " + mSocket.errorString ());
	mSocket.abort ();
}

void ExecuteAndProcessOutput::hasConnected (void) {
}

void ExecuteAndProcessOutput::canSendData (void) {
}

void ExecuteAndProcessOutput::canReadData (void) {
}

