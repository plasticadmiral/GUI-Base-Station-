#ifndef LINE_PLOT_H
#define LINE_PLOT_H

#include <QObject>
#include <QtCharts>

using namespace QtCharts;

class line_plot : public QObject
{
    Q_OBJECT
public:
    explicit line_plot(QObject *parent = nullptr);

signals:

public slots:
};

#endif // LINE_PLOT_H
