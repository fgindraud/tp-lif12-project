#include "main.h"

#include <QAbstractSocket>

/* ------ ConfigWidget ------ */
ConfigWidget::ConfigWidget (ExecuteAndProcessOutput * executorHandle) :
	executor (executorHandle)
{
	QCommonStyle style;

	// Create Gui
	setTitle ("Configuration");

	mainLayout = new QVBoxLayout;
	setLayout (mainLayout);

	// Program config
	programConfig = new QHBoxLayout;
	mainLayout->addLayout (programConfig);

	programAddress = new QLineEdit;
	programAddress->setPlaceholderText ("Simulator network address");
	programConfig->addWidget (programAddress, 1);

	programPort = new QSpinBox;
	programPort->setRange (1, (1 << 16) - 1);
	programPort->setValue (8000);
	programPort->setToolTip ("Simulator port");
	programConfig->addWidget (programPort);

	programInit = new QPushButton (style.standardIcon (QStyle::SP_ArrowUp), QString ());
	programInit->setToolTip ("Load data into simulator");
	programConfig->addWidget (programInit);

	programStart = new QPushButton (style.standardIcon (QStyle::SP_MediaPlay), QString ());
	programConfig->addWidget (programStart);

	programPause = new QPushButton (style.standardIcon (QStyle::SP_MediaPause), QString ());
	programConfig->addWidget (programPause);

	programStop = new QPushButton (style.standardIcon (QStyle::SP_MediaStop), QString ());
	programConfig->addWidget (programStop);

	// Map file config
	wireworldMapConfig = new QHBoxLayout;
	mainLayout->addLayout (wireworldMapConfig);

	mapName = new QLineEdit;
	mapName->setPlaceholderText ("Map file name");
	wireworldMapConfig->addWidget (mapName, 1);

	openFromFile = new QPushButton (style.standardIcon (QStyle::SP_DirIcon), QString ());
	openFromFile->setToolTip ("Open...");
	wireworldMapConfig->addWidget (openFromFile);

	cellSize = new QSpinBox;
	cellSize->setRange (1, 300);
	cellSize->setValue (1);
	cellSize->setToolTip ("Size of cells in the image");
	wireworldMapConfig->addWidget (cellSize);

	setState (Stopped);

	// Signals
	QObject::connect (openFromFile, SIGNAL (clicked ()),
			this, SLOT (openFile ()));

	QObject::connect (programInit, SIGNAL (clicked ()),
			this, SLOT (initClicked ()));
	QObject::connect (programStart, SIGNAL (clicked ()),
			this, SLOT (playClicked ()));
	QObject::connect (programPause, SIGNAL (clicked ()),
			this, SLOT (pauseClicked ()));
	QObject::connect (programStop, SIGNAL (clicked ()),
			this, SLOT (stopClicked ()));

	QObject::connect (executor, SIGNAL (errored (QString)),
			this, SLOT (onError (QString)));
}

void ConfigWidget::setState (SimulatorState state) {
	mState = state;
	bool enableSettings = state == Stopped;

	programAddress->setEnabled (enableSettings);
	programPort->setEnabled (enableSettings);
	programInit->setEnabled (enableSettings);
	programStart->setEnabled (state == Paused);
	programPause->setEnabled (state == Running || state == Paused);
	programStop->setEnabled (state == Running || state == Paused);
	mapName->setEnabled (enableSettings);
	openFromFile->setEnabled (enableSettings);
	cellSize->setEnabled (enableSettings);
}

void ConfigWidget::openFile (void) {
	QString file = QFileDialog::getOpenFileName (
			this, "Open image", QDir::currentPath (),
			"Images (*.png *.jpg *.xpm)");
	if (file != QString ())
		mapName->setText (file);
}

void ConfigWidget::initClicked (void) {
	if (mState == Stopped) {
		setState (Initializing);
		executor->init (
				programAddress->text (), programPort->value (),
				mapName->text (), cellSize->value ()
				);
	}
}

void ConfigWidget::playClicked (void) {
	if (mState == Paused) {
		executor->start ();
		setState (Running);
	}
}

void ConfigWidget::pauseClicked (void) {
	if (mState == Running) {
		executor->pause ();
		setState (Paused);
	} else if (mState == Paused) {
		executor->step ();
	}
}

void ConfigWidget::stopClicked (void) {
	if (mState != Stopped) {
		executor->stop ();
		setState (Stopped);
	}
}

void ConfigWidget::onError (QString errorText) {
	QMessageBox::critical (this, "Simuator error - aborting simulation", errorText);
	setState (Stopped);
}

void ConfigWidget::onInitSuccess (void) {
	setState (Paused);
}

/* ------ main ------ */
int main (int argc, char * argv[]) {
	QApplication app (argc, argv);

	ExecuteAndProcessOutput * executor = new ExecuteAndProcessOutput;

	QWidget * window = new QWidget;
	window->setWindowTitle ("Wireworld simulator - gui");

	QVBoxLayout * mainLayout = new QVBoxLayout;
	window->setLayout (mainLayout);

	ConfigWidget * configWidget = new ConfigWidget (executor);
	mainLayout->addWidget (configWidget);

	WireWorldDrawZone * wireworldState = new WireWorldDrawZone (executor);
	mainLayout->addWidget (wireworldState, 1);

	window->show ();
	return app.exec ();
}

