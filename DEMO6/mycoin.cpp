#include "mycoin.h"
#include <QDebug>

mycoin::mycoin(QString btnImg)
{
    QPixmap pix;
    if (!pix.load(btnImg)) {
        QString str = QString("图片 %1 加载失败").arg(btnImg);
        qDebug() << str;
        return;
    }
    this->setFixedSize(pix.width(), pix.height());
    this->setStyleSheet("QPushButton{border:0px;}");
    this->setIcon(pix);
    this->setIconSize(QSize(pix.width(), pix.height()));

    timer1 = new QTimer(this);
    timer2 = new QTimer(this);

    connect(timer1, &QTimer::timeout, [=](){
        QPixmap pix;
        QString str = QString(":/res/Coin000%1.png").arg(this->min++);
        pix.load(str);
        this->setFixedSize(pix.width(), pix.height());
        this->setStyleSheet("QPushButton{border:0px;}");
        this->setIcon(pix);
        this->setIconSize(QSize(pix.width(), pix.height()));

        if (this->max < this->min) {
            this->min = 1;
            isAnimation = false;
            timer1->stop();
        }
    });

    connect(timer2, &QTimer::timeout, [=](){
        QPixmap pix;
        QString str = QString(":/res/Coin000%1.png").arg(this->max--);
        pix.load(str);
        this->setFixedSize(pix.width(), pix.height());
        this->setStyleSheet("QPushButton{border:0px;}");
        this->setIcon(pix);
        this->setIconSize(QSize(pix.width(), pix.height()));

        if (this->min > this->max) {
            this->max = 8;
            isAnimation = false;
            timer2->stop();
        }
    });

}

void mycoin::changeFlag()
{
    if (this->flag) {
        this->timer1->start(30);
        isAnimation = true;
        this->flag = false;
    } else {
        this->timer2->start(30);
        isAnimation = true;
        this->flag = true;
    }
}

void mycoin::mousePressEvent(QMouseEvent *e)
{
    if (this->isAnimation || this->isWin ) {
        return;
    } else {
        QPushButton::mousePressEvent(e);
    }
}
