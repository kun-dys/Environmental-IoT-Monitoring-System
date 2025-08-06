#include "mqtt.h"
#include <QtCore/QDateTime>
#include <QtMqtt/QMqttClient>
#include <QtWidgets/QMessageBox>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonObject>

mqtt::mqtt(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    initChart();

    m_client = new QMqttClient(this);
    connect(m_client, &QMqttClient::connected, this, &mqtt::onConnected);
    connect(m_client, &QMqttClient::disconnected, this, &mqtt::onDisconnected);
    connect(m_client, &QMqttClient::messageReceived, this, &mqtt::onMessageReceived);
}

mqtt::~mqtt()
{
    // 确保客户端断开连接
    if (m_client->state() != QMqttClient::Disconnected)
        m_client->disconnectFromHost();
}


void mqtt::connectMQTT()
{
    if (m_client->state() == QMqttClient::Disconnected)
    {
        // 连接前设置
        QString host = ui.lineEditHost->text();
        QString port_str = ui.lineEditPort->text();
        quint16 port = port_str.toUShort();
        QString username = ui.lineEditUser->text();
        QString password = ui.lineEditPassword->text();

        ui.lineEditHost->setEnabled(false);
        ui.lineEditPort->setEnabled(false);
        ui.lineEditUser->setEnabled(false);
        ui.lineEditPassword->setEnabled(false);

        m_client->setHostname(host);
        m_client->setPort(port);
        m_client->setUsername(username);
        m_client->setPassword(password);

        ui.buttonConnect->setText(tr("Disconnect"));

        // 标记开始连接尝试
        m_isConnecting = true;

        m_client->connectToHost(); // 发起连接
    }
    else
    {
        // 断开连接（用户主动操作）
        m_isConnecting = false;  // 重置标志

        ui.lineEditHost->setEnabled(true);
        ui.lineEditPort->setEnabled(true);
        ui.lineEditUser->setEnabled(true);
        ui.lineEditPassword->setEnabled(true);
        ui.buttonConnect->setText(tr("Connect"));

        m_client->disconnectFromHost();
    }
}

void mqtt::onConnected()
{
    // 连接成功
    m_isConnecting = false;  // 重置标志

    QMessageBox::information(this, tr("MQTT Connection"), tr("Connection successful"));
}

void mqtt::onDisconnected()
{
    // 如果是连接尝试失败（而非用户主动断开）
    if (m_isConnecting) {
        QMessageBox::critical(this, tr("MQTT Connection"), tr("Connection failed"));

        // 恢复UI状态
        ui.lineEditHost->setEnabled(true);
        ui.lineEditPort->setEnabled(true);
        ui.lineEditUser->setEnabled(true);
        ui.lineEditPassword->setEnabled(true);
        ui.buttonConnect->setText(tr("Connect"));
    }

    m_isConnecting = false;  // 重置标志
}

void mqtt::subscribeTopic()
{
    if (!m_isSubscribe)
    {
        ui.lineEditTopic->setEnabled(false);
        auto subscription = m_client->subscribe(ui.lineEditTopic->text(), m_qos);
        if (!subscription) {
            QMessageBox::critical(this, "Error", "Could not subscribe...");
            return;
        }
        m_isSubscribe = true;
        ui.buttonSubscribe->setText(tr("Unsubscribe"));
        // 订阅成功提示
        QMessageBox::information(
            this,
            "Success",
            "Subscribed to topic: " + ui.lineEditTopic->text()
            );
    }
    else
    {
        m_client->unsubscribe(ui.lineEditTopic->text());
        m_isSubscribe = false;
        ui.lineEditTopic->setEnabled(true);
        ui.buttonSubscribe->setText(tr("Subscribe"));
    }
}

