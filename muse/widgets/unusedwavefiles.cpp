#include <stdio.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qstringlist.h>
#include <qtextstream.h>
#include <qmessagebox.h>
#include "unusedwavefiles.h"
#include "ui_unusedwavefiles.h"
#include "globals.h"
#include "app.h"

UnusedWaveFiles::UnusedWaveFiles(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UnusedWaveFiles)
{

    ui->setupUi(this);
    ui->currentProjRadioButton->setChecked(true);
    connect(ui->currentProjRadioButton, SIGNAL(clicked()), SLOT(findWaveFiles()));
    connect(ui->allProjRadioButton, SIGNAL(clicked()), SLOT(findWaveFiles()));
    findWaveFiles();

}

void UnusedWaveFiles::findWaveFiles()
{
    ui->filelistWidget->clear();
    printf("museProject =%s\n", museProject.toLatin1().data());
//    QFileInfo proj(museProject);
//    QString projPath = proj.absolutePath();
    QDir dir(museProject);
    QStringList filter;
    filter.append("*.wav");
    filter.append("*.ogg");
    allWaveFiles= dir.entryList(filter);
    if (!allWaveFiles.count())
        return;
    // get med files
    QStringList medFiles;
    if (ui->currentProjRadioButton->isChecked()) {
        medFiles.append(muse->projectName());
    } else {
        printf("get ALLL *.med files!\n");
        QStringList medFilter("*.med");
        medFiles = dir.entryList(medFilter);
    }
    foreach (QString medFile, medFiles) {
        QString fname = museProject+"/"+ medFile;
        //printf("fopen %s\n", fname.toLatin1().data());
        FILE *fp =fopen(fname.toLatin1().data(),"r");
        QTextStream fileContent(fp);
        while (!fileContent.atEnd()) {
            QString line = fileContent.readLine();
            if (line.contains(".wav") || line.contains(".ogg")) { // optimization
                foreach (QString wav, allWaveFiles) {
                    //printf("checking wav [%s]\n", wav.toLatin1().data() );
                    if (line.contains(wav)) {
                        //int beforeSize=allWaveFiles.size();
                        allWaveFiles.removeOne(wav);
                        //printf("removed one from list, %d %d\n", beforeSize, allWaveFiles.size());
                        break;
                    }
                }
            }
        }
    }

    ui->filelistWidget->addItems(allWaveFiles);
    update();
}

UnusedWaveFiles::~UnusedWaveFiles()
{
    delete ui;
}

void UnusedWaveFiles::accept()
{
    int ret = QMessageBox::question(this,"Move files", "Are you sure you want to move away the unused files?",
                                    QMessageBox::Ok, QMessageBox::Cancel);
    if (ret == QMessageBox::Ok) {
        QDir currDir(museProject);
        currDir.mkdir("unused");

        foreach(QString file, allWaveFiles) {
            QFile::rename(museProject+ "/"+file, museProject + "/unused/" +file);
            // move the wca file if it exists
            QFileInfo wf(museProject + "/" + file);
            if (QFile::exists(museProject + "/" + wf.baseName()+".wca")) {
                QFile::rename(museProject + "/" + wf.baseName()+".wca", museProject + "/unused/" +wf.baseName()+".wca");

            }
        }
    }
    QDialog::accept();
}