#ifndef SCATTER_PLOT_H
#define SCATTER_PLOT_H

#include <QObject>
#include <QtDataVisualization>

using namespace QtDataVisualization;

class scatter_plot : public QObject
{
    Q_OBJECT
public:
    explicit scatter_plot(Q3DScatter *plot = nullptr);

    int pointsToRetain = 0;

private:
    Q3DScatter* m_graph;
signals:

public slots:
};

#endif // SCATTER_PLOT_H