void mqtt::onMessageReceived(const QByteArray &message, const QMqttTopicName &topic)
{
    qDebug() << "MQTT Message Received:" << message;
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(message, &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Invalid JSON message received:" << message;
        return;
    }

    QJsonObject obj = doc.object();

    if (!obj.contains("temp") || !obj.contains("humi")) {
        qWarning() << "Missing temp or humi in message:" << message;
        return;
    }

    double tempValue = obj["temp"].toDouble();
    double humiValue = obj["humi"].toDouble();

    if (!is_start) return;

    // 显示接收的内容
    QString displayText = QString("Topic: %1\nTemp: %2 °C\nHumi: %3 %%")
                              .arg(topic.name())
                              .arg(tempValue)
                              .arg(humiValue);
    ui.message->setText(displayText);

    // 添加数据点
    tempSeries->append(xValue, tempValue);
    humiSeries->append(xValue, humiValue);

    // 滚动X轴
    if (xValue > 10) {
        tempChart->axisX()->setRange(xValue - 10, xValue);
        humiChart->axisX()->setRange(xValue - 10, xValue);
    }

    // Y轴范围自动调整
    qreal minTemp = tempValue, maxTemp = tempValue;
    for (const QPointF& point : tempSeries->points()) {
        minTemp = qMin(minTemp, point.y());
        maxTemp = qMax(maxTemp, point.y());
    }
    tempChart->axisY()->setRange(minTemp - 1, maxTemp + 1);

    qreal minHumi = humiValue, maxHumi = humiValue;
    for (const QPointF& point : humiSeries->points()) {
        minHumi = qMin(minHumi, point.y());
        maxHumi = qMax(maxHumi, point.y());
    }
    humiChart->axisY()->setRange(minHumi - 1, maxHumi + 1);

    xValue += 1;
}


void mqtt::publishStart()
{
    QString topic = ui.lineEditTopic->text();
    QString message = "start";

    if (m_client->publish(topic, message.toUtf8(), m_qos, retain) == -1 || !m_isSubscribe) {
        QMessageBox::critical(this, "Error", "Could not publish message");
        return;
    }

    QString displayText = QString("Topic: %1\nstate: %2")
                              .arg(topic)
                              .arg(message);
    ui.message->setText(displayText);

    //模拟开始
    if (!is_start)
    {
        is_start = true;
        tempSeries->clear();
        humiSeries->clear();
        xValue = 0;
        tempChart->axisX()->setRange(0, 10);
        tempChart->axisY()->setRange(0, 10);
        humiChart->axisX()->setRange(0, 10);
        humiChart->axisY()->setRange(0, 10);
    }
}

void mqtt::publishStop()
{
    QString topic = ui.lineEditTopic->text();
    QString message = "stop";

    if (m_client->publish(topic, message.toUtf8(), m_qos, retain) == -1 || !m_isSubscribe) {
        QMessageBox::critical(this, "Error", "Could not publish message");
        return;
    }

    QString displayText = QString("Topic: %1\nstate: %2")
                              .arg(topic)
                              .arg(message);
    ui.message->setText(displayText);

    //模拟结束
    is_start = false;
}

void mqtt::initChart()
{
    // 创建图表
    tempChart = new QChart();
    tempChart->setTitle(u8"动态实时温度");
    humiChart = new QChart();
    humiChart->setTitle(u8"动态实时湿度");

    // 创建空的数据系列（初始化时不添加任何点）
    tempSeries = new QLineSeries();
    humiSeries = new QLineSeries();

    // 将空系列添加到图表
    tempChart->addSeries(tempSeries);
    humiChart->addSeries(humiSeries);

    // 设置坐标轴
    QValueAxis* axisX_temp = new QValueAxis();
    axisX_temp->setTitleText(u8"时间(秒)");
    axisX_temp->setRange(0, 10);  // 初始X轴范围

    QValueAxis* axisX_humi = new QValueAxis();
    axisX_humi->setTitleText(u8"时间(秒)");
    axisX_humi->setRange(0, 10);  // 初始X轴范围

    QValueAxis* axisY_temp = new QValueAxis();
    axisY_temp->setTitleText(u8"数值(℃)");
    axisY_temp->setRange(0, 100);  // 初始Y轴范围

    QValueAxis* axisY_humi = new QValueAxis();
    axisY_humi->setTitleText(u8"数值(RH)");
    axisY_humi->setRange(0, 100);  // 初始Y轴范围

    tempChart->setAxisX(axisX_temp, tempSeries);
    tempChart->setAxisY(axisY_temp, tempSeries);
    humiChart->setAxisX(axisX_humi, humiSeries);
    humiChart->setAxisY(axisY_humi, humiSeries);

    tempChart->legend()->setVisible(false);
    humiChart->legend()->setVisible(false);

    // 设置图表视图
    ui.graphicsView_temp->setChart(tempChart);
    ui.graphicsView_temp->setRenderHint(QPainter::Antialiasing);
    ui.graphicsView_humi->setChart(humiChart);
    ui.graphicsView_humi->setRenderHint(QPainter::Antialiasing);
}
