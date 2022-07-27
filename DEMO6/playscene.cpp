#include "playscene.h"
#include <QDebug>
#include <QMenu>
#include <QMenuBar>
#include <QPainter>
#include "mypushbotton.h"
#include <QLabel>
#include "mycoin.h"
#include "dataconfig.h"
#include <QPropertyAnimation>
#include <QSoundEffect>

PlayScene::PlayScene(int levelNum)
{
    QString str = QString("第 %1 关").arg(levelNum);
    qDebug() << "进入了" << str;
    this->levelIndex = levelNum;

    this->setFixedSize(320, 588);
    this->setWindowIcon(QPixmap(":/res/Coin0001.png"));
    this->setWindowTitle(str);

    QMenuBar *bar =  menuBar();
    setMenuBar(bar);

    QMenu *startMenu = bar->addMenu("开始");
    QAction * quitAction = startMenu->addAction("退出");
    connect(quitAction, &QAction::triggered, [=](){
        this->close();
    });

    QSoundEffect *backSound = new QSoundEffect();
    backSound->setSource(QUrl::fromLocalFile(":/res/BackButtonSound.wav"));
    QSoundEffect *flipSound = new QSoundEffect();
    backSound->setSource(QUrl::fromLocalFile(":/res/ConFlipSound.wav"));
    QSoundEffect *winSound = new QSoundEffect();
    backSound->setSource(QUrl::fromLocalFile(":/res/LevelWinSound.wav"));

    MyPushBotton *backBtn = new MyPushBotton(":/res/BackButton.png", ":/res/BackButtonSelected.png");
    backBtn->setParent(this);
    backBtn->move(this->width() - backBtn->width(), this->height() - backBtn->height());
    connect(backBtn, &MyPushBotton::clicked, [=](){
        qDebug() << "点击了返回";
        backSound->play();
        emit this->chooseSceneBack();
    });

    QLabel *label = new QLabel;
    label->setParent(this);
    QFont font;
    font.setFamily("华文新魏");
    font.setPointSize(20);
    QString str1 = QString("Level: %1").arg(this->levelIndex);
    label->setFont(font);
    label->setText(str1);
    label->setGeometry(30, this->height() - 50, 120, 50);

    dataConfig config;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            this->gameArrary[i][j] = config.mData[this->levelIndex][i][j];
        }
    }

    QLabel *winLabel = new QLabel;
    QPixmap tmpPix;
    tmpPix.load(":/res/LevelCompletedDialogBg.png");
    winLabel->setGeometry(0, 0, tmpPix.width(), tmpPix.height());
    winLabel->setPixmap(tmpPix);
    winLabel->setParent(this);
    winLabel->move( (this->width() - tmpPix.width()) * 0.5, -tmpPix.height());


    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            QPixmap pix = QPixmap(":/res/BoardNode.png");
            QLabel* label = new QLabel;
            label->setGeometry(0, 0, pix.width(), pix.height());
            label->setPixmap(pix);
            label->setParent(this);
            label->move(57 + i * 50, 200 + j * 50);

            QString str;
            if (this->gameArrary[i][j] == 1) {
                str = ":/res/Coin0001.png";
            } else {
                str = ":/res/Coin0008.png";
            }
            mycoin *coin = new mycoin(str);
            coin->setParent(this);
            coin->move(59 + i * 50, 204 + j * 50);

            coin->posX = i;
            coin->posY = j;
            coin->flag = this->gameArrary[i][j];

            coinBtn[i][j] = coin;

            connect(coin, &mycoin::clicked, [=](){

                flipSound->play();

                for (int i = 0; i < 4; i++)
                    for (int j = 0; j < 4; j++)
                        this->coinBtn[i][j]->isWin = true;

                coin->changeFlag();
                this->gameArrary[i][j] = this->gameArrary[i][j] == 0 ? 1 : 0;

                QTimer::singleShot(100, this, [=](){
                    if (coin->posX + 1 <= 3) {
                        coinBtn[coin->posX + 1][coin->posY]->changeFlag();
                        this->gameArrary[coin->posX + 1][coin->posY] = this->gameArrary[coin->posX + 1][coin->posY] == 0 ? 1 : 0;
                    }
                    if (coin->posX - 1 >= 0) {
                        coinBtn[coin->posX - 1][coin->posY]->changeFlag();
                        this->gameArrary[coin->posX - 1][coin->posY] = this->gameArrary[coin->posX - 1][coin->posY] == 0 ? 1 : 0;
                    }
                    if (coin->posY + 1 <= 3) {
                        coinBtn[coin->posX][coin->posY + 1]->changeFlag();
                        this->gameArrary[coin->posX][coin->posY + 1] = this->gameArrary[coin->posX][coin->posY + 1] == 0 ? 1 : 0;
                    }
                    if (coin->posY - 1 >= 0) {
                        coinBtn[coin->posX][coin->posY - 1]->changeFlag();
                        this->gameArrary[coin->posX][coin->posY - 1] = this->gameArrary[coin->posX][coin->posY - 1] == 0 ? 1 : 0;
                    }

                    for (int i = 0; i < 4; i++)
                        for (int j = 0; j < 4; j++)
                            this->coinBtn[i][j]->isWin = false;

                    this->isWin = true;
                    for (int i = 0; i < 4; i++)
                        for (int j = 0; j < 4; j++)
                            if (coinBtn[i][j]->flag == false){
                                this->isWin = false;
                                break;
                            }
                    if (this->isWin) {
                        winSound->play();
                        qDebug() << "游戏胜利";
                        for (int i = 0; i < 4; i++)
                            for (int j = 0; j < 4; j++){
                                coinBtn[i][j]->isWin = true;
                            }

                        QPropertyAnimation *animation = new QPropertyAnimation(winLabel, "geometry");
                        animation->setDuration(100);
                        animation->setStartValue(QRect(winLabel->x(), winLabel->y(), winLabel->width(), winLabel->height()));
                        animation->setEndValue(QRect(winLabel->x(), winLabel->y() + 144, winLabel->width(), winLabel->height()));
                        animation->setEasingCurve(QEasingCurve::OutBounce);
                        animation->start();
                    }
                });
            });
        }
    }
}

void PlayScene::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    QPixmap pix;
    pix.load(":/res/PlayLevelSceneBg.png");
    painter.drawPixmap(0, 0, this->width(), this->height(), pix);

    pix.load(":/res/Title.png");
    pix = pix.scaled(pix.width() * 0.5, pix.height() * 0.5);
    painter.drawPixmap(10, 30, pix.width(), pix.height(), pix);
}
