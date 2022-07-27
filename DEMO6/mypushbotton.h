#ifndef MYPUSHBOTTON_H
#define MYPUSHBOTTON_H

#include <QPushButton>>

class MyPushBotton : public QPushButton
{
    Q_OBJECT
public:

    MyPushBotton(QString normalImg, QString pressImg = "");

    QString norrmalImgPath;
    QString pressImgPath;

    void zoom1();
    void zoom2();

    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
signals:

};

#endif // MYPUSHBOTTON_H
