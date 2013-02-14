#ifndef MAIN_H
#define MAIN_H

#include <QtGui>
#include <QtCore>

#include "simulator.h"

/*
 * Input and simulation control widget
 */
class ConfigWidget : public QGroupBox {
	Q_OBJECT

	public:
		enum SimulatorState {
			Stopped, Initializing, Paused, Running
		};

		ConfigWidget (ExecuteAndProcessOutput * executorHandle);

	signals:
		void setScaling (bool enabled);

	private slots:
		void setState (SimulatorState state);

		void scalingChecked (int state);
		
		void openFile (void);
		void initClicked (void);
		void playClicked (void);
		void pauseClicked (void);
		void stopClicked (void);

		void onError (QString errorText);
		void onInitSuccess (void);
		void onConnectionEnded (void);

	private:
		SimulatorState mState;
		ExecuteAndProcessOutput * executor;

		QVBoxLayout * mainLayout;

		QHBoxLayout * programConfig;
		QLineEdit * programAddress;
		QSpinBox * programPort;
		QPushButton * programInit;
		QPushButton * programStart;
		QPushButton * programPause;
		QPushButton * programStop;

		QHBoxLayout * wireworldMapConfig;
		QLineEdit * mapName;
		QPushButton * openFromFile;
		QSpinBox * cellSize;
		QCheckBox * enableScaling;
};

/*
 * Auto scaled image viewer
 */
class WireWorldDrawZone : public QLabel {
	Q_OBJECT

	public:
		WireWorldDrawZone (ExecuteAndProcessOutput * executor) : scalingEnabled (true) {
			setAlignment (Qt::AlignCenter);
			setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);

			QObject::connect (executor, SIGNAL (redraw (QImage)),
					this, SLOT (updateWireworld (QImage)));
		}
		~WireWorldDrawZone () {}

	public slots:
		void updateWireworld (QImage image) {
			// Avoid absurd resizes
			setMinimumSize (image.size ());

			// Bufferise image
			buffer = QPixmap::fromImage (image);
			updateScreen ();
		}

		void setScalingStatus (bool enabled) {
			scalingEnabled = enabled;
			updateScreen ();
		}

	private:
		void updateScreen (void) {
			if (buffer.isNull ())
				setPixmap (QPixmap ());
			else if (scalingEnabled)
				setPixmap (buffer.scaled (size (), Qt::KeepAspectRatio));
			else
				setPixmap (buffer);
		}

		void resizeEvent (QResizeEvent * event) {
			(void) event;
			updateScreen ();
		}

		QPixmap buffer;
		bool scalingEnabled;
};

#endif

