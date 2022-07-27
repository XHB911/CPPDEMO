#ifndef MYCOIN_H
#define MYCOIN_H

#include <QPushButton>>
#include <QTimer>

class mycoin : public QPushButton
{
    Q_OBJECT
public:
//    explicit mycoin(QWidget *parent = nullptr);
    int posX;
    int posY;
    bool flag;
    bool isAnimation = false;
    bool isWin = false;
    void mousePressEvent(QMouseEvent *e);

    void changeFlag();
    QTimer *timer1;
    QTimer *timer2;
    int min = 1;
    int max = 8;

    mycoin(QString btnImg);
signals:

};

#endif // MYCOIN_H
