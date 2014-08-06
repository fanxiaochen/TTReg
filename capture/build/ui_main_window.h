/********************************************************************************
** Form generated from reading UI file 'main_window.ui'
**
** Created: Wed Aug 6 13:41:54 2014
**      by: Qt User Interface Compiler version 4.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAIN_WINDOW_H
#define UI_MAIN_WINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QMainWindow>
#include <QtGui/QStatusBar>
#include <QtGui/QToolBar>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_EvoGeoCaptureClass
{
public:
    QAction *actionStart;
    QAction *actionStop;
    QAction *actionWater;
    QWidget *centralWidget;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *EvoGeoCaptureClass)
    {
        if (EvoGeoCaptureClass->objectName().isEmpty())
            EvoGeoCaptureClass->setObjectName(QString::fromUtf8("EvoGeoCaptureClass"));
        EvoGeoCaptureClass->resize(800, 600);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(EvoGeoCaptureClass->sizePolicy().hasHeightForWidth());
        EvoGeoCaptureClass->setSizePolicy(sizePolicy);
        actionStart = new QAction(EvoGeoCaptureClass);
        actionStart->setObjectName(QString::fromUtf8("actionStart"));
        actionStop = new QAction(EvoGeoCaptureClass);
        actionStop->setObjectName(QString::fromUtf8("actionStop"));
        actionWater = new QAction(EvoGeoCaptureClass);
        actionWater->setObjectName(QString::fromUtf8("actionWater"));
        centralWidget = new QWidget(EvoGeoCaptureClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        sizePolicy.setHeightForWidth(centralWidget->sizePolicy().hasHeightForWidth());
        centralWidget->setSizePolicy(sizePolicy);
        centralWidget->setMinimumSize(QSize(128, 256));
        EvoGeoCaptureClass->setCentralWidget(centralWidget);
        mainToolBar = new QToolBar(EvoGeoCaptureClass);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        EvoGeoCaptureClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(EvoGeoCaptureClass);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        EvoGeoCaptureClass->setStatusBar(statusBar);

        mainToolBar->addAction(actionStart);
        mainToolBar->addAction(actionStop);
        mainToolBar->addAction(actionWater);

        retranslateUi(EvoGeoCaptureClass);

        QMetaObject::connectSlotsByName(EvoGeoCaptureClass);
    } // setupUi

    void retranslateUi(QMainWindow *EvoGeoCaptureClass)
    {
        EvoGeoCaptureClass->setWindowTitle(QApplication::translate("EvoGeoCaptureClass", "EvoGeoCapture", 0, QApplication::UnicodeUTF8));
        actionStart->setText(QApplication::translate("EvoGeoCaptureClass", "Start", 0, QApplication::UnicodeUTF8));
        actionStop->setText(QApplication::translate("EvoGeoCaptureClass", "Stop", 0, QApplication::UnicodeUTF8));
        actionWater->setText(QApplication::translate("EvoGeoCaptureClass", "Water", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class EvoGeoCaptureClass: public Ui_EvoGeoCaptureClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAIN_WINDOW_H
