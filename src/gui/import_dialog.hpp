// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QDialog>
#include <QCoreApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QFileDialog>
#include <QProcess>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QSet>
#include <functional>
class ImportDialog final : public QDialog {
    QLineEdit *folder{};QTableWidget *products{};QLabel *status{};QCheckBox *automatic{};
    QProcess process;QTimer timer;QByteArray received;QString scannedFolder;QSet<QString> imported;bool overflow=false;
    const bool it;
    const std::function<bool(const QJsonObject&,bool)> import;
    QString text(const char* en,const char* italian)const{return QString::fromUtf8(it?italian:en);}
public:
    ImportDialog(bool italian,std::function<bool(const QJsonObject&,bool)> callback,QWidget *parent=nullptr):QDialog(parent),it(italian),import(std::move(callback)) {
        setObjectName("importDialog");setWindowTitle(text("Monitor acquisitions","Monitora acquisizioni"));resize(740,460);
        auto *layout=new QVBoxLayout(this);auto *explanation=new QLabel(text("Choose the parent folder where SatDump writes acquisitions. Only stable native products with unambiguous VIS metadata are accepted. Importing never starts processing.","Scegli la cartella principale in cui SatDump scrive le acquisizioni. Sono accettati solo prodotti nativi stabili con metadati VIS non ambigui. L’importazione non avvia l’elaborazione."));explanation->setWordWrap(true);layout->addWidget(explanation);
        auto *row=new QHBoxLayout;folder=new QLineEdit;folder->setObjectName("watchFolder");folder->setAccessibleName(text("Acquisition parent folder","Cartella principale acquisizioni"));auto *choose=new QPushButton(text("Choose…","Seleziona…"));row->addWidget(folder,1);row->addWidget(choose);layout->addLayout(row);
        connect(folder,&QLineEdit::textChanged,this,[this]{imported.clear();});
        automatic=new QCheckBox(text("Automatically queue products whose satellite matches the selected profile","Aggiungi automaticamente in coda i prodotti con satellite corrispondente al profilo selezionato"));automatic->setObjectName("watchAutomatic");layout->addWidget(automatic);
        products=new QTableWidget(0,2);products->setObjectName("importProducts");products->setHorizontalHeaderLabels({text("Acquisition","Acquisizione"),text("Status / metadata","Stato / metadati")});products->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);products->setSelectionBehavior(QAbstractItemView::SelectRows);products->setEditTriggers(QAbstractItemView::NoEditTriggers);products->setAccessibleName(text("Detected acquisitions","Acquisizioni rilevate"));layout->addWidget(products,1);
        status=new QLabel;status->setWordWrap(true);layout->addWidget(status);auto *buttons=new QHBoxLayout;auto *scan=new QPushButton(text("Scan / monitor","Scansiona / monitora"));scan->setObjectName("watchScan");auto *selected=new QPushButton(text("Import selected","Importa selezionata"));selected->setObjectName("watchImport");auto *close=new QPushButton(text("Close","Chiudi"));buttons->addWidget(scan);buttons->addWidget(selected);buttons->addStretch();buttons->addWidget(close);layout->addLayout(buttons);
        connect(choose,&QPushButton::clicked,this,[this]{auto p=QFileDialog::getExistingDirectory(this,windowTitle(),folder->text());if(!p.isEmpty()){folder->setText(p);start();}});
        connect(scan,&QPushButton::clicked,this,[this]{start();});connect(close,&QPushButton::clicked,this,&QDialog::reject);
        connect(selected,&QPushButton::clicked,this,[this]{int row=products->currentRow();if(row<0)return;auto data=products->item(row,0)->data(Qt::UserRole).toJsonObject();if(data["status"]=="ready")import(data,false);});
        timer.setInterval(5000);connect(&timer,&QTimer::timeout,this,[this]{scanOnce();});
        connect(&process,&QProcess::readyReadStandardOutput,this,[this]{received+=process.readAllStandardOutput();if(received.size()>4*1024*1024){overflow=true;process.kill();}});
        connect(&process,&QProcess::errorOccurred,this,[this](QProcess::ProcessError e){if(e==QProcess::FailedToStart)status->setText(process.errorString());});
        connect(&process,&QProcess::finished,this,[this](int code,QProcess::ExitStatus exit) {
            received+=process.readAllStandardOutput();
            if(scannedFolder!=folder->text())return;
            if(overflow||code||exit!=QProcess::NormalExit){status->setText(text("Inspection failed; no product imported.","Ispezione non riuscita; nessun prodotto importato."));return;}
            QJsonParseError error;const auto doc=QJsonDocument::fromJson(received,&error);
            if(error.error!=QJsonParseError::NoError||doc.object()["schema"]!="orbyx-import/1"||!doc.object()["products"].isArray()){status->setText(text("Invalid inspection report","Report di ispezione non valido"));return;}
            products->setRowCount(0);
            for(const auto &entry:doc.object()["products"].toArray()) {
                auto data=entry.toObject();int row=products->rowCount();products->insertRow(row);auto *name=new QTableWidgetItem(QFileInfo(data["path"].toString()).fileName());name->setData(Qt::UserRole,data);products->setItem(row,0,name);
                const QString state=data["status"].toString();QString translated=state=="ready"?text("Ready","Pronta"):state=="unstable"?text("Still writing","Scrittura in corso"):state=="incomplete"?text("Incomplete","Incompleta"):text("Rejected","Rifiutata");
                products->setItem(row,1,new QTableWidgetItem(translated+" · "+data["satellite"].toString()+" "+data["reason"].toString()));
                QString identity=data["path"].toString()+":"+data["fingerprint"].toString();
                if(automatic->isChecked()&&state=="ready"&&!data["satellite"].isNull()&&!imported.contains(identity)) {if(import(data,true))imported.insert(identity);}
            }
            status->setText(text("Monitoring active. Select a product to import; use Run queue in the main window to process it.","Monitoraggio attivo. Seleziona un prodotto da importare; usa Avvia coda nella finestra principale per elaborarlo."));
        });
    }
    ~ImportDialog()override {timer.stop();if(process.state()!=QProcess::NotRunning){process.kill();process.waitForFinished(1000);}}
    bool scanning()const{return process.state()!=QProcess::NotRunning;}
    void start(){timer.start();scanOnce();}
    void scanOnce() {
        if(process.state()!=QProcess::NotRunning||folder->text().isEmpty())return;
        QString script;const QDir app(QCoreApplication::applicationDirPath());
        for(const auto& candidate:{app.filePath("../share/orbyx/orbyx_import.py"),app.filePath("scripts/orbyx_import.py"),app.filePath("../scripts/orbyx_import.py")})if(QFileInfo::exists(candidate)){script=candidate;break;}
        const auto python=QStandardPaths::findExecutable("python3");if(script.isEmpty()||python.isEmpty()){status->setText(text("Import tool unavailable","Strumento di importazione non disponibile"));return;}
        scannedFolder=folder->text();received.clear();overflow=false;process.start(python,{script,folder->text(),"--settle","0.4"});
    }
};
