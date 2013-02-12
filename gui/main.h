#ifndef MAIN_H
#define MAIN_H

#include <QtGui>
#include <QtCore>

#include "simulator.h"

class ConfigWidget : public QGroupBox {
	Q_OBJECT

	public:
		enum SimulatorState {
			Stopped, Paused, Running
		};

		ConfigWidget (ExecuteAndProcessOutput * executorHandle);

	private slots:
		void setState (SimulatorState state);
		
		void openFile (void);
		void playClicked (void);
		void pauseClicked (void);
		void stopClicked (void);

	private:
		SimulatorState mState;
		ExecuteAndProcessOutput * executor;

		QVBoxLayout * mainLayout;

		QHBoxLayout * programConfig;
		QLineEdit * programName;
		QPushButton * programStart;
		QPushButton * programPause;
		QPushButton * programStop;

		QHBoxLayout * wireworldMapConfig;
		QLineEdit * mapName;
		QPushButton * openFromFile;
		QSpinBox * cellSize;
};

class WireWorldDrawZone : public QLabel {
	Q_OBJECT

	public:
		WireWorldDrawZone (ExecuteAndProcessOutput * executor) {
			setAlignment (Qt::AlignCenter);

			QObject::connect (executor, SIGNAL (redraw (QPixmap &)),
					this, SLOT (updateWireworld (QPixmap &)));
		}
		~WireWorldDrawZone () {}

	public slots:
		void updateWireworld (QPixmap & pixmap) {
			setPixmap (pixmap.scaled (size (), Qt::KeepAspectRatio));
		}
};
#endif

