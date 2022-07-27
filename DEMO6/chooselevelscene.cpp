#include "chooselevelscene.h"
#include <QPainter>
#include <QMenuBar>
#include "mypushbotton.h"
#include <QDebug>
#include <QLabel>
#include <QSoundEffect>

ChooseLevelScene::ChooseLevelScene(QWidget *parent)
    : QMainWindow{parent}
{
    this->setFixedSize(320, 588);
    this->setWindowIcon(QPixmap(":/res/Coin0001.png"));
    this->setWindowTitle("选择关卡场景");

    QMenuBar *bar =  menuBar();
    setMenuBar(bar);

    QMenu *startMenu = bar->addMenu("开始");
    QAction * quitAction = startMenu->addAction("退出");
    connect(quitAction, &QAction::triggered, [=](){
        this->close();
    });

    QSoundEffect *startSound = new QSoundEffect;
    startSound->setSource(QUrl::fromLocalFile(":/res/BackButtonSound.wav"));

    MyPushBotton *backBtn = new MyPushBotton(":/res/BackButton.png", ":/res/BackButtonSelected.png");
    backBtn->setParent(this);
    backBtn->move(this->width() - backBtn->width(), this->height() - backBtn->height());
    connect(backBtn, &MyPushBotton::clicked, [=](){
        startSound->play();
        qDebug() << "点击了返回";
        emit this->chooseSceneBack();
    });

    for (int i = 0; i < 20; i++) {
        MyPushBotton *menuBtn = new MyPushBotton(":/res/LevelIcon.png");
        menuBtn->setParent(this);
        menuBtn->move(25 + i%4 * 70, 130 + i/4 * 70);

        connect(menuBtn, &MyPushBotton::clicked, [=](){
            startSound->play();
            QString str = QString("你选择的是第 %1 关").arg( i + 1 );
            qDebug() << str;

            this->hide();
            play = new PlayScene(i + 1);
            play->setGeometry(this->geometry());
            play->show();

            connect(play, &PlayScene::chooseSceneBack, [=](){
                this->setGeometry(play->geometry());
                this->show();
                delete play;
                play = nullptr;
            });
        });

        QLabel *label = new QLabel;
        label->setParent(this);
        label->setFixedSize(menuBtn->width(), menuBtn->height());
        label->setText(QString::number(i+1));
        label->move(25 + i%4 * 70, 130 + i/4 *70);
        label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

        // 设置鼠标穿透
        label->setAttribute(Qt::WA_TransparentForMouseEvents);

    }
}

void ChooseLevelScene::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QPixmap pix;
    pix.load(":/res/OtherSceneBg.png");
    painter.drawPixmap(0, 0, this->width(), this->height(), pix);

    pix.load(":/res/Title.png");
    pix = pix.scaled(pix.width() * 0.5, pix.height() * 0.5);
    painter.drawPixmap(10, 30, pix);
}
