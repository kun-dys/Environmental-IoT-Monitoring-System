#pragma once

#include <QtWidgets/QWidget>
#include "ui_mqtt.h"

#include <QtMqtt/qmqttclient.h>
#include <QDebug>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

class mqtt : public QWidget
{
    Q_OBJECT

public:
    mqtt(QWidget *parent = nullptr);
    ~mqtt();
    void onConnected();
    void onDisconnected();
    void onMessageReceived(const QByteArray &message, const QMqttTopicName &topic);
    void initChart();
    void updateChart();
private slots:
    void connectMQTT();
    void subscribeTopic();
    void publishStart();
    void publishStop();
private:
    Ui::mqttClass ui;
    QMqttClient* m_client;
    bool m_isConnecting = false;
    quint8 m_qos = 0;
    bool m_isSubscribe = false;
    bool retain = false;
    QChart* tempChart;
    QChart* humiChart;
    QLineSeries* tempSeries;
    QLineSeries* humiSeries;
    QTimer* timer;
    double xValue = 0;
    bool is_start = false;
};
