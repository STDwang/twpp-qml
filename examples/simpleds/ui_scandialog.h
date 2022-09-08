/********************************************************************************
** Form generated from reading UI file 'scandialog.ui'
**
** Created by: Qt User Interface Compiler version 5.9.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SCANDIALOG_H
#define UI_SCANDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_ScanDialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *questionLabel;
    QSpacerItem *verticalSpacer;
    QPushButton *scanButton;
    QPushButton *cancelButton;

    void setupUi(QDialog *ScanDialog)
    {
        if (ScanDialog->objectName().isEmpty())
            ScanDialog->setObjectName(QStringLiteral("ScanDialog"));
        ScanDialog->resize(321, 105);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(ScanDialog->sizePolicy().hasHeightForWidth());
        ScanDialog->setSizePolicy(sizePolicy);
        ScanDialog->setModal(true);
        verticalLayout = new QVBoxLayout(ScanDialog);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        questionLabel = new QLabel(ScanDialog);
        questionLabel->setObjectName(QStringLiteral("questionLabel"));

        verticalLayout->addWidget(questionLabel);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        scanButton = new QPushButton(ScanDialog);
        scanButton->setObjectName(QStringLiteral("scanButton"));
        QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(scanButton->sizePolicy().hasHeightForWidth());
        scanButton->setSizePolicy(sizePolicy1);
        QFont font;
        font.setPointSize(16);
        scanButton->setFont(font);

        verticalLayout->addWidget(scanButton);

        cancelButton = new QPushButton(ScanDialog);
        cancelButton->setObjectName(QStringLiteral("cancelButton"));

        verticalLayout->addWidget(cancelButton);


        retranslateUi(ScanDialog);

        QMetaObject::connectSlotsByName(ScanDialog);
    } // setupUi

    void retranslateUi(QDialog *ScanDialog)
    {
        ScanDialog->setWindowTitle(QApplication::translate("ScanDialog", "Dialog", Q_NULLPTR));
        questionLabel->setText(QApplication::translate("ScanDialog", "Do you want to scan something?", Q_NULLPTR));
        scanButton->setText(QApplication::translate("ScanDialog", "Scan!", Q_NULLPTR));
        cancelButton->setText(QApplication::translate("ScanDialog", "Cancel", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class ScanDialog: public Ui_ScanDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SCANDIALOG_H
