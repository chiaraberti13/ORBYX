// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QWizard>
#include <QWizardPage>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QtEndian>
#include <QGroupBox>
class CalibrationReviewPage final : public QWizardPage {
public: void requireConfirmation(QCheckBox *box) {registerField("confirmed*",box);} };
// A real validation wizard: configuring it never starts processing or overwrites data.
class CalibrationWizard final : public QWizard {
    bool italian;
    QLineEdit *inputEdit{},*profileEdit{},*outputEdit{};
    QComboBox *satelliteEdit{};
    QDoubleSpinBox *scaleEdit{};
    QCheckBox *confirm{};
    QString text(const char* en,const char* it) const {return QString::fromUtf8(italian?it:en);}
    void pathField(QVBoxLayout *layout,QLineEdit *edit,const QString& title,bool profile,bool destination) {
        auto *label=new QLabel(title);label->setBuddy(edit);edit->setAccessibleName(title);
        layout->addWidget(label);layout->addWidget(edit);auto *button=new QPushButton(text("Choose…","Seleziona…"));
        button->setAccessibleName(title);layout->addWidget(button);
        connect(button,&QPushButton::clicked,this,[=] {
            QString path=profile?QFileDialog::getOpenFileName(this,title,edit->text(),"JSON (*.json)"):
                QFileDialog::getExistingDirectory(this,title,destination?QFileInfo(edit->text()).absolutePath():edit->text());
            if(!path.isEmpty())edit->setText(destination?QDir(path).filePath("orbyx-calibration"):path);
        });
    }
public:
    CalibrationWizard(bool it,const QString& input,const QString& profile,const QString& output,const QString& satellite,QWidget *parent=nullptr):QWizard(parent),italian(it) {
        setObjectName("calibrationWizard");setWindowTitle(text("Guided calibration","Calibrazione guidata"));setMinimumSize(620,540);
        setButtonText(QWizard::NextButton,text("Next","Avanti"));setButtonText(QWizard::BackButton,text("Back","Indietro"));
        setButtonText(QWizard::CancelButton,text("Cancel","Annulla"));setButtonText(QWizard::FinishButton,text("Use these settings","Usa queste impostazioni"));
        auto *source=new QWizardPage;source->setTitle(text("1. Choose the decoded acquisition","1. Scegli l’acquisizione decodificata"));auto *form=new QVBoxLayout(source);
        inputEdit=new QLineEdit(input);inputEdit->setObjectName("wizardInput");profileEdit=new QLineEdit(profile);profileEdit->setObjectName("wizardProfile");
        outputEdit=new QLineEdit(output);outputEdit->setObjectName("wizardOutput");
        pathField(form,inputEdit,text("SatDump RDAS folder","Cartella RDAS SatDump"),false,false);
        pathField(form,profileEdit,text("Existing calibration profile","Profilo di calibrazione esistente"),true,false);
        pathField(form,outputEdit,text("New output folder","Nuova cartella di destinazione"),false,true);addPage(source);
        auto *options=new QWizardPage;options->setTitle(text("2. Select satellite and working scale","2. Scegli satellite e scala di lavoro"));auto *fields=new QVBoxLayout(options);
        satelliteEdit=new QComboBox;satelliteEdit->setObjectName("wizardSatellite");satelliteEdit->addItems({"L2","L3","L4","L5","M1","M2"});if(satelliteEdit->findText(satellite)<0)satelliteEdit->addItem(satellite);satelliteEdit->setCurrentText(satellite);
        satelliteEdit->setAccessibleName(text("Satellite","Satellite"));fields->addWidget(satelliteEdit);
        auto *scaleLabel=new QLabel(text("ECC working scale (exports retain native resolution)","Scala di lavoro ECC (esportazione a risoluzione originale)"));fields->addWidget(scaleLabel);
        scaleEdit=new QDoubleSpinBox;scaleEdit->setObjectName("wizardScale");scaleEdit->setDecimals(3);scaleEdit->setRange(.0625,1);scaleEdit->setSingleStep(.125);scaleEdit->setValue(.125);scaleLabel->setBuddy(scaleEdit);scaleEdit->setAccessibleName(scaleLabel->text());fields->addWidget(scaleEdit);
        auto *note=new QLabel(text("Large images: use 0.125 or 0.25 to reduce calibration memory. ECC runs on CPU. The original images are kept. Merge regions and absolute geolocation need separate validation.","Immagini grandi: usa 0,125 o 0,25 per ridurre la memoria della calibrazione. ECC usa la CPU. Le immagini originali sono conservate. Le regioni di fusione e la geolocalizzazione assoluta richiedono una verifica separata."));note->setWordWrap(true);fields->addWidget(note);fields->addStretch();addPage(options);
        auto *review=new CalibrationReviewPage;review->setTitle(text("3. Review and apply","3. Controlla e applica"));auto *summary=new QVBoxLayout(review);
        auto *explanation=new QLabel(text("Finish prepares the main window. Press Start processing there to calculate calibration.json. After success, the new profile is selected automatically and the operation changes to channel alignment with a new destination.","Fine prepara la finestra principale. Premi Avvia elaborazione per calcolare calibration.json. Al termine il nuovo profilo viene selezionato automaticamente e l’operazione passa all’allineamento con una nuova destinazione."));explanation->setWordWrap(true);summary->addWidget(explanation);
        confirm=new QCheckBox(text("I have checked the satellite and understand the geometry limits.","Ho controllato il satellite e compreso i limiti della geometria."));confirm->setObjectName("wizardConfirm");summary->addWidget(confirm);review->requireConfirmation(confirm);summary->addStretch();addPage(review);
    }
    bool validateCurrentPage() override {
        if(currentId()==0) {
            QString error;
            const QFileInfo in(inputEdit->text()),profile(profileEdit->text()),out(outputEdit->text());
            if(!in.isDir()||in.isSymLink())error=text("Choose a local input directory.","Scegli una cartella di origine locale.");
            for(const auto& vis:{"MSUGS_VIS1","MSUGS_VIS2"})for(const auto& name:{"MSUGS-VIS-1.png","MSUGS-VIS-2.png","MSUGS-VIS-3.png","product.cbor"}) {
                QFileInfo file(QDir(inputEdit->text()).filePath(QString(vis)+"/"+name));
                if(!file.isFile()||file.isSymLink()||file.size()==0)error=text("The acquisition is incomplete.","L’acquisizione è incompleta.");
            }
            if(!profile.isFile()||profile.isSymLink())error=text("Choose an existing JSON profile.","Scegli un profilo JSON esistente.");
            else {
                QFile json(profile.absoluteFilePath());
                if(!json.open(QIODevice::ReadOnly)||json.size()>1024*1024||!QJsonDocument::fromJson(json.read(1024*1024+1)).isObject())error=text("Choose a valid JSON object profile (maximum 1 MiB).","Scegli un profilo JSON valido (massimo 1 MiB).");
            }
            for(const auto& vis:{"MSUGS_VIS1","MSUGS_VIS2"}) {
                QByteArray reference;
                for(const auto& name:{"MSUGS-VIS-1.png","MSUGS-VIS-2.png","MSUGS-VIS-3.png"}) {
                    QFileInfo info(QDir(inputEdit->text()).filePath(QString(vis)+"/"+name));
                    if(!info.isFile()||info.isSymLink())continue;
                    QFile file(info.absoluteFilePath());if(!file.open(QIODevice::ReadOnly)){error=text("Cannot read PNG header.","Impossibile leggere l’intestazione PNG.");continue;}
                    const auto header=file.read(33);
                    if(header.size()!=33||header.left(8)!=QByteArray::fromHex("89504e470d0a1a0a")||header.mid(8,8)!=QByteArray::fromHex("0000000d49484452")){error=text("Invalid PNG header.","Intestazione PNG non valida.");continue;}
                    const quint32 w=qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(header.constData()+16)),h=qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(header.constData()+20));
                    if(!w||h<=3606||quint64(w)*h>150000000||(header[24]!=8&&header[24]!=16)||header[25]!=0)error=text("Use native grayscale RDAS products, not reduced previews.","Usa prodotti RDAS nativi in scala di grigi, non anteprime ridotte.");
                    if(reference.isEmpty())reference=header.mid(16,10);else if(reference!=header.mid(16,10))error=text("Sensor channel geometry differs.","I canali del sensore hanno geometria differente.");
                }
            }
            for(const auto& vis:{"MSUGS_VIS1","MSUGS_VIS2"})
                if(QFileInfo(QDir(inputEdit->text()).filePath(vis)).isSymLink())error=text("Sensor directories must not be symlinks.","Le cartelle dei sensori non possono essere collegamenti simbolici.");
            const auto destination=QDir::cleanPath(out.absoluteFilePath());const auto origin=QDir::cleanPath(in.canonicalFilePath());
            if(outputEdit->text().trimmed().isEmpty()||out.exists()||out.isSymLink()||!QFileInfo(out.absolutePath()).isDir()||destination==origin||destination.startsWith(origin+"/"))error=text("Choose a new destination outside the input.","Scegli una destinazione nuova esterna all’origine.");
            if(!error.isEmpty()){setProperty("validationError",error);currentPage()->setSubTitle(error);return false;}
            currentPage()->setSubTitle({});
        }
        return QWizard::validateCurrentPage();
    }
    QString inputPath()const{return inputEdit->text();}QString profilePath()const{return profileEdit->text();}
    QString outputPath()const{return outputEdit->text();}QString satelliteId()const{return satelliteEdit->currentText();}
    double scale()const{return scaleEdit->value();}
};
