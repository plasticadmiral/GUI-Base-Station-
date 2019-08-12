#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <QKeyEvent>
#include <QtNetwork>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>


class BlockWriter
{
public:
    BlockWriter(QIODevice *io)
    {
        buffer.open(QIODevice::WriteOnly);
        this->io = io;
        _stream.setVersion(QDataStream::Qt_4_8);
        _stream.setDevice(&buffer);
        _stream << quint64(0);
    }

    ~BlockWriter()
    {
        _stream.device()->seek(0);
        _stream << static_cast<quint64>(buffer.size());
        io->write(buffer.buffer());
    }

    QDataStream &stream()
    {
        return _stream;
    }

private:
    QBuffer buffer;
    QDataStream _stream;
    QIODevice *io;
};


class BlockReader
{
public:
    BlockReader(QIODevice *io)
    {
        buffer.open(QIODevice::ReadWrite);
        _stream.setVersion(QDataStream::Qt_4_8);
        _stream.setDevice(&buffer);

        qint64 blockSize;
        readMax(io, sizeof(blockSize));
        buffer.seek(0);
        _stream >> blockSize;
        readMax(io, blockSize);
        buffer.seek(sizeof(blockSize));
    }

    QDataStream& stream()
    {
        return _stream;
    }

private:
    void readMax(QIODevice *io, qint64 n)
    {
        while (buffer.size() < n) {
            buffer.write(io->read(n - buffer.size()));
        }
    }
    QBuffer buffer;
    QDataStream _stream;
};



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    starting_slot = 1;
    current_slot = starting_slot;
    ending_slot = 18;


    ui->setupUi(this);

    ui->ESC_throttle_lcd->setPalette(Qt::blue);
    ui->ESC_roll_lcd->setPalette(Qt::blue);
    ui->ESC_pitch_lcd->setPalette(Qt::blue);
    ui->ESC_yaw_lcd->setPalette(Qt::blue);

    ui->ESC_throttle_slider->installEventFilter(this);
    ui->ESC_roll_slider->installEventFilter(this);
    ui->ESC_pitch_slider->installEventFilter(this);
    ui->ESC_yaw_slider->installEventFilter(this);

    ui->dummy_data_msg_status_label->setStyleSheet("background-color:red;");
    ui->dummy_data_ctrl_status_label->setStyleSheet("background-color:red;");

    //ui->dummy_data_data1_x->setText(" ");

    ui->dummy_data_receive_btn->setEnabled(false);

    socket_msg = new QTcpSocket(this);
    socket_ctrl = new QTcpSocket(this);

    connectToHostMsg();
    connectToHostCtrl();

    connect(ui->dummy_data_msg_conn_btn, SIGNAL(clicked()), this, SLOT(connectToHostMsg()));
    connect(ui->dummy_data_ctrl_conn_btn, SIGNAL(clicked()), this, SLOT(connectToHostCtrl()));

    connect(ui->dummy_data_receive_btn, SIGNAL(clicked()), this, SLOT(setDatatoGetMsg()));

    connect(this, SIGNAL(dataSet()), this , SLOT(setDatatoGetMsg()));

    connect(ui->ESC_throttle_slider, SIGNAL(valueChanged(int)), this, SLOT(_on_moved_thrust_slider(int)));
    connect(ui->ESC_roll_slider, SIGNAL(valueChanged(int)), this, SLOT(_on_moved_roll_slider(int)));
    connect(ui->ESC_pitch_slider, SIGNAL(valueChanged(int)), this, SLOT(_on_moved_pitch_slider(int)));
    connect(ui->ESC_yaw_slider, SIGNAL(valueChanged(int)), this, SLOT(_on_moved_yaw_slider(int)));
    connect(ui->ESC_zero_all_btn, SIGNAL(clicked()), this, SLOT(zeroDown()));
    connect(this, SIGNAL(controlChanged()), this, SLOT(sendCtrl()));

    //ADD to hash table
    hash_msg.insert(1, "data1x");
    hash_msg.insert(2, "data1y");
    hash_msg.insert(3, "data1z");

    hash_msg.insert(4, "data2x");
    hash_msg.insert(5, "data2y");
    hash_msg.insert(6, "data2z");

    hash_msg.insert(7, "baro");

    hash_msg.insert(8, "bmptemp");

    hash_msg.insert(9, "accelx");
    hash_msg.insert(10, "accely");
    hash_msg.insert(11, "accelz");

    hash_msg.insert(12, "mputemp");

    hash_msg.insert(13, "gyrox");
    hash_msg.insert(14, "gyroy");
    hash_msg.insert(15, "gyroz");

    hash_msg.insert(16, "magx");
    hash_msg.insert(17, "magy");
    hash_msg.insert(18, "magz");


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connectToHostMsg()
{
    //below sequence is to reinitiate connection if connection is lost.
    socket_msg->deleteLater();
    socket_msg = new QTcpSocket(this);

    socket_msg->connectToHost(QHostAddress("192.168.0.127"), msg_port);
    connect(socket_msg, SIGNAL(connected()), this, SLOT(connectedMsg()));
    connect(socket_msg, SIGNAL(disconnected()), this, SLOT(disconnectedMsg()));


}

