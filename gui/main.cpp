#include "main.h"

#include <QAbstractSocket>
#include <QCommonStyle>
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>

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

	programUpdateRate = new QSpinBox;
	programUpdateRate->setRange (0, 10000);
	programUpdateRate->setValue (1000);
	programUpdateRate->setToolTip ("Minimum time between screen updates (msec)");
	programConfig->addWidget (programUpdateRate);

	programSamplingRate = new QSpinBox;
	programSamplingRate->setRange (1, 1000000);
	programSamplingRate->setValue (1);
	programSamplingRate->setToolTip ("Number of steps to compute between each screen updates (sampling rate)");
	programConfig->addWidget (programSamplingRate);

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

	openFromFile = new QPushButton (style.standardIcon (QStyle::SP_DialogOpenButton), QString ());
	openFromFile->setToolTip ("Open...");
	wireworldMapConfig->addWidget (openFromFile);

	cellSize = new QSpinBox;
	cellSize->setRange (1, 300);
	cellSize->setValue (1);
	cellSize->setToolTip ("Size of cells in the loaded image");
	wireworldMapConfig->addWidget (cellSize);

	saveToFile = new QPushButton (style.standardIcon (QStyle::SP_DialogSaveButton), QString ());
	saveToFile->setToolTip ("Save...");
	wireworldMapConfig->addWidget (saveToFile);

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
	QObject::connect (executor, SIGNAL (initialized ()),
			this, SLOT (onInitSuccess ()));
	QObject::connect (executor, SIGNAL (connectionEnded ()),
			this, SLOT (onConnectionEnded ()));
}

void ConfigWidget::setState (SimulatorState state) {
	mState = state;
	bool enableSettings = state == Stopped;

	programAddress->setEnabled (enableSettings);
	programPort->setEnabled (enableSettings);
	programUpdateRate->setEnabled (enableSettings);
	programSamplingRate->setEnabled (enableSettings);
	
	programInit->setEnabled (enableSettings);
	programStart->setEnabled (state == Paused);
	programPause->setEnabled (state == Running || state == Paused);
	programStop->setEnabled (state == Running || state == Paused);

	mapName->setEnabled (enableSettings);
	openFromFile->setEnabled (enableSettings);
	cellSize->setEnabled (enableSettings);
	saveToFile->setEnabled (enableSettings || state == Paused);
}

void ConfigWidget::openFile (void) {
	QString file = QFileDialog::getOpenFileName (
			this, "Open image", QDir::currentPath (),
			"Images (*.png *.jpg *.xpm *.gif)");
	if (file != QString ())
		mapName->setText (file);
}

void ConfigWidget::saveFile (void) {	
}

void ConfigWidget::initClicked (void) {
	if (mState == Stopped) {
		setState (Initializing);
		executor->init ( programAddress->text (), programPort->value (),
				mapName->text (), cellSize->value (),
				programUpdateRate->value (), programSamplingRate->value ());
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
	QMessageBox::critical (this, "Simulator error - aborting simulation", errorText);
	onConnectionEnded ();
}

void ConfigWidget::onInitSuccess (void) { setState (Paused); } 
void ConfigWidget::onConnectionEnded (void) { setState (Stopped); }

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

	WireWorldDrawZone * wireworldDrawer = new WireWorldDrawZone (executor);
	mainLayout->addWidget (wireworldDrawer, 1);

	window->show ();
	return app.exec ();
}

