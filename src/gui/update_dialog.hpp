// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QDialog>
#include <QCoreApplication>
#include <QCloseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDateTime>
#include <QUuid>
#include <QFile>
#include <functional>
class UpdateDialog final : public QDialog {
    const bool it;std::function<void(bool)> busy;QLineEdit *archive{},*bundle{},*commit{};QPushButton *install{},*rollback{},*close{};QPlainTextEdit *log{};QProcess process;QByteArray output;bool completed=true;
    QString text(const char *en,const char *italian)const{return QString::fromUtf8(it?italian:en);}
    void state(bool running) {archive->setEnabled(!running);bundle->setEnabled(!running);commit->setEnabled(!running);install->setEnabled(!running);rollback->setEnabled(!running);close->setEnabled(!running);busy(running);}
    void finish(int code) {
        if(completed)return;completed=true;output+=process.readAllStandardOutput();log->setPlainText(QString::fromUtf8(output));
        auto dir=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/logs";QDir().mkpath(dir);
        QFile file(dir+"/update-"+QDateTime::currentDateTimeUtc().toString("yyyyMMddTHHmmssZ")+"-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".log");
        if(file.open(QIODevice::WriteOnly|QIODevice::NewOnly)){file.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner);file.write(output);}
        log->appendPlainText(text(code==0?"Verified operation completed. Restart the app to use the activated version.":"Operation failed. The previously active version is retained unless activation completed atomically.",code==0?"Operazione verificata completata. Riavvia l’app per usare la versione attivata.":"Operazione non riuscita. La versione precedente resta attiva salvo attivazione atomica già completata."));state(false);
    }
protected:
    void closeEvent(QCloseEvent *event)override {if(process.state()!=QProcess::NotRunning)event->ignore();else QDialog::closeEvent(event);}
    void reject()override {if(process.state()==QProcess::NotRunning)QDialog::reject();}
public:
    UpdateDialog(bool italian,std::function<void(bool)> onBusy,QWidget *parent=nullptr):QDialog(parent),it(italian),busy(std::move(onBusy)) {
        setObjectName("updateDialog");setWindowTitle(text("Verified updates and rollback","Aggiornamenti verificati e ripristino"));resize(700,540);auto *layout=new QVBoxLayout(this);
        auto *note=new QLabel(text("Select the native package for your distribution, its Sigstore provenance bundle and the approved full source commit. GitHub CLI verifies the ORBYX main workflow identity before extraction. No personal GPG key is requested. A legacy source installation is preserved, but never treated as a signed rollback.","Seleziona il pacchetto nativo per la tua distribuzione, il bundle di provenienza Sigstore e il commit sorgente completo approvato. GitHub CLI verifica l’identità del workflow ORBYX su main prima dell’estrazione. Non viene richiesta la chiave GPG personale. L’installazione sorgente precedente è conservata, ma non viene considerata un rollback firmato."));note->setWordWrap(true);layout->addWidget(note);
        auto field=[&](QLineEdit*& edit,const QString& label,bool choose,const QString& filter) {auto *title=new QLabel(label);layout->addWidget(title);edit=new QLineEdit;edit->setAccessibleName(label);title->setBuddy(edit);auto *row=new QHBoxLayout;row->addWidget(edit,1);if(choose){auto *button=new QPushButton(text("Choose…","Seleziona…"));row->addWidget(button);connect(button,&QPushButton::clicked,this,[=]{auto file=QFileDialog::getOpenFileName(this,label,{},filter);if(!file.isEmpty())edit->setText(file);});}layout->addLayout(row);};
        field(archive,text("Native signed package (.tar.gz)","Pacchetto nativo firmato (.tar.gz)"),true,"TGZ (*.tar.gz)");archive->setObjectName("updateArchive");
        field(bundle,text("Sigstore provenance bundle (.json)","Bundle di provenienza Sigstore (.json)"),true,"JSON (*.json)");bundle->setObjectName("updateBundle");
        field(commit,text("Approved source commit (40 hexadecimal characters)","Commit sorgente approvato (40 caratteri esadecimali)"),false,{});commit->setMaxLength(40);commit->setObjectName("updateCommit");
        log=new QPlainTextEdit;log->setReadOnly(true);log->setObjectName("updateLog");layout->addWidget(log,1);auto *buttons=new QHBoxLayout;install=new QPushButton(text("Verify and install","Verifica e installa"));install->setObjectName("updateInstall");rollback=new QPushButton(text("Restore previous verified version","Ripristina versione verificata precedente"));rollback->setObjectName("updateRollback");close=new QPushButton(text("Close","Chiudi"));buttons->addWidget(install);buttons->addWidget(rollback);buttons->addWidget(close);layout->addLayout(buttons);
        connect(close,&QPushButton::clicked,this,&UpdateDialog::reject);
        connect(install,&QPushButton::clicked,this,[this]{if(!QRegularExpression("^[0-9a-fA-F]{40}$").match(commit->text()).hasMatch()||!QFileInfo::exists(archive->text())||!QFileInfo::exists(bundle->text())){log->setPlainText(text("Choose both files and a full approved commit.","Scegli entrambi i file e un commit completo approvato."));return;}run({"install","--archive",archive->text(),"--bundle",bundle->text(),"--commit",commit->text()});});
        connect(rollback,&QPushButton::clicked,this,[this]{run({"rollback"});});
        process.setProcessChannelMode(QProcess::MergedChannels);connect(&process,&QProcess::readyReadStandardOutput,this,[this]{output+=process.readAllStandardOutput();if(output.size()>1024*1024){process.kill();output="Update diagnostics exceeded limits";}log->setPlainText(QString::fromUtf8(output));});
        connect(&process,&QProcess::finished,this,[this](int code,QProcess::ExitStatus status){finish(status==QProcess::NormalExit?code:2);});connect(&process,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error){if(error==QProcess::FailedToStart){output+=process.errorString().toUtf8();finish(2);}});
    }
    void run(QStringList arguments) {
        if(process.state()!=QProcess::NotRunning)return;QString script;QDir app(QCoreApplication::applicationDirPath());for(const auto& path:{app.filePath("../share/orbyx/orbyx_update.py"),app.filePath("scripts/orbyx_update.py"),app.filePath("../scripts/orbyx_update.py")})if(QFileInfo::exists(path)){script=path;break;}
        auto python=QStandardPaths::findExecutable("python3");if(script.isEmpty()||python.isEmpty()){log->setPlainText(text("Update tool unavailable","Strumento di aggiornamento non disponibile"));return;}
        output.clear();completed=false;state(true);arguments.prepend(script);process.start(python,arguments);
    }
};