void MainWindow::connectedMsg()
{
    ui->dummy_data_msg_status_label->setStyleSheet("background-color:green;");
    ui->dummy_data_receive_btn->setEnabled(true);
    connect(socket_msg, SIGNAL(readyRead()),this, SLOT(getMsg()));

}

void MainWindow::disconnectedMsg()
{
    ui->dummy_data_msg_status_label->setStyleSheet("background-color:red;");
    ui->dummy_data_receive_btn->setEnabled(false);
    disconnect(socket_msg, SIGNAL(readyRead()),this, SLOT(getMsg()));

}

void MainWindow::setDatatoGetMsg()
{

    if(current_slot == ending_slot + 1)
    {
        current_slot = starting_slot;
    }

    qDebug() <<"calling request data slot " << current_slot;


    BlockWriter(socket_msg).stream() << hash_msg.value(current_slot);

    socket_msg->flush();
    current_slot++;

}

void MainWindow::getMsg()
{
    qDebug()<<"handling received data!";
    BlockReader(socket_msg).stream() >> received_msg_data;
    //qDebug() <<"received response, current received data is for slot "<< data <<"and current number is" << current_slot;
    switch (current_slot - 1)
    {
    case 1:
        start = clock();
        qDebug()<<"received data1 x"<<received_msg_data;
        data1[0] = received_msg_data.toFloat();
        break;
    case 2:
        qDebug()<<"received data1 y"<<received_msg_data;
        data1[1] = received_msg_data.toFloat();
        break;
    case 3:
        qDebug()<<"received data1 z"<<received_msg_data;
        data1[2] = received_msg_data.toFloat();
        break;
    case 4:
        qDebug()<<"received data2 x"<<received_msg_data;
        data2[0] = received_msg_data.toFloat();
        break;
    case 5:
        qDebug()<<"received data2 y"<<received_msg_data;
        data2[1] = received_msg_data.toFloat();
        break;
    case 6:
        qDebug()<<"received data2 z"<<received_msg_data;
        data2[2] = received_msg_data.toFloat();
        break;
    case 7:
        qDebug()<<"received pressure data"<<received_msg_data;
        press = received_msg_data.toFloat();
        break;
    case 8:
        qDebug()<<"received bmp temperature data"<<received_msg_data;
        bmptemp = received_msg_data.toFloat();
        break;
    case 9:
        qDebug()<<"received accelx data"<<received_msg_data;
        accel[0] = received_msg_data.toFloat();
        break;
    case 10:
        qDebug()<<"received accely data"<<received_msg_data;
        accel[1] = received_msg_data.toFloat();
        break;
    case 11:
        qDebug()<<"received accelz data"<<received_msg_data;
        accel[2] = received_msg_data.toFloat();
        break;
    case 12:
        qDebug()<<"received mpu temperature data"<<received_msg_data;
        mputemp = received_msg_data.toFloat();
        break;
    case 13:
        qDebug()<<"received gyrox data"<<received_msg_data;
        gyro[0] = received_msg_data.toFloat();
        break;
    case 14:
        qDebug()<<"received gyroy data"<<received_msg_data;
        gyro[1] = received_msg_data.toFloat();
        break;
    case 15:
        qDebug()<<"received gyroz data"<<received_msg_data;
        gyro[2] = received_msg_data.toFloat();
        break;
    case 16:
        qDebug()<<"received magx data"<<received_msg_data;
        mag[0] = received_msg_data.toFloat();
        break;
    case 17:
        qDebug()<<"received magy data"<<received_msg_data;
        mag[1] = received_msg_data.toFloat();
        break;
    case 18:
        qDebug()<<"received magz data"<<received_msg_data;
        mag[2] = received_msg_data.toFloat();
        DisplayData();
        end = clock();
        ui->dummy_data_hz_label->setNum((double)(end-start)/CLOCKS_PER_SEC);
        break;
    default:
        qDebug() << "Invalid response from server to display.. Please check sequence!";

    }

    emit dataSet();
}


void MainWindow::DisplayData()
{
    ui->dummy_data_data1_x->setNum(data1[0]);
    ui->dummy_data_data1_y->setNum(data1[1]);
    ui->dummy_data_data1_z->setNum(data1[2]);
    ui->dummy_data_data2_x->setNum(data2[0]);
    ui->dummy_data_data2_y->setNum(data2[1]);
    ui->dummy_data_data2_z->setNum(data2[2]);

    ui->BMP280_data_pressure_data->setNum(press);
    ui->BMP280_data_temperature_data->setNum(bmptemp);

    ui->MPU9250_data_accelerometer_data_x->setNum(accel[0]);
    ui->MPU9250_data_accelerometer_data_y->setNum(accel[1]);
    ui->MPU9250_data_accelerometer_data_z->setNum(accel[2]);

    ui->MPU9250_data_gyroscope_data_x->setNum(gyro[0]);
    ui->MPU9250_data_gyroscope_data_y->setNum(gyro[1]);
    ui->MPU9250_data_gyroscope_data_z->setNum(gyro[2]);

    ui->MPU9250_data_magnetometer_data_x->setNum(mag[0]);
    ui->MPU9250_data_magnetometer_data_y->setNum(mag[1]);
    ui->MPU9250_data_magnetometer_data_z->setNum(mag[2]);

    ui->MPU9250_data_temperature_data->setNum(mputemp);
}

