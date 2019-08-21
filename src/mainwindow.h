#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork>
#include <vector>
#include <time.h>
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    quint16 msg_port = 5400;
    quint16 ctrl_port = 5401;
    quint16 param_port = 5402;
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool eventFilter(QObject* watched, QEvent* event);
private:
    Ui::MainWindow *ui;

    QTcpSocket *socket_msg;
    QTcpSocket *socket_ctrl;

    QHash<int, QString> hash_msg;
    QHash<int, QString> hash_ctrl;

    int current_slot, starting_slot, ending_slot;
    int current_ctrl;

    QString received_msg_data;
    QString received_ctrl_data;

    int thrust_percent = 0;
    int roll_percent = 0;
    int pitch_percent = 0;
    int yaw_percent = 0;

    //std::vector<int> pwmSignals;
    int pwmSignals[4] = {0};

    float data1[3] = {0};
    float data2[3] = {0};
    float accel[3] = {0};
    float gyro[3] = {0};
    float mag[3] = {0};
    float mputemp = 0;
    float bmptemp = 0;
    float press = 0;

    clock_t start, end;
    double cpu_time_used;

    void DisplayData();

    inline void handlethegraphs();


signals:
    void dataSet();
    void controlChanged();

public slots:
    void connectToHostMsg();
    void connectedMsg();
    void disconnectedMsg();
    void setDatatoGetMsg();
    void getMsg();
    //bool writeData(QByteArray data);

    void connectToHostCtrl();
    void connectedCtrl();
    void disconnectedCtrl();
    void sendCtrl();
    void getCtrl();
    void zeroDown();

    void _on_moved_thrust_slider(int var);
    void _on_moved_roll_slider(int var);
    void _on_moved_pitch_slider(int var);
    void _on_moved_yaw_slider(int var);


};

#endif // MAINWINDOW_H
