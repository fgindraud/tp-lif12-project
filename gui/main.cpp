#include "main.h"

ConfigWidget::ConfigWidget (ExecuteAndProcessOutput * executorHandle) :
	executor (executorHandle) {
	QCommonStyle style;

	// Create Gui
	setTitle ("Configuration");

	mainLayout = new QVBoxLayout;
	setLayout (mainLayout);

	// Program config
	programConfig = new QHBoxLayout;
	mainLayout->addLayout (programConfig);

	programName = new QLineEdit;
	programName->setPlaceholderText ("Simulator program & arguments");
	programConfig->addWidget (programName, 1);

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
	QObject::connect (programStart, SIGNAL (clicked ()),
		this, SLOT (playClicked ()));
	QObject::connect (programPause, SIGNAL (clicked ()),
		this, SLOT (pauseClicked ()));
	QObject::connect (programStop, SIGNAL (clicked ()),
		this, SLOT (stopClicked ()));
}

void ConfigWidget::setState (SimulatorState state) {
	mState = state;
	bool isStopped = state == Stopped;

	programName->setEnabled (isStopped);
	programStart->setEnabled (state != Running);
	programPause->setEnabled (not isStopped);
	programStop->setEnabled (not isStopped);
	mapName->setEnabled (isStopped);
	openFromFile->setEnabled (isStopped);
	cellSize->setEnabled (isStopped);
}

void ConfigWidget::openFile (void) {
	QString file = QFileDialog::getOpenFileName (
		this, "Open image", QDir::currentPath (),
		"Images (*.png *.jpg *.xpm)");
	if (file != QString ())
		mapName->setText (file);
}

void ConfigWidget::playClicked (void) {
	if (mState == Stopped) {
		// Try to launch process
		QString error = executor->start (
			programName->text (),
			mapName->text (), cellSize->value ()
		);
		if (error == QString ())
			setState (Running);
		else
			QMessageBox::critical (this, "Unable to launch simulator", error);
	} else if (mState == Paused) {
		executor->resume ();
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
	if (mState == Running || mState == Paused) {
		executor->stop ();
		setState (Stopped);
	}
}

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

