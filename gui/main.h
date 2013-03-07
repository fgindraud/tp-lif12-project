#ifndef MAIN_H
#define MAIN_H

#include <QtCore>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>

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

	private slots:
		void setState (SimulatorState state);

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

		QSpinBox * programSamplingRate;
		QSpinBox * programUpdateRate;

		QPushButton * programInit;
		QPushButton * programStart;
		QPushButton * programPause;
		QPushButton * programStop;

		QHBoxLayout * wireworldMapConfig;
		QLineEdit * mapName;
		QPushButton * openFromFile;
		QSpinBox * cellSize;
};

/*
 * Auto scaled image viewer
 */
class WireWorldDrawZone : public QLabel {
	Q_OBJECT

	public:
		WireWorldDrawZone (ExecuteAndProcessOutput * executor) {
			setAlignment (Qt::AlignCenter);
			setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);

			QObject::connect (executor, SIGNAL (redraw (QPixmap)),
					this, SLOT (updateWireworld (QPixmap)));
		}

	public slots:
		void updateWireworld (QPixmap pixmap) {
			// Avoid absurd resizes
			setMinimumSize (pixmap.size ());

			// Bufferise image
			buffer = pixmap;
			updateScreen ();
		}

	private:
		void updateScreen (void) {
			if (not buffer.isNull ()) {
				QSize scaledSize (
						(width () / buffer.width ()) * buffer.width (),
						(height () / buffer.height ()) * buffer.height ());
				
				// Avoid a call to scaled() if same size
				if (scaledSize != buffer.size ())
					setPixmap (buffer.scaled (scaledSize, Qt::KeepAspectRatio));
				else
					setPixmap (buffer);
			} else {
				setPixmap (buffer);
			}
		}

		void resizeEvent (QResizeEvent * event) {
			(void) event;
			updateScreen ();
		}

		QPixmap buffer;
};

#endif

