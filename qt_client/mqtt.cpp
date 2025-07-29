#include "mqtt.h"
#include <QtCore/QDateTime>
#include <QtMqtt/QMqttClient>
#include <QtWidgets/QMessageBox>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QRandomGenerator>

mqtt::mqtt(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    initChart();

    m_client = new QMqttClient(this);
    connect(m_client, &QMqttClient::connected, this, &mqtt::onConnected);
    connect(m_client, &QMqttClient::disconnected, this, &mqtt::onDisconnected);
    connect(m_client, &QMqttClient::messageReceived, this, &mqtt::onMessageReceived);

    // 创建定时器，每秒触发一次更新
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &mqtt::updateChart);
}

mqtt::~mqtt()
{
    // 确保客户端断开连接
    if (m_client->state() != QMqttClient::Disconnected)
        m_client->disconnectFromHost();
}


void mqtt::connectMQTT()
{
    if (m_client->state() == QMqttClient::Disconnected && !m_isConnecting)
    {
        m_isConnecting = true;

        // 连接前设置
        QString host = ui.lineEditHost->text();
        QString port_str = ui.lineEditPort->text();
        quint16 port = port_str.toUShort();
        QString username = ui.lineEditUser->text();
        QString password = ui.lineEditPassword->text();

        if (host.isEmpty() || username.isEmpty() || password.isEmpty()) {
            QMessageBox::warning(this, tr("Input Error"), tr("Input cannot be empty."));
            return;
        }
        if (port == 0) {
            QMessageBox::warning(this, tr("Input Error"), tr("Port must be a valid number."));
            return;
        }

        ui.lineEditHost->setEnabled(false);
        ui.lineEditPort->setEnabled(false);
        ui.lineEditUser->setEnabled(false);
        ui.lineEditPassword->setEnabled(false);

        m_client->setHostname(host);
        m_client->setPort(port);
        m_client->setUsername(username);
        m_client->setPassword(password);

        ui.buttonConnect->setText(tr("Disconnect"));

        m_client->connectToHost(); // 发起连接
    }
    else
    {
        // 断开连接（用户主动操作）
        m_isConnecting = false;  // 重置标志
        m_client->disconnectFromHost();

        ui.lineEditHost->setEnabled(true);
        ui.lineEditPort->setEnabled(true);
        ui.lineEditUser->setEnabled(true);
        ui.lineEditPassword->setEnabled(true);
        ui.buttonConnect->setText(tr("Connect"));
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
    }
    // 恢复UI状态
    ui.lineEditHost->setEnabled(true);
    ui.lineEditPort->setEnabled(true);
    ui.lineEditUser->setEnabled(true);
    ui.lineEditPassword->setEnabled(true);
    ui.buttonConnect->setText(tr("Connect"));
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
    // 格式化消息：主题 + 内容
    QString displayText = QString("Topic: %1\nMessage: %2")
                              .arg(topic.name(), QString::fromUtf8(message));

    // 在label上显示消息
    ui.message->setText(displayText);

    // 或者，如果需要保留历史消息，可以使用append()配合QTextEdit
    // ui->textEditMessageDisplay->append(displayText);
}

void mqtt::publishStart()
{
    QString topic = ui.lineEditTopic->text();
    QString message = "start";

    if (m_client->publish(topic, message.toUtf8(), m_qos, retain) == -1 || !m_isSubscribe) {
        QMessageBox::critical(this, "Error", "Could not publish message");
        return;
    }

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
        timer->start(100);  // 设置1秒的时间间隔
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

    //模拟结束
    is_start = false;
    timer->stop();  // 停止定时器
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

void mqtt::updateChart()
{
    // 基准值
    static qreal tempBase = 25.0;
    static qreal humiBase = 50.0;

    // 在范围内波动
    qreal tempDelta = QRandomGenerator::global()->generateDouble() * 2.0 - 1.0;
    qreal humiDelta = QRandomGenerator::global()->generateDouble() * 3.0 - 1.5;

    // 计算新值并限制范围
    qreal tempValue = qBound(15.0, tempBase + tempDelta, 35.0);
    qreal humiValue = qBound(30.0, humiBase + humiDelta, 70.0);

    // 更新基准值（模拟趋势变化）
    tempBase = tempValue;
    humiBase = humiValue;

    // 添加新数据点
    tempSeries->append(xValue, tempValue);
    humiSeries->append(xValue, humiValue);

    // X轴滚动
    if (xValue > 10) {
        tempChart->axisX()->setRange(xValue - 10, xValue);
        humiChart->axisX()->setRange(xValue - 10, xValue);
    }

    // 计算温度Y轴范围
    qreal minTemp = tempValue, maxTemp = tempValue;
    for (int i = 0; i < tempSeries->count(); ++i) {
        qreal y = tempSeries->at(i).y();
        minTemp = qMin(minTemp, y);
        maxTemp = qMax(maxTemp, y);
    }
    tempChart->axisY()->setRange(minTemp - 1, maxTemp + 1);

    // 计算湿度Y轴范围
    qreal minHumi = humiValue, maxHumi = humiValue;
    for (int i = 0; i < humiSeries->count(); ++i) {
        qreal y = humiSeries->at(i).y();
        minHumi = qMin(minHumi, y);
        maxHumi = qMax(maxHumi, y);
    }
    humiChart->axisY()->setRange(minHumi - 1, maxHumi + 1);

    // 增加时间
    xValue += 0.1;
}