void MainWindow::connectToHostCtrl()
{
    //below sequence is to reinitiate connection if connection is lost.
    socket_ctrl->deleteLater();
    socket_ctrl = new QTcpSocket(this);

    socket_ctrl->connectToHost(QHostAddress("192.168.0.127"), ctrl_port);
    connect(socket_ctrl, SIGNAL(connected()), this, SLOT(connectedCtrl()));
    connect(socket_ctrl, SIGNAL(disconnected()), this, SLOT(disconnectedCtrl()));

}

void MainWindow::connectedCtrl()
{
    ui->dummy_data_ctrl_status_label->setStyleSheet("background-color:green;");
    connect(socket_ctrl, SIGNAL(readyRead()),this, SLOT(getCtrl()));

}

void MainWindow::disconnectedCtrl()
{
    ui->dummy_data_ctrl_status_label->setStyleSheet("background-color:red;");
    disconnect(socket_ctrl, SIGNAL(readyRead()),this, SLOT(getCtrl()));

}


void MainWindow::_on_moved_thrust_slider(int var)
{
    thrust_percent = var;
    ui->ESC_throttle_lcd->display(var);
    emit controlChanged();
}

void MainWindow::_on_moved_roll_slider(int var)
{
    roll_percent = var;
    ui->ESC_roll_lcd->display(var);
    emit controlChanged();
}

void MainWindow::_on_moved_pitch_slider(int var)
{
    pitch_percent = var;
    ui->ESC_pitch_lcd->display(var);
    emit controlChanged();
}

void MainWindow::_on_moved_yaw_slider(int var)
{
    yaw_percent = var;
    ui->ESC_yaw_lcd->display(var);
    emit controlChanged();
}


void MainWindow::sendCtrl()
{
    std::string str =
            std::to_string(thrust_percent) + ' ' +
            std::to_string(roll_percent) + ' ' +
            std::to_string(pitch_percent) + ' ' +
            std::to_string(yaw_percent);
    qDebug() << QString::fromStdString(str);
    BlockWriter(socket_ctrl).stream() << QString::fromStdString(str);
}

//optimize this
void MainWindow::getCtrl()
{
    BlockReader(socket_ctrl).stream() >> received_ctrl_data;
    std::cout<<received_ctrl_data.toStdString()<<std::endl;
    int count = 0;
    std::string buffer = "";
    //received_ctrl_data = "1000 200 -50 700";
    for (auto x : received_ctrl_data.toStdString())
    {
        if(x == ' ')
        {
            pwmSignals[count] = stoi(buffer);
            count++;
            buffer = "";
        }
        else
        {
            buffer = buffer + x;
        }
    }
    pwmSignals[count] = stoi(buffer);
    //std::cout <<pwmSignals[0] << " " <<pwmSignals[1] << " " <<pwmSignals[2] << " " <<pwmSignals[3];
    ui->ESC_FL_label->setNum(pwmSignals[0]);
    ui->ESC_FR_label->setNum(pwmSignals[1]);
    ui->ESC_BL_label->setNum(pwmSignals[2]);
    ui->ESC_BR_label->setNum(pwmSignals[3]);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = (QKeyEvent* )event;

        switch (keyEvent->key())
        {
        case 'R':

            ui->ESC_throttle_slider->setSliderPosition(ui->ESC_throttle_slider->sliderPosition()+1);
            break;

        case 'F':
            ui->ESC_throttle_slider->setSliderPosition(ui->ESC_throttle_slider->sliderPosition()-1);
            break;

        case 'W':
            ui->ESC_pitch_slider->setSliderPosition(ui->ESC_pitch_slider->sliderPosition()+1);
            break;

        case 'S':
            ui->ESC_pitch_slider->setSliderPosition(ui->ESC_pitch_slider->sliderPosition()-1);
            break;

        case 'D':
            ui->ESC_roll_slider->setSliderPosition(ui->ESC_roll_slider->sliderPosition()+1);
            break;

        case 'A':
            ui->ESC_roll_slider->setSliderPosition(ui->ESC_roll_slider->sliderPosition()-1);
            break;

        case 'E':
            ui->ESC_yaw_slider->setSliderPosition(ui->ESC_yaw_slider->sliderPosition()+1);
            break;

        case 'Q':
            ui->ESC_yaw_slider->setSliderPosition(ui->ESC_yaw_slider->sliderPosition()-1);
            break;

        }

    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::zeroDown()
{

    ui->ESC_throttle_slider->setSliderPosition(0);
    ui->ESC_roll_slider->setSliderPosition(0);
    ui->ESC_pitch_slider->setSliderPosition(0);
    ui->ESC_yaw_slider->setSliderPosition(0);

}
