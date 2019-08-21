#include "scatter_plot.h"
#include<QtDataVisualization>


using namespace QtDataVisualization;


scatter_plot::scatter_plot(Q3DScatter *plot) : m_graph(plot)
{
    m_graph->activeTheme()->setType(Q3DTheme::ThemeStoneMoss);

}
