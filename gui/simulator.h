#ifndef SIM_H
#define SIM_H

#include <QtGui>
#include <QtCore>

#include "../protocol.h"

class ExecuteAndProcessOutput : public QObject {
	Q_OBJECT

	public:
		ExecuteAndProcessOutput () {}
		
		QString start (QString program, QString mapFile, int cellSize);
		void pause (void);
		void resume (void);
		void step (void);
		void stop (void);

	signals:
		void redraw (QPixmap & pixmap);
};

#endif

