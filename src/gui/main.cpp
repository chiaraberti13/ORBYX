// SPDX-License-Identifier: GPL-3.0-only
// ORBYX desktop workspace. Processing lives exclusively in the shared core.
#include "orbyx/engine.hpp"
#include "calibration_wizard.hpp"
#include "import_dialog.hpp"
#include "update_dialog.hpp"
#include "queue_persistence.hpp"
#include "mosaic_dialog.hpp"
#include "final_image_wizard.hpp"

#include <QAbstractItemView>
#include <QApplication>
#include <QCloseEvent>
#include <QFile>
#include <QSaveFile>
#include <QSet>
#include <QRegularExpression>
#include <QPointer>
#include <QUuid>
#include <QDateTime>
#include <QComboBox>
#include <QDir>
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMap>
#include <QMessageBox>
#include <QPixmap>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSplitter>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>
#include <filesystem>

namespace {
const QMap<QString, QString> italian = {
 {"Satellite imaging workspace", "Ambiente di elaborazione satellitare"},
 {"ALIGN / CALIBRATE / EXPLORE", "ALLINEA / CALIBRA / ESPLORA"},
 {"Processing setup", "Configurazione elaborazione"},
 {"RDAS input folder", "Cartella RDAS di origine"},
 {"Calibration profile", "Profilo di calibrazione"},
 {"Calibration profile (included)", "Profilo di calibrazione (incluso)"},
 {"Run the image demo on a chosen folder of six JPEGs; defaults to the bundled demo images.",
  "Esegue la demo su una cartella di sei JPEG a tua scelta; predefinita la cartella demo inclusa."},
 {"Choose six demo JPEGs (sx_/dx_ MSUGS-VIS-1/2/3)", "Scegli sei JPEG demo (sx_/dx_ MSUGS-VIS-1/2/3)"},
 {"This folder does not contain the six JPEGs sx_/dx_ MSUGS-VIS-1/2/3.",
  "Questa cartella non contiene i sei JPEG sx_/dx_ MSUGS-VIS-1/2/3."},
 {"New output folder", "Nuova cartella di destinazione"},
 {"Select…", "Seleziona…"}, {"Satellite", "Satellite"}, {"Operation", "Operazione"},
 {"Align channels", "Allinea canali"}, {"Calculate calibration", "Calcola calibrazione"},
 {"Automatic CPU / OpenCL", "CPU / OpenCL automatico"}, {"CPU only", "Solo CPU"},
 {"Processing engine", "Motore di elaborazione"}, {"Start processing", "Avvia elaborazione"},
 {"Ready", "Pronto"}, {"Processing…", "Elaborazione…"}, {"Completed", "Completato"},
 {"Processing failed", "Elaborazione non riuscita"}, {"Image preview", "Anteprima immagine"},
 {"Preview channel", "Canale di anteprima"},
 {"Drop one local RDAS folder here, or select it using the browser.", "Trascina qui una cartella RDAS locale oppure selezionala con Sfoglia."},
 {"Input", "Origine"}, {"Output", "Risultato"}, {"Fit", "Adatta"},
 {"Choose a folder to preview its first channel. Scroll to pan; use + / − to zoom.", "Scegli una cartella per visualizzare il primo canale. Scorri per spostarti; usa + / − per lo zoom."},
 {"Activity", "Attività"}, {"Clear", "Pulisci"}, {"Language", "Lingua"},
 {"Dark", "Scuro"}, {"Light", "Chiaro"}, {"Theme", "Tema"},
 {"A new output directory is required. Existing files are never overwritten.", "È necessaria una nuova cartella di destinazione. I file esistenti non vengono sovrascritti."},
 {"Calibration writes a new calibration.json in the output folder. Merge regions remain manual.", "La calibrazione crea un nuovo calibration.json nella destinazione. Le regioni di fusione restano manuali."},
 {"Alignment preserves the original input and uses the selected calibration profile.", "L’allineamento preserva i dati originali e usa il profilo di calibrazione selezionato."},
 {"Choose RDAS folder", "Scegli cartella RDAS"}, {"Choose calibration profile", "Scegli profilo di calibrazione"},
 {"Select output parent folder", "Scegli cartella padre per il risultato"},
 {"Missing settings", "Impostazioni mancanti"},
 {"Select an input folder, calibration profile and new output folder.", "Seleziona origine, profilo di calibrazione e nuova destinazione."},
 {"Cannot close during processing", "Elaborazione in corso"},
 {"Wait for processing to finish before closing this window.", "Attendi il termine dell’elaborazione prima di chiudere la finestra."},
 {"No readable PNG preview", "Nessuna anteprima PNG leggibile"},
 {"Full-resolution exports are unchanged; preview is limited to 2048 pixels.", "Le esportazioni mantengono la risoluzione completa; l’anteprima è limitata a 2048 pixel."},
 {"Technical diagnostics from the processing engine", "Diagnostica tecnica del motore di elaborazione"},
 {"Browse the RDAS input folder", "Sfoglia la cartella RDAS di origine"},
 {"Browse calibration profiles", "Sfoglia i profili di calibrazione"},
 {"Choose a parent folder and edit the new destination name", "Scegli una cartella padre e modifica il nome della nuova destinazione"},
 {"L4 needs a calibrated profile. Select an existing L4 profile or calculate a new one.", "L4 richiede un profilo calibrato. Seleziona un profilo L4 esistente oppure calcolane uno nuovo."},
 {"Zoom in", "Ingrandisci"}, {"Zoom out", "Riduci"},
 {"Processing queue", "Coda di elaborazione"},
 {"Add to queue", "Aggiungi alla coda"}, {"Remove", "Rimuovi"}, {"Run queue", "Avvia coda"},
 {"Job", "Lavoro"}, {"Status", "Stato"}, {"Queued", "In coda"}, {"Running…", "In esecuzione…"},
 {"Needs verification", "Da verificare"}, {"Interrupted", "Interrotto"},
 {"Done", "Completato"}, {"Skipped", "Saltato"}, {"Error", "Errore"},
 {"The queue runs jobs in order with durable, resumable state. Existing outputs are never overwritten.",
  "La coda esegue i lavori in ordine con stato durevole e ripristinabile. Gli output esistenti non vengono mai sovrascritti."},
 {"Fill in input, calibration profile and a new output folder before queueing.",
  "Compila origine, profilo di calibrazione e una nuova cartella di destinazione prima di accodare."},
 {"The batch runner is not available.", "L'esecutore batch non è disponibile."},
 {"Duplicate profile…", "Duplica profilo…"},
 {"Duplicate calibration profile", "Duplica profilo di calibrazione"},
 {"Source satellite", "Satellite di origine"}, {"New satellite", "Nuovo satellite"},
 {"Save duplicated profile", "Salva profilo duplicato"},
 {"Profile duplicated", "Profilo duplicato"}, {"Could not duplicate the profile", "Impossibile duplicare il profilo"},
 {"Export Natural Color…", "Esporta Natural Color…"},
 {"Run image demo", "Avvia demo immagini"},
 {"Real JPEG demo — six channels, one image", "Demo JPEG reali — sei canali, un’unica immagine"},
 {"Align the six bundled JPEGs and join the left and right halves.", "Allinea i sei JPEG inclusi e unisce le metà sinistra e destra."},
 {"Demo image saved", "Immagine demo salvata"},
 {"Choose aligned VIS channels", "Scegli i canali VIS allineati"},
 {"Save Natural Color PNG", "Salva PNG Natural Color"},
 {"Natural Color exported", "Natural Color esportato"},
 {"RGB321 hue rendering preserves 8/16-bit depth. Select one aligned VIS product. This is not calibrated true colour.", "La resa RGB321 con correzione hue mantiene 8/16 bit. Seleziona un prodotto VIS allineato. Non è colore scientificamente calibrato."}
};

class Window final : public QMainWindow {
    QSettings settings{"ORBYX", "Desktop"};
    bool busy = false;
    bool it = false;
    bool dark = true;
    bool quitAfterRun = false;
    bool demoMode = false;
    QString lastOutput;
    QString demoResult;
    QPushButton *demoButton{}, *simulateButton{}, *wizardButton{}, *finalImageButton{}, *watchButton{}, *updatesButton{};
    bool openFinalEditor=false;
    bool finalNaturalColour=true;
    QString importedFingerprint;
    QPointer<ImportDialog> importWindow;
    QPointer<UpdateDialog> updateWindow;
    double guidedScale=1.0;
    QPixmap image;
    double zoom = 1.0;
    QLabel *subtitle{}, *eyebrow{}, *inputLabel{}, *configLabel{}, *outputLabel{}, *satLabel{}, *modeLabel{}, *engineLabel{}, *hint{}, *previewTitle{}, *empty{}, *status{}, *activityTitle{}, *languageLabel{}, *themeLabel{}, *previewNote{};
    QGroupBox *setup{};
    QLineEdit *input{}, *config{}, *output{};
    QPushButton *inputBrowse{}, *configBrowse{}, *outputBrowse{}, *start{}, *fit{}, *plus{}, *minus{}, *clear{};
    QComboBox *satellite{}, *mode{}, *engine{}, *language{}, *theme{}, *source{}, *channel{};
    QProgressBar *progress{};
    QScrollArea *scroll{};
    QLabel *canvas{};
    QTextEdit *log{};
    QGroupBox *queueBox{};
    QTableWidget *queue{};
    QPushButton *queueAdd{}, *queueRemove{}, *queueRun{}, *profileDup{}, *naturalExport{}, *mosaicButton{};
    QProcess *queueProcess{};
    QueuePersistence queueStore{QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/desktop-queue.json"};
    bool queueStoreHealthy=true;

    QString trText(const QString &s) const { return it ? italian.value(s, s) : s; }
    QLabel *label(const QString &name = {}) { auto *w = new QLabel; w->setObjectName(name); return w; }
    QHBoxLayout *pathRow(QLineEdit *edit, QPushButton *button) {
        auto *row = new QHBoxLayout; row->setSpacing(8); row->addWidget(edit, 1); row->addWidget(button); return row;
    }
    QString diagnostic(const QString &message) const {
        return QString::fromStdString(orbyx::translate(message.toStdString(), it ? "it" : "en"));
    }
    void applyText() {
        subtitle->setText(trText(demoMode?"Real JPEG demo — six channels, one image":"Satellite imaging workspace")); eyebrow->setText(trText("ALIGN / CALIBRATE / EXPLORE"));
        updatesButton->setText(it?"Aggiornamenti verificati…":"Verified updates…");updatesButton->setAccessibleName(updatesButton->text());
        watchButton->setText(it?"Monitora acquisizioni…":"Monitor acquisitions…");watchButton->setAccessibleName(watchButton->text());
        wizardButton->setText(it?"Calibrazione guidata…":"Guided calibration…");
        wizardButton->setAccessibleName(wizardButton->text());
        finalImageButton->setText(it?"Crea immagine finale…":"Create final image…");
        finalImageButton->setAccessibleName(finalImageButton->text());
        demoButton->setText(trText("Run image demo"));
        demoButton->setAccessibleName(trText("Run image demo"));
        demoButton->setToolTip(trText("Align the six bundled JPEGs and join the left and right halves."));
        simulateButton->setText(it?"Simulazione immagini…":"Image simulation…");
        simulateButton->setAccessibleName(simulateButton->text());
        simulateButton->setToolTip(trText("Run the image demo on a chosen folder of six JPEGs; defaults to the bundled demo images."));
        setup->setTitle(trText("Processing setup"));
        inputLabel->setText(trText("RDAS input folder")); configLabel->setText(trText("Calibration profile (included)")); outputLabel->setText(trText("New output folder"));
        satLabel->setText(trText("Satellite")); modeLabel->setText(trText("Operation")); engineLabel->setText(trText("Processing engine"));
        for (auto *b : {inputBrowse, configBrowse, outputBrowse}) b->setText(trText("Select…"));
        inputBrowse->setAccessibleName(trText("Browse the RDAS input folder")); configBrowse->setAccessibleName(trText("Browse calibration profiles")); outputBrowse->setAccessibleName(trText("Choose a parent folder and edit the new destination name"));
        input->setAccessibleName(trText("RDAS input folder")); config->setAccessibleName(trText("Calibration profile")); output->setAccessibleName(trText("New output folder"));
        output->setToolTip(trText("A new output directory is required. Existing files are never overwritten."));
        mode->setItemText(0, trText("Align channels")); mode->setItemText(1, trText("Calculate calibration"));
        engine->setItemText(0, trText("Automatic CPU / OpenCL")); engine->setItemText(1, trText("CPU only"));
        source->setItemText(0, trText("Input")); source->setItemText(1, trText("Output"));
        theme->setItemText(0, trText("Dark")); theme->setItemText(1, trText("Light"));
        start->setText(trText(busy ? "Processing…" : "Start processing"));
        hint->setText(trText(satellite->currentText() == "L4" && mode->currentIndex() == 0 ? "L4 needs a calibrated profile. Select an existing L4 profile or calculate a new one." : mode->currentIndex() == 1 ? "Calibration writes a new calibration.json in the output folder. Merge regions remain manual." : "Alignment preserves the original input and uses the selected calibration profile."));
        previewTitle->setText(trText("Image preview")); empty->setText(trText("Drop one local RDAS folder here, or select it using the browser."));
        previewNote->setText(trText("Full-resolution exports are unchanged; preview is limited to 2048 pixels."));
        fit->setText(trText("Fit")); plus->setAccessibleName(trText("Zoom in")); minus->setAccessibleName(trText("Zoom out"));
        activityTitle->setText(trText("Activity")); clear->setText(trText("Clear")); languageLabel->setText(trText("Language")); themeLabel->setText(trText("Theme"));
        log->setAccessibleName(trText("Technical diagnostics from the processing engine"));
        satellite->setAccessibleName(trText("Satellite")); mode->setAccessibleName(trText("Operation")); engine->setAccessibleName(trText("Processing engine"));
        language->setAccessibleName(trText("Language")); theme->setAccessibleName(trText("Theme")); source->setAccessibleName(trText("Image preview")); channel->setAccessibleName(trText("Preview channel"));
        inputLabel->setBuddy(input); configLabel->setBuddy(config); outputLabel->setBuddy(output); satLabel->setBuddy(satellite); modeLabel->setBuddy(mode); engineLabel->setBuddy(engine);
        queueBox->setTitle(trText("Processing queue"));
        queueBox->setToolTip(trText("The queue runs jobs in order with durable, resumable state. Existing outputs are never overwritten."));
        queueAdd->setText(trText("Add to queue")); queueRemove->setText(trText("Remove")); queueRun->setText(trText("Run queue"));
        profileDup->setText(trText("Duplicate profile…"));
        mosaicButton->setText(it?"Componi porzioni e colore…":"Compose portions and colour…");
        mosaicButton->setAccessibleName(mosaicButton->text());
        naturalExport->setText(trText("Export Natural Color…"));
        naturalExport->setAccessibleName(trText("Export Natural Color…"));
        naturalExport->setToolTip(trText("RGB321 hue rendering preserves 8/16-bit depth. Select one aligned VIS product. This is not calibrated true colour."));
        for(int row=0;row<queue->rowCount();++row) {
            queue->item(row,2)->setText(trText(queueStatusLabel(queue->item(row,2)->data(Qt::UserRole).toString())));
            const auto job=queue->item(row,0)->data(Qt::UserRole).toMap();
            queue->item(row,1)->setText(job["satellite"].toString()+" · "+trText(job["mode"]=="calibrate"?"Calculate calibration":"Align channels"));
        }
        queue->setHorizontalHeaderLabels({trText("Job"), trText("Operation"), trText("Status")});
        queueAdd->setAccessibleName(trText("Add to queue")); queueRemove->setAccessibleName(trText("Remove"));
        queueRun->setAccessibleName(trText("Run queue")); profileDup->setAccessibleName(trText("Duplicate profile…"));
        queue->setAccessibleName(trText("Processing queue"));
        queue->setObjectName("queueTable"); queueAdd->setObjectName("queueAdd");
        queueRemove->setObjectName("queueRemove"); queueRun->setObjectName("queueRun");
        if (!busy && progress->value() == 0) status->setText(trText("Ready"));
    }
    void applyTheme() {
        const QString bg = dark ? "#0b111b" : "#f0f4f8";
        const QString panel = dark ? "#131e2d" : "#ffffff";
        const QString field = dark ? "#0c1521" : "#f5f8fc";
        const QString fg = dark ? "#e7eef7" : "#1b2a3f";
        const QString muted = dark ? "#a3b3c8" : "#526780";
        const QString border = dark ? "#2a3b51" : "#cfdae6";
        setStyleSheet(QString(
            "QMainWindow,QWidget#root {background:%1;color:%4;}"
            "QWidget {font-family:'Inter','DejaVu Sans',sans-serif;font-size:13px;color:%4;}"
            "QGroupBox {background:%2;border:1px solid %6;border-radius:12px;margin-top:13px;padding:12px 12px 10px;font-weight:600;}"
            "QGroupBox::title {subcontrol-origin:margin;left:17px;padding:0 7px;}"
            "QFrame#panel {background:%2;border:1px solid %6;border-radius:12px;}"
            "QLabel#brand {font-size:32px;font-weight:700;letter-spacing:5px;color:#46cabc;}"
            "QLabel#muted,QLabel#eyebrow {color:%5;} QLabel#eyebrow {font-size:11px;letter-spacing:2px;}"
            "QLabel#section {font-size:16px;font-weight:600;}"
            "QLineEdit,QComboBox,QTextEdit {background:%3;border:1px solid %6;border-radius:6px;padding:4px 8px;selection-background-color:#238d87;}"
            "QTableWidget {background:%3;alternate-background-color:%2;gridline-color:%6;border:1px solid %6;}"
            "QTableWidget::item:selected {background:#238d87;color:#ffffff;}"
            "QHeaderView::section {background:%2;color:%4;border:1px solid %6;padding:5px;}"
            "QTableCornerButton::section {background:%2;border:1px solid %6;}"
            "QComboBox::drop-down {border:0;width:22px;} QComboBox QAbstractItemView {background:%2;color:%4;selection-background-color:#238d87;}"
            "QPushButton {background:%2;border:1px solid %6;border-radius:6px;padding:4px 10px;}"
            "QPushButton:hover {border-color:#46cabc;} QPushButton:focus,QLineEdit:focus,QComboBox:focus {border:2px solid #46cabc;}"
            "QPushButton#start {background:#46cabc;color:#062c2c;border:0;font-weight:700;padding:13px;}"
            "QPushButton:disabled {color:%5;background:%3;}"
            "QScrollArea {border:0;background:%3;} QLabel#canvas {background:%3;}"
            "QTextEdit {font-family:'DejaVu Sans Mono',monospace;font-size:12px;}"
            "QProgressBar {background:%3;border:0;border-radius:3px;max-height:6px;} QProgressBar::chunk {background:#46cabc;border-radius:3px;}"
            "QSplitter::handle {background:transparent;}"
        ).arg(bg,panel,field,fg,muted,border));
    }
    void resizeImage() {
        if (image.isNull()) return;
        const auto size = image.size() * zoom;
        canvas->setPixmap(image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        canvas->resize(size);
    }
    void fitImage() {
        if (image.isNull()) return;
        zoom = std::min(double(scroll->viewport()->width()-20)/image.width(), double(scroll->viewport()->height()-20)/image.height());
        zoom = std::max(0.02, std::min(zoom, 4.0)); resizeImage();
    }
    void loadPreview() {
        const QString folder = source->currentIndex() == 1 ? lastOutput : input->text();
        QString file;
        if (!folder.isEmpty() && QDir(folder).exists()) {
            // Fixed product locations avoid following arbitrary recursive symlinks.
            const QString name=QString("MSUGS-VIS-%1.png").arg(channel->currentIndex()+1);
            const QStringList paths = {"MSUGS_VIS_Aligned/"+name, "MSUGS_VIS-1_Aligned/"+name, "MSUGS_VIS1/"+name, name};
            for (const auto &p : paths) if (QFileInfo(QDir(folder).filePath(p)).isFile()) { file = QDir(folder).filePath(p); break; }
        }
        if (source->currentIndex() == 1 && !demoResult.isEmpty() && folder == lastOutput) file = demoResult;
        image = {};
        if (!file.isEmpty()) {
            QImageReader reader(file);
            const QSize original = reader.size();
            // Prevent oversized untrusted images from exhausting preview memory.
            if (original.isValid() && original.width() <= 100000 && original.height() <= 100000 && qint64(original.width()) * original.height() <= 250000000) {
                reader.setScaledSize(original.scaled(2048, 2048, Qt::KeepAspectRatio));
                image = QPixmap::fromImage(reader.read());
            }
        }
        canvas->setVisible(!image.isNull()); empty->setVisible(image.isNull());
        if (!image.isNull()) fitImage();
        else if (!file.isEmpty()) log->append(trText("No readable PNG preview"));
    }
    void exportNatural(const QString &folder,const QString &destination) {
        if (busy) return;
        setQueueBusy(true); progress->setRange(0,0); status->setText(trText("Processing…")); applyText();
        auto *worker=QThread::create([this,folder,destination] {
            QString error;
            try {
                std::array<std::filesystem::path,3> channels;
                for(int i=0;i<3;++i)channels[i]=QDir(folder).filePath(QString("MSUGS-VIS-%1.png").arg(i+1)).toStdString();
                orbyx::composeNaturalColor(channels,destination.toStdString());
            } catch(const std::exception &e) {error=QString::fromUtf8(e.what());}
            catch(...) {error="Unknown processing exception";}
            QMetaObject::invokeMethod(this,[this,error,destination] {
                setQueueBusy(false); progress->setRange(0,100); progress->setValue(error.isEmpty()?100:0); applyText();
                status->setText(trText(error.isEmpty()?"Natural Color exported":"Processing failed"));
                if(quitAfterRun) {QApplication::exit(error.isEmpty()?0:2);return;}
                if(error.isEmpty())log->append((trText("Natural Color exported")+": "+destination).toHtmlEscaped());
                else {
                    log->append(diagnostic(error).toHtmlEscaped());
                    QMessageBox box(QMessageBox::Warning,trText("Processing failed"),diagnostic(error),QMessageBox::Ok,this);
                    box.setTextFormat(Qt::PlainText);box.exec();
                }
            },Qt::QueuedConnection);
        });
        connect(worker,&QThread::finished,worker,&QObject::deleteLater);worker->start();
    }
    // Bundled calibration profile shipped with ORBYX so users never need to
    // supply or hunt for a .json: the GUI pre-fills it and restores it if cleared.
    QString bundledProfile() const {
        const QDir appDir(QApplication::applicationDirPath());
        for (const QString &candidate : {appDir.filePath("../share/orbyx/calibration.json"),
             QDir::current().filePath("resources/calibration.json"),
             appDir.filePath("../resources/calibration.json")})
            if (QFileInfo::exists(candidate)) return candidate;
        return {};
    }
    // Folder holding the six bundled demo JPEGs (sx_/dx_ MSUGS-VIS-1/2/3).
    QString bundledDemoFolder() const {
        const QDir appDir(QApplication::applicationDirPath());
        for (const QString &candidate : {appDir.filePath("../share/orbyx/demo-jpeg"),appDir.filePath("share/orbyx/demo-jpeg"),
             appDir.filePath("../assets/demo-jpeg"),QDir::current().filePath("assets/demo-jpeg")})
            if (QFileInfo(QDir(candidate).filePath("sx_MSUGS-VIS-1.jpg")).isFile()) return candidate;
        return {};
    }
    bool hasDemoImages(const QString &folder) const {
        if (folder.isEmpty()) return false;
        for (const QString &name : {"sx_MSUGS-VIS-1.jpg","sx_MSUGS-VIS-2.jpg","sx_MSUGS-VIS-3.jpg",
             "dx_MSUGS-VIS-1.jpg","dx_MSUGS-VIS-2.jpg","dx_MSUGS-VIS-3.jpg"})
            if (!QFileInfo(QDir(folder).filePath(name)).isFile()) return false;
        return true;
    }
    // Manual image simulation: run the real image pipeline on a folder of six
    // JPEGs the user picks, defaulting to the bundled demo images. No RDAS data
    // and no profile file are required; it reuses the same engine as the demo.
    void imageSimulationDialog() {
        if (busy) return;
        const QString start=bundledDemoFolder();
        const QString folder=QFileDialog::getExistingDirectory(this,
            trText("Choose six demo JPEGs (sx_/dx_ MSUGS-VIS-1/2/3)"),
            start.isEmpty()?QDir::homePath():start);
        if (folder.isEmpty()) return;
        if (!hasDemoImages(folder)) {
            QMessageBox::warning(this,trText("Missing settings"),
                trText("This folder does not contain the six JPEGs sx_/dx_ MSUGS-VIS-1/2/3."));
            return;
        }
        executeImageDemo({},folder);
    }
    void executeImageDemo(const QString &requestedOutput = {}, const QString &sampleFolder = {}) {
        if (busy) return;
        const QString sample = hasDemoImages(sampleFolder) ? sampleFolder : bundledDemoFolder();
        const QString base=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        const QString token=QDateTime::currentDateTimeUtc().toString("yyyyMMddTHHmmssZ")+"-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
        QDir().mkpath(base+"/demos"); QDir().mkpath(base+"/logs");
        const QString destination=requestedOutput.isEmpty()?base+"/demos/"+token:requestedOutput;
        const QString logPath=base+"/logs/demo-"+token+".log";
        setProperty("demoFinished",false);
        demoMode=true;setQueueBusy(true);progress->setRange(0,0);applyText();status->setText(trText("Processing…"));
        auto *worker=QThread::create([this,sample,destination,logPath] {
            QString error;QStringList entries;
            try {
                orbyx::runImageDemo(sample.toStdString(),destination.toStdString(),[this,&entries](const std::string &line) {
                    const auto text=QString::fromStdString(line);entries.append(text);
                    QMetaObject::invokeMethod(this,[this,text]{log->append(text.toHtmlEscaped());},Qt::QueuedConnection);
                });
            } catch (const std::exception &e) {error=QString::fromUtf8(e.what());}
            catch (...) {error="Unknown processing exception";}
            if (!error.isEmpty())entries.append("ERROR: "+error);
            QFile diagnosticFile(logPath);
            if (!diagnosticFile.open(QIODevice::WriteOnly|QIODevice::NewOnly) ||
                diagnosticFile.write(entries.join('\n').toUtf8())<0) {
                if(error.isEmpty())error="Could not save demo log";
            }
            QMetaObject::invokeMethod(this,[this,error,destination,logPath] {
                setProperty("demoFinished",true);setProperty("demoSucceeded",error.isEmpty());
                setQueueBusy(false);progress->setRange(0,100);progress->setValue(error.isEmpty()?100:0);applyText();
                status->setText(trText(error.isEmpty()?"Completed":"Processing failed"));
                if(error.isEmpty()) {
                    lastOutput=destination;demoResult=QDir(destination).filePath("composite.png");
                    source->setCurrentIndex(1);loadPreview();
                    log->append((trText("Demo image saved")+": "+demoResult).toHtmlEscaped());
                } else log->append(error.toHtmlEscaped());
                log->append(("Log: "+logPath).toHtmlEscaped());
                if(quitAfterRun)QApplication::exit(error.isEmpty()?0:2);
            },Qt::QueuedConnection);
        });
        connect(worker,&QThread::finished,worker,&QObject::deleteLater);worker->start();
    }
    void naturalColorDialog() {
        if(busy)return;
        const auto folder=QFileDialog::getExistingDirectory(this,trText("Choose aligned VIS channels"),lastOutput);
        if(folder.isEmpty())return;
        const auto destination=QFileDialog::getSaveFileName(this,trText("Save Natural Color PNG"),QDir(folder).filePath("natural-color.png"),"PNG (*.png)");
        if(!destination.isEmpty())exportNatural(folder,destination);
    }
    void openMosaic(bool natural=true) {
        MosaicDialog dialog(it,this);
        const QDir generated(lastOutput);
        if(QFileInfo(generated.filePath("MSUGS_VIS-1_Aligned/MSUGS-VIS-1.png")).isFile()&&
           QFileInfo(generated.filePath("MSUGS_VIS-2_Aligned/MSUGS-VIS-1.png")).isFile())
            dialog.appendGeneratedProducts(lastOutput,config->text(),satellite->currentText(),natural);
        dialog.exec();
    }
    void execute() {
        if (busy) return;
        // The calibration profile is bundled: if the user cleared it, restore the
        // included one so they are never blocked for lack of a .json to load.
        if (config->text().isEmpty()) config->setText(bundledProfile());
        if (input->text().isEmpty() || config->text().isEmpty() || output->text().isEmpty()) {
            if (quitAfterRun) { QApplication::exit(2); return; }
            QMessageBox::warning(this, trText("Missing settings"), trText("Select an input folder, calibration profile and new output folder.")); return;
        }
        orbyx::RunOptions options;
        options.input = input->text().toStdString(); options.config = config->text().toStdString(); options.output = output->text().toStdString();
        options.calibrationScale=mode->currentIndex()==1?guidedScale:1.0;
        options.language = it ? "it" : "en";
        options.satellite = satellite->currentText().toStdString(); options.cpu = engine->currentIndex() == 1; options.calibrate = mode->currentIndex() == 1;
        options.editableParts=!options.calibrate;
        lastOutput = output->text(); setQueueBusy(true);
        progress->setRange(0,0); status->setText(trText("Processing…")); applyText();
        if (!quitAfterRun) { settings.setValue("input", input->text()); settings.setValue("config", config->text()); settings.setValue("satellite", satellite->currentText()); }
        auto *worker = QThread::create([this, options] {
            QString error;
            try {
                orbyx::run(options, [this](const std::string &message) {
                    const QString text = QString::fromStdString(message);
                    QMetaObject::invokeMethod(this, [this,text]{ log->append(text.toHtmlEscaped()); }, Qt::QueuedConnection);
                });
            } catch (const std::exception &e) { error = QString::fromUtf8(e.what()); }
            catch (...) { error = "Unknown processing exception"; }
            QMetaObject::invokeMethod(this, [this,error,options] {
                const bool launchFinalEditor=openFinalEditor;
                openFinalEditor=false;
                setQueueBusy(false); progress->setRange(0,100); progress->setValue(error.isEmpty()?100:0); applyText();
                status->setText(trText(error.isEmpty()?"Completed":"Processing failed"));
                if (quitAfterRun) { QApplication::exit(error.isEmpty()?0:2); return; }
                if (!error.isEmpty()) { log->append(error.toHtmlEscaped()); QMessageBox box(QMessageBox::Warning,trText("Processing failed"),diagnostic(error),QMessageBox::Ok,this); box.setTextFormat(Qt::PlainText); box.setDetailedText(error); box.exec(); }
                else {
                    if(options.calibrate) {
                        config->setText(QDir(lastOutput).filePath("calibration.json"));mode->setCurrentIndex(0);
                        output->setText(lastOutput+"-aligned");guidedScale=1.0;
                        log->append(it?"Profilo calcolato selezionato. Controlla le regioni di fusione, poi avvia l’allineamento.":"Calculated profile selected. Review merge regions, then start alignment.");
                    }
                    source->setCurrentIndex(1); loadPreview();
                    if(launchFinalEditor&&!options.calibrate)
                        QTimer::singleShot(0,this,[this]{openMosaic(finalNaturalColour);});
                }
            }, Qt::QueuedConnection);
        });
        connect(worker,&QThread::finished,worker,&QObject::deleteLater); worker->start();
    }
    QString batchRunner() const {
        const QString appDir = QCoreApplication::applicationDirPath();
        QString runner = QStandardPaths::findExecutable("orbyx-batch");
        if (runner.isEmpty()) {
            const QString local = QDir(appDir).filePath("orbyx-batch");
            if (QFileInfo::exists(local)) runner = local;
        }
        if (runner.isEmpty())
            for (const QString &candidate : {QDir(appDir).filePath("../scripts/orbyx_batch.py"),
                                             QDir(appDir).filePath("scripts/orbyx_batch.py")})
                if (QFileInfo::exists(candidate)) { runner = candidate; break; }
        return runner;
    }
    const char *queueStatusLabel(const QString& state) const {
        if(state=="completed")return "Done";if(state=="skipped")return "Skipped";
        if(state=="running")return "Running…";if(state=="queued")return "Queued";
        if(state=="needs_verification")return "Needs verification";if(state=="interrupted")return "Interrupted";
        return "Error";
    }
    void appendQueueJob(const QVariantMap& job,QString state="queued") {
        const int row=queue->rowCount();queue->insertRow(row);auto *item=new QTableWidgetItem(QString::number(row+1));
        item->setData(Qt::UserRole,job);item->setToolTip(job["id"].toString());queue->setItem(row,0,item);
        queue->setItem(row,1,new QTableWidgetItem(job["satellite"].toString()+" · "+trText(job["mode"]=="calibrate"?"Calculate calibration":"Align channels")));
        auto *statusItem=new QTableWidgetItem(trText(queueStatusLabel(state)));statusItem->setData(Qt::UserRole,state);queue->setItem(row,2,statusItem);
    }
    bool persistQueue() {
        try {
            QJsonArray jobs;QJsonObject statuses;
            for(int row=0;row<queue->rowCount();++row) {
                const auto job=queue->item(row,0)->data(Qt::UserRole).toMap();jobs.append(QJsonObject::fromVariantMap(job));
                statuses[job["id"].toString()]=queue->item(row,2)->data(Qt::UserRole).toString();
            }
            queueStore.save({{"version",1},{"jobs",jobs},{"statuses",statuses}});return true;
        }catch(const std::exception& error) {
            queueStoreHealthy=false;log->append((it?"Coda non salvata: ":"Queue not saved: ")+QString::fromUtf8(error.what()).toHtmlEscaped());
            queueAdd->setEnabled(false);queueRemove->setEnabled(false);queueRun->setEnabled(false);return false;
        }
    }
    void restoreQueue() {
        try {
            auto document=queueStore.load();
            const auto dataDir=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            if(!QFileInfo::exists(dataDir+"/desktop-queue.json")&&QFileInfo::exists(dataDir+"/queue.json")) {
                QFile legacy(dataDir+"/queue.json");
                if(QFileInfo(legacy).isSymLink()||!legacy.open(QIODevice::ReadOnly))throw std::runtime_error("Cannot migrate previous queue");
                const auto raw=legacy.read(1024*1024+1);QJsonParseError error;const auto parsed=QJsonDocument::fromJson(raw,&error);
                if(raw.size()>1024*1024||error.error!=QJsonParseError::NoError||!parsed.isObject()||!parsed.object()["jobs"].isArray())throw std::runtime_error("Invalid previous queue");
                document["jobs"]=parsed.object()["jobs"];
                QFile state(dataDir+"/queue.state.json");QJsonObject states;
                if(!QFileInfo(state).isSymLink()&&state.open(QIODevice::ReadOnly)) {
                    const auto data=state.read(16*1024*1024+1);const auto saved=QJsonDocument::fromJson(data).object();
                    if(data.size()<=16*1024*1024&&saved["version"].toInt()==1) {
                        const auto entries=saved["jobs"].toObject();
                        for(auto entry=entries.begin();entry!=entries.end();++entry)states[entry.key()]=entry.value().toObject()["status"];
                    }
                }
                document["statuses"]=states;
            }
            const auto jobs=document["jobs"].toArray();const auto statuses=document["statuses"].toObject();
            if(jobs.size()>10000)throw std::runtime_error("Too many saved jobs");
            QSet<QString> ids;
            for(const auto& value:jobs) {
                auto job=value.toObject();const auto id=job["id"].toString();
                const QSet<QString> allowed{"id","satellite","input","output","config","mode","cpu","calibration_scale","acquisition_fingerprint"};
                for(auto key=job.begin();key!=job.end();++key)if(!allowed.contains(key.key()))throw std::runtime_error("Unknown saved job field");
                if(!QRegularExpression("^[A-Za-z0-9_.-]{1,128}$").match(id).hasMatch()||ids.contains(id)||!job["cpu"].isBool()||(job["mode"]!="generate"&&job["mode"]!="calibrate"))throw std::runtime_error("Invalid saved job");
                if(!QRegularExpression("^[A-Z0-9_]{1,32}$").match(job["satellite"].toString()).hasMatch())throw std::runtime_error("Invalid saved satellite");
                for(const auto& field:{"input","output","config"})if(!job[field].isString()||job[field].toString().isEmpty()||job[field].toString().contains(QChar(0))||job[field].toString().size()>4096)throw std::runtime_error("Invalid saved path");
                if(job.contains("calibration_scale")&&(!job["calibration_scale"].isDouble()||job["mode"]!="calibrate"||!(job["calibration_scale"].toDouble()>0&&job["calibration_scale"].toDouble()<=1)))throw std::runtime_error("Invalid saved calibration scale");
                if(job.contains("acquisition_fingerprint")&&!QRegularExpression("^[0-9a-f]{64}$").match(job["acquisition_fingerprint"].toString()).hasMatch())throw std::runtime_error("Invalid saved acquisition fingerprint");
                ids.insert(id);
            }
            for(const auto& value:jobs) {
                auto job=value.toObject();auto state=statuses[job["id"].toString()].toString("queued");
                if(state=="completed"||state=="skipped")state="needs_verification";
                if(state=="running")state="interrupted";
                appendQueueJob(job.toVariantMap(),state);
            }
        }catch(const std::exception& error){queueStoreHealthy=false;log->append((it?"Ripristino coda non riuscito: ":"Queue restore failed: ")+QString::fromUtf8(error.what()).toHtmlEscaped());}
        setQueueBusy(false);
    }
    bool addToQueue() {
        if (busy||!queueStoreHealthy||queue->rowCount()>=10000) return false;
        if (input->text().isEmpty() || config->text().isEmpty() || output->text().isEmpty()) {
            QMessageBox::warning(this, trText("Missing settings"),
                trText("Fill in input, calibration profile and a new output folder before queueing."));
            return false;
        }
        QVariantMap job;
        job["id"] = "job-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
        job["satellite"] = satellite->currentText();
        if(!importedFingerprint.isEmpty())job["acquisition_fingerprint"]=importedFingerprint;
        job["input"] = QFileInfo(input->text()).absoluteFilePath(); job["config"] = QFileInfo(config->text()).absoluteFilePath(); job["output"] = QFileInfo(output->text()).absoluteFilePath();
        job["mode"] = mode->currentIndex() == 1 ? "calibrate" : "generate";
        job["cpu"] = engine->currentIndex() == 1;
        if(mode->currentIndex()==1&&guidedScale!=1.0)job["calibration_scale"]=guidedScale;
        const int row=queue->rowCount();appendQueueJob(job);
        if(!persistQueue()){queue->removeRow(row);return false;}
        queueRun->setEnabled(!busy);return true;
    }
    void removeQueued() {
        if(busy||!queueStoreHealthy)return;
        const int row=queue->currentRow();
        if(row<0)return;
        const auto job=queue->item(row,0)->data(Qt::UserRole).toMap();const auto state=queue->item(row,2)->data(Qt::UserRole).toString();
        queue->removeRow(row);if(!persistQueue())appendQueueJob(job,state);
        for(int i=0;i<queue->rowCount();++i)queue->item(i,0)->setText(QString::number(i+1));
        queueRun->setEnabled(queueStoreHealthy&&queue->rowCount()>0&&!busy);
    }
    void setQueueBusy(bool running) {
        busy = running;
        setup->setEnabled(!running); start->setEnabled(!running);
        naturalExport->setEnabled(!running); mosaicButton->setEnabled(!running); demoButton->setEnabled(!running); simulateButton->setEnabled(!running); wizardButton->setEnabled(!running);finalImageButton->setEnabled(!running);watchButton->setEnabled(!running);updatesButton->setEnabled(!running);
        queueAdd->setEnabled(!running&&queueStoreHealthy); queueRemove->setEnabled(!running&&queueStoreHealthy);
        queueRun->setEnabled(!running && queueStoreHealthy && queue->rowCount() > 0);
    }
    void executeQueue() {
        if (busy || !queueStoreHealthy || queueProcess || queue->rowCount() == 0) return;
        const QString runner = batchRunner();
        const QString python = runner.endsWith(".py") ? QStandardPaths::findExecutable("python3") : QString();
        if (runner.isEmpty() || (runner.endsWith(".py") && python.isEmpty())) {
            QMessageBox::warning(this, trText("Processing failed"), trText("The batch runner is not available."));
            return;
        }
        QJsonArray jobs;
        for (int row = 0; row < queue->rowCount(); ++row) {
            const QVariantMap job = queue->item(row, 0)->data(Qt::UserRole).toMap();
            QJsonObject entry;
            for (const QString &key : {"id", "satellite", "input", "output", "config", "mode"})
                entry[key] = job[key].toString();
            if(job.contains("acquisition_fingerprint"))entry["acquisition_fingerprint"]=job["acquisition_fingerprint"].toString();
            entry["cpu"] = job["cpu"].toBool();
            if(job.contains("calibration_scale"))entry["calibration_scale"]=job["calibration_scale"].toDouble();
            jobs.append(entry);
        }
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dir);
        const QString manifestPath = QDir(dir).filePath("queue.json");
        const QString statePath = QDir(dir).filePath("queue.state.json");
        QJsonObject manifest; manifest["jobs"] = jobs;
        QSaveFile manifestFile(manifestPath);
        manifestFile.setDirectWriteFallback(false);
        const QByteArray payload = QJsonDocument(manifest).toJson(QJsonDocument::Compact);
        if (QFileInfo(manifestPath).isSymLink() || !manifestFile.open(QIODevice::WriteOnly) ||
            manifestFile.write(payload) != payload.size() || !manifestFile.commit()) {
            QMessageBox::warning(this, trText("Processing failed"),
                it ? "Impossibile salvare la coda." : "Queue could not be saved.");
            return;
        }
        for (int row = 0; row < queue->rowCount(); ++row)
            {queue->item(row, 2)->setText(trText("Running…"));queue->item(row,2)->setData(Qt::UserRole,"running");}
        if(!persistQueue())return;
        QStringList command;
        command << manifestPath << "--orbyx" << QDir(QCoreApplication::applicationDirPath()).filePath("orbyx")
                << "--state" << statePath << "--lang" << (it ? "it" : "en");
        if (QFileInfo::exists(statePath)) command << "--resume";
        if (!python.isEmpty()) command.prepend(runner);
        setQueueBusy(true);
        progress->setRange(0, 0); status->setText(trText("Processing…"));
        queueProcess = new QProcess(this);
        queueProcess->setProcessChannelMode(QProcess::MergedChannels);
        connect(queueProcess, &QProcess::readyReadStandardOutput, this, [this] {
            const QString text = QString::fromUtf8(queueProcess->readAllStandardOutput()).trimmed();
            if (!text.isEmpty()) log->append(text.toHtmlEscaped());
        });
        connect(queueProcess, &QProcess::finished, this, [this, statePath](int code, QProcess::ExitStatus st) {
            finishQueue(statePath, st == QProcess::NormalExit ? code : 2);
        });
        connect(queueProcess, &QProcess::errorOccurred, this, [this, statePath](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart && queueProcess) {
                log->append(queueProcess->errorString().toHtmlEscaped());
                finishQueue({}, 2);
            }
        });
        queueProcess->start(python.isEmpty() ? runner : python, command);
    }
    void finishQueue(const QString &statePath, int code) {
        // Locale-independent: read the durable state the runner persisted.
        QMap<QString, QString> statuses;
        QFile stateFile(statePath);
        if (!statePath.isEmpty() && !QFileInfo(statePath).isSymLink() && stateFile.open(QIODevice::ReadOnly)) {
            constexpr qint64 limit = 16 * 1024 * 1024;
            const auto data = stateFile.read(limit + 1);
            QJsonParseError error;
            const auto document = QJsonDocument::fromJson(data, &error);
            if (data.size() <= limit && error.error == QJsonParseError::NoError && document.isObject() &&
                document.object().value("version").toInt() == 1 && document.object().value("jobs").isObject()) {
                const auto jobs = document.object().value("jobs").toObject();
                for (auto entry = jobs.begin(); entry != jobs.end(); ++entry) {
                    const auto job = entry.value().toObject();
                    const auto attempt = job.value("last_attempt").toObject().value("status").toString();
                    statuses[entry.key()] = (attempt == "failed" || attempt == "interrupted") ? attempt : job.value("status").toString();
                }
            }
        }
        for (int row = 0; row < queue->rowCount(); ++row) {
            const QString state = statuses.value(queue->item(row,0)->data(Qt::UserRole).toMap()["id"].toString());
            const char *label = state == "completed" ? "Done" : state == "skipped" ? "Skipped"
                : "Error";
            if (state != "completed" && state != "skipped") code = 2;
            queue->item(row, 2)->setText(trText(label));queue->item(row,2)->setData(Qt::UserRole,state.isEmpty()?"failed":state);
        }
        persistQueue();setQueueBusy(false);
        progress->setRange(0, 100); progress->setValue(code == 0 ? 100 : 0);
        status->setText(trText(code == 0 ? "Completed" : "Processing failed"));
        if (queueProcess) {
            const auto remaining = QString::fromUtf8(queueProcess->readAllStandardOutput()).trimmed();
            if (!remaining.isEmpty()) log->append(remaining.toHtmlEscaped());
            queueProcess->deleteLater(); queueProcess = nullptr;
        }
    }
    void duplicateProfileDialog() {
        if (busy) return;
        bool ok = false;
        const QString from = QInputDialog::getText(this, trText("Duplicate calibration profile"),
            trText("Source satellite"), QLineEdit::Normal, satellite->currentText(), &ok).trimmed();
        if (!ok || from.isEmpty()) return;
        const QString to = QInputDialog::getText(this, trText("Duplicate calibration profile"),
            trText("New satellite"), QLineEdit::Normal, QString(), &ok).trimmed();
        if (!ok || to.isEmpty()) return;
        const QString destination = QFileDialog::getSaveFileName(this, trText("Save duplicated profile"),
            QString(), "JSON (*.json)");
        if (destination.isEmpty()) return;
        try {
            orbyx::duplicateProfile(config->text().toStdString(), from.toStdString(),
                                    to.toStdString(), destination.toStdString());
            config->setText(destination);
            if(satellite->findText(to)<0)satellite->addItem(to);satellite->setCurrentText(to);
            auto custom=settings.value("customSatellites").toStringList();custom.append(to);custom.removeDuplicates();settings.setValue("customSatellites",custom);
            QMessageBox::information(this, trText("Profile duplicated"), destination);
        } catch (const std::exception &error) {
            QMessageBox box(QMessageBox::Warning, trText("Could not duplicate the profile"),
                diagnostic(QString::fromUtf8(error.what())), QMessageBox::Ok, this);
            box.setTextFormat(Qt::PlainText); box.exec();
        }
    }
    static QString droppedFolder(const QMimeData *mime) {
        if (!mime->hasUrls() || mime->urls().size()!=1 || !mime->urls().front().isLocalFile()) return {};
        const QString path=mime->urls().front().toLocalFile();
        const QFileInfo info(path);
        return info.isDir() && !info.isSymLink() ? info.absoluteFilePath() : QString{};
    }
    void selectInput(const QString &folder) {
        importedFingerprint.clear();
        input->setText(folder);
        if (output->text().isEmpty()) output->setText(folder+"-orbyx");
        source->setCurrentIndex(0); loadPreview();
    }
protected:
    void dragEnterEvent(QDragEnterEvent *event) override {
        if (!busy && !droppedFolder(event->mimeData()).isEmpty()) event->acceptProposedAction();
        else event->ignore();
    }
    void dropEvent(QDropEvent *event) override {
        const QString folder=droppedFolder(event->mimeData());
        if (busy || folder.isEmpty()) { event->ignore(); return; }
        selectInput(folder); event->acceptProposedAction();
    }
    void closeEvent(QCloseEvent *event) override {
        if (busy) { QMessageBox::information(this,trText("Cannot close during processing"),trText("Wait for processing to finish before closing this window.")); event->ignore(); }
        else { settings.setValue("geometry",saveGeometry()); QMainWindow::closeEvent(event); }
    }
public:
    // Test seam: resolve the bundled demo folder and run a manual image
    // simulation on a chosen folder without going through the native picker.
    QString demoFolderForTests() const { return bundledDemoFolder(); }
    void runImageSimulation(const QString &folder) { executeImageDemo({},folder); }
    void previewSettings(const QString &locale,const QString &appearance,const QSize &size) {
        if (!locale.isEmpty()) { it=locale=="it"; language->setCurrentIndex(it?0:1); }
        if (!appearance.isEmpty()) { dark=appearance=="dark"; theme->setCurrentIndex(dark?0:1); }
        if (size.isValid()) resize(size);
        applyText(); applyTheme();
    }
    void loadArguments(const QStringList &args) {
        demoMode=args.contains("--demo"); applyText();
        auto value=[&](const QString &key){const int i=args.indexOf(key);return i>=0&&i+1<args.size()?args.at(i+1):QString{};};
        if (!value("--input").isEmpty()) input->setText(value("--input"));
        if (!value("--config").isEmpty()) config->setText(value("--config"));
        if (!value("--output").isEmpty()) output->setText(value("--output"));
        if (!value("--satellite").isEmpty()) {
            const int i=satellite->findText(value("--satellite"));
            if (i<0) { QTimer::singleShot(0,qApp,[]{QApplication::exit(2);}); return; }
            satellite->setCurrentIndex(i);
        }
        if (args.contains("--cpu")) engine->setCurrentIndex(1);
        if (args.contains("--calibrate")) mode->setCurrentIndex(1);
        if (!value("--input").isEmpty()) loadPreview();
        quitAfterRun=args.contains("--run-and-exit");
        if(args.contains("--updates"))QTimer::singleShot(0,updatesButton,&QPushButton::click);
        if (demoMode) {
            const auto destination=value("--output");
            QTimer::singleShot(0,this,[this,destination]{executeImageDemo(destination);});
        } else if (quitAfterRun) {
            if(args.contains("--natural-color"))QTimer::singleShot(0,this,[this]{exportNatural(input->text(),output->text());});
            else QTimer::singleShot(0,this,[this]{execute();});
        }
    }
    Window() {
        it = settings.value("language", "it").toString() == "it"; dark = settings.value("dark",true).toBool();
        setAcceptDrops(true); setWindowTitle("ORBYX"); resize(1280,860); setMinimumSize(960,680);
        auto *root = new QWidget; root->setObjectName("root"); setCentralWidget(root);
        auto *page = new QVBoxLayout(root); page->setContentsMargins(28,24,28,24); page->setSpacing(12);
        auto *headerWidget = new QWidget; headerWidget->setMinimumHeight(90); auto *header = new QHBoxLayout(headerWidget); header->setContentsMargins(0,0,0,0); auto *titles = new QVBoxLayout; titles->setSpacing(2); auto *brand = label("brand"); brand->setText("ORBYX"); subtitle=label("muted"); eyebrow=label("eyebrow");
        titles->addWidget(eyebrow); titles->addWidget(brand); titles->addWidget(subtitle); header->addLayout(titles,1);
        languageLabel=label("muted"); language=new QComboBox; language->addItems({"Italiano","English"}); language->setCurrentIndex(it?0:1);
        themeLabel=label("muted"); theme=new QComboBox; theme->addItems({"Dark","Light"}); theme->setCurrentIndex(dark?0:1);
        demoButton=new QPushButton;demoButton->setObjectName("demoButton");
        demoButton->setShortcut(QKeySequence("Ctrl+Shift+D"));header->addWidget(demoButton);
        connect(demoButton,&QPushButton::clicked,this,[this]{executeImageDemo();});
        simulateButton=new QPushButton;simulateButton->setObjectName("simulateButton");
        simulateButton->setShortcut(QKeySequence("Ctrl+Shift+S"));header->addWidget(simulateButton);
        connect(simulateButton,&QPushButton::clicked,this,[this]{imageSimulationDialog();});
        header->addWidget(languageLabel); header->addWidget(language); header->addSpacing(12); header->addWidget(themeLabel); header->addWidget(theme); page->addWidget(headerWidget);
        auto *splitter = new QSplitter; page->addWidget(splitter,1);
        auto *left = new QWidget; left->setObjectName("root"); left->setMinimumWidth(360); auto *leftLayout = new QVBoxLayout(left); leftLayout->setContentsMargins(0,0,16,0); leftLayout->setSpacing(8);
        setup=new QGroupBox; auto *form = new QVBoxLayout(setup); form->setSpacing(4); form->setContentsMargins(8,12,8,8);
        inputLabel=label(); input=new QLineEdit(settings.value("input").toString()); inputBrowse=new QPushButton; form->addWidget(inputLabel); form->addLayout(pathRow(input,inputBrowse));
        configLabel=label();
        config=new QLineEdit(settings.value("config",bundledProfile()).toString()); configBrowse=new QPushButton; form->addWidget(configLabel); form->addLayout(pathRow(config,configBrowse));
        wizardButton=new QPushButton;wizardButton->setObjectName("wizardButton");form->addWidget(wizardButton);
        connect(wizardButton,&QPushButton::clicked,this,[this]{
            CalibrationWizard wizard(it,input->text(),config->text(),output->text(),satellite->currentText(),this);
            if(wizard.exec()!=QDialog::Accepted)return;
            input->setText(wizard.inputPath());config->setText(wizard.profilePath());output->setText(wizard.outputPath());
            satellite->setCurrentText(wizard.satelliteId());guidedScale=wizard.scale();mode->setCurrentIndex(1);engine->setCurrentIndex(1);loadPreview();
        });
        watchButton=new QPushButton;watchButton->setObjectName("watchButton");form->addWidget(watchButton);
        connect(watchButton,&QPushButton::clicked,this,[this]{
            if(importWindow){importWindow->show();importWindow->raise();return;}
            importWindow=new ImportDialog(it,[this](const QJsonObject& data,bool automatic) {
                if(busy)return false;
                QString identity=data["satellite"].toString().toUpper();identity.remove(QRegularExpression("[^A-Z0-9]"));
                if(identity.startsWith("ELEKTRO"))identity.remove(0,7);
                if(automatic&&(identity.isEmpty()||identity!=satellite->currentText())){log->append(it?"Satellite non corrispondente: importazione automatica sospesa.":"Satellite mismatch: automatic import withheld.");return false;}
                if(automatic&&mode->currentIndex()==0) {
                    QFile profile(config->text());
                    if(!profile.open(QIODevice::ReadOnly)||profile.size()>1024*1024)return false;
                    const auto document=QJsonDocument::fromJson(profile.readAll()).object();
                    if(!document.contains(satellite->currentText()+"VIS1")||!document.contains(satellite->currentText()+"VIS2")) {
                        log->append(it?"Profilo del satellite assente: esegui prima la calibrazione.":"Satellite profile missing: calculate calibration first.");return false;
                    }
                }
                selectInput(data["path"].toString());importedFingerprint=data["fingerprint"].toString();
                const auto base=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/imports";QDir().mkpath(base);
                output->setText(base+"/"+QUuid::createUuid().toString(QUuid::WithoutBraces));
                if(automatic)return addToQueue();
                return true;
            },this);importWindow->setAttribute(Qt::WA_DeleteOnClose);importWindow->show();
        });
        updatesButton=new QPushButton;updatesButton->setObjectName("updatesButton");form->addWidget(updatesButton);
        connect(updatesButton,&QPushButton::clicked,this,[this]{if(updateWindow){updateWindow->show();updateWindow->raise();return;}updateWindow=new UpdateDialog(it,[this](bool running){setQueueBusy(running);},this);updateWindow->setAttribute(Qt::WA_DeleteOnClose);updateWindow->show();});
        profileDup=new QPushButton; profileDup->setObjectName("profileDup"); form->addWidget(profileDup);
        outputLabel=label(); output=new QLineEdit; outputBrowse=new QPushButton; form->addWidget(outputLabel); form->addLayout(pathRow(output,outputBrowse));
        satLabel=label(); satellite=new QComboBox; satellite->addItems({"L2","L3","L4","L5","M1","M2"});
        for(const auto& name:settings.value("customSatellites").toStringList().mid(0,128))if(QRegularExpression("^[A-Z0-9_]{1,32}$").match(name).hasMatch()&&satellite->findText(name)<0)satellite->addItem(name);
        satellite->setCurrentText(settings.value("satellite","L2").toString()); form->addWidget(satLabel); form->addWidget(satellite);
        modeLabel=label(); mode=new QComboBox; mode->addItems({"Align channels","Calculate calibration"}); form->addWidget(modeLabel); form->addWidget(mode);
        engineLabel=label(); engine=new QComboBox; engine->addItems({"Automatic CPU / OpenCL","CPU only"}); form->addWidget(engineLabel); form->addWidget(engine);
        hint=label("muted"); hint->setWordWrap(true); hint->setMinimumHeight(48); hint->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum); form->addWidget(hint); leftLayout->addWidget(setup);
        finalImageButton=new QPushButton;finalImageButton->setObjectName("finalImageButton");leftLayout->addWidget(finalImageButton);
        connect(finalImageButton,&QPushButton::clicked,this,[this]{
            FinalImageWizard wizard(it,input->text(),config->text(),output->text(),satellite->currentText(),engine->currentIndex()==1,this);
            if(wizard.exec()!=QDialog::Accepted)return;
            input->setText(wizard.inputPath());config->setText(wizard.profilePath());output->setText(wizard.outputPath());
            if(satellite->findText(wizard.satelliteId())<0)satellite->addItem(wizard.satelliteId());
            satellite->setCurrentText(wizard.satelliteId());engine->setCurrentIndex(wizard.cpuOnly()?1:0);mode->setCurrentIndex(0);
            finalNaturalColour=wizard.naturalColour();openFinalEditor=true;loadPreview();execute();
        });
        start=new QPushButton; start->setObjectName("start"); start->setShortcut(QKeySequence("Ctrl+Return")); leftLayout->addWidget(start);
        mosaicButton=new QPushButton; mosaicButton->setObjectName("mosaicButton");leftLayout->addWidget(mosaicButton);
        connect(mosaicButton,&QPushButton::clicked,this,[this]{openMosaic();});
        naturalExport=new QPushButton; naturalExport->setObjectName("naturalExport");
        naturalExport->setShortcut(QKeySequence("Ctrl+Shift+N"));leftLayout->addWidget(naturalExport);
        status=label("status"); leftLayout->addWidget(status); progress=new QProgressBar; progress->setTextVisible(false); progress->setValue(0); leftLayout->addWidget(progress);
        queueBox=new QGroupBox; auto *queueLayout=new QVBoxLayout(queueBox); queueLayout->setSpacing(6); queueLayout->setContentsMargins(8,12,8,8);
        queue=new QTableWidget(0,3); queue->verticalHeader()->setVisible(false); queue->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        queue->setSelectionBehavior(QAbstractItemView::SelectRows); queue->setSelectionMode(QAbstractItemView::SingleSelection); queue->setEditTriggers(QAbstractItemView::NoEditTriggers);
        queue->setMinimumHeight(96); queue->setMaximumHeight(170); queueLayout->addWidget(queue);
        auto *queueButtons=new QHBoxLayout; queueAdd=new QPushButton; queueRemove=new QPushButton; queueRun=new QPushButton; queueRun->setEnabled(false);
        queueButtons->addWidget(queueAdd); queueButtons->addWidget(queueRemove); queueButtons->addStretch(); queueButtons->addWidget(queueRun); queueLayout->addLayout(queueButtons);
        leftLayout->addWidget(queueBox); leftLayout->addStretch();
        auto *setupScroll = new QScrollArea; setupScroll->setObjectName("setupScroll"); setupScroll->setWidgetResizable(true); setupScroll->setFrameShape(QFrame::NoFrame); setupScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); setupScroll->setMinimumWidth(390); setupScroll->setMaximumWidth(490); setupScroll->setWidget(left); splitter->addWidget(setupScroll);
        auto *right = new QFrame; right->setObjectName("panel"); auto *preview = new QVBoxLayout(right); preview->setContentsMargins(16,16,16,16); auto *tools = new QHBoxLayout;
        previewTitle=label("section"); preview->addWidget(previewTitle); source=new QComboBox; source->addItems({"Input","Output"}); channel=new QComboBox; channel->addItems({"CH1","CH2","CH3"}); channel->setCurrentIndex(settings.value("previewChannel",0).toInt()); fit=new QPushButton; minus=new QPushButton("−"); plus=new QPushButton("+");
        plus->setShortcut(QKeySequence("Ctrl++")); minus->setShortcut(QKeySequence("Ctrl+-"));
        tools->addWidget(source); tools->addWidget(channel); tools->addStretch(); tools->addWidget(minus); tools->addWidget(plus); tools->addWidget(fit); preview->addLayout(tools);
        empty=label("muted"); empty->setAlignment(Qt::AlignCenter); empty->setWordWrap(true); preview->addWidget(empty);
        scroll=new QScrollArea; canvas=label("canvas"); canvas->setAlignment(Qt::AlignCenter); scroll->setWidget(canvas); scroll->setAlignment(Qt::AlignCenter); canvas->hide(); preview->addWidget(scroll,1);
        previewNote=label("muted"); previewNote->setWordWrap(true); preview->addWidget(previewNote); splitter->addWidget(right); splitter->setStretchFactor(1,1);
        auto *activity = new QHBoxLayout; activityTitle=label("section"); clear=new QPushButton; activity->addWidget(activityTitle,1); activity->addWidget(clear); page->addLayout(activity);
        log=new QTextEdit; log->setReadOnly(true); log->setMinimumHeight(70); log->setMaximumHeight(90); page->addWidget(log);
        connect(language,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int i){it=i==0;settings.setValue("language",it?"it":"en");applyText();});
        connect(theme,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int i){dark=i==0;settings.setValue("dark",dark);applyTheme();});
        connect(mode,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{applyText();});
        connect(satellite,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{applyText();});
        connect(inputBrowse,&QPushButton::clicked,this,[this]{const auto p=QFileDialog::getExistingDirectory(this,trText("Choose RDAS folder"),input->text());if(!p.isEmpty()) selectInput(p);});
        connect(configBrowse,&QPushButton::clicked,this,[this]{const auto p=QFileDialog::getOpenFileName(this,trText("Choose calibration profile"),config->text(),"JSON (*.json)");if(!p.isEmpty())config->setText(p);});
        connect(outputBrowse,&QPushButton::clicked,this,[this]{const auto p=QFileDialog::getExistingDirectory(this,trText("Select output parent folder"),QFileInfo(output->text()).absolutePath());if(!p.isEmpty())output->setText(QDir(p).filePath("orbyx-output"));});
        connect(input,&QLineEdit::textEdited,this,[this]{importedFingerprint.clear();});
        connect(input,&QLineEdit::editingFinished,this,[this]{loadPreview();}); connect(source,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{loadPreview();});
        connect(channel,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int i){settings.setValue("previewChannel",i);loadPreview();});
        connect(start,&QPushButton::clicked,this,[this]{execute();}); connect(clear,&QPushButton::clicked,log,&QTextEdit::clear);
        connect(naturalExport,&QPushButton::clicked,this,[this]{naturalColorDialog();});
        connect(queue,&QTableWidget::cellDoubleClicked,this,[this](int row,int){
            if(busy||row<0||row>=queue->rowCount())return;
            const auto job=queue->item(row,0)->data(Qt::UserRole).toMap();selectInput(job["input"].toString());
            importedFingerprint=job["acquisition_fingerprint"].toString();config->setText(job["config"].toString());output->setText(job["output"].toString());
            if(satellite->findText(job["satellite"].toString())<0)satellite->addItem(job["satellite"].toString());satellite->setCurrentText(job["satellite"].toString());
            mode->setCurrentIndex(job["mode"]=="calibrate"?1:0);engine->setCurrentIndex(job["cpu"].toBool()?1:0);guidedScale=job.value("calibration_scale",1.0).toDouble();
        });
        connect(queueAdd,&QPushButton::clicked,this,[this]{addToQueue();}); connect(queueRemove,&QPushButton::clicked,this,[this]{removeQueued();});
        connect(queueRun,&QPushButton::clicked,this,[this]{executeQueue();}); connect(profileDup,&QPushButton::clicked,this,[this]{duplicateProfileDialog();});
        connect(fit,&QPushButton::clicked,this,[this]{fitImage();}); connect(plus,&QPushButton::clicked,this,[this]{zoom=std::min(zoom*1.25,8.0);resizeImage();}); connect(minus,&QPushButton::clicked,this,[this]{zoom=std::max(zoom/1.25,0.02);resizeImage();});
        for (auto *w : {static_cast<QWidget*>(input),static_cast<QWidget*>(config),static_cast<QWidget*>(output),static_cast<QWidget*>(inputBrowse),static_cast<QWidget*>(configBrowse),static_cast<QWidget*>(outputBrowse),static_cast<QWidget*>(satellite),static_cast<QWidget*>(mode),static_cast<QWidget*>(engine)}) w->setFixedHeight(32);
        input->setObjectName("input"); config->setObjectName("config"); output->setObjectName("output");
        for (auto *edit : {input,config,output}) edit->setAcceptDrops(false);
        setup->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
        applyText(); applyTheme(); restoreGeometry(settings.value("geometry").toByteArray());
        restoreQueue();
        // Preview is explicit: startup never reads a previously selected acquisition.
    }
};

// Durable desktop queue: delegate sequential, resumable execution to the already
// verified offline batch runner (scripts/orbyx_batch.py / installed orbyx-batch)
// instead of re-implementing durability. No shell and no network are used: the
// runner is launched directly with an explicit argument vector.
int runQueue(const QStringList &args) {
    auto value = [&](const QString &key) -> QString {
        const int i = args.indexOf(key);
        return i >= 0 && i + 1 < args.size() ? args.at(i + 1) : QString{};
    };
    const QString manifest = value("--queue");
    if (manifest.isEmpty()) return 2;
    const QString appDir = QCoreApplication::applicationDirPath();
    QString cli = value("--orbyx");
    if (cli.isEmpty()) cli = QDir(appDir).filePath("orbyx");
    QString runner = value("--batch-runner");
    if (runner.isEmpty()) runner = QStandardPaths::findExecutable("orbyx-batch");
    if (runner.isEmpty()) {
        const QString local = QDir(appDir).filePath("orbyx-batch");
        if (QFileInfo::exists(local)) runner = local;
    }
    if (runner.isEmpty()) { qWarning("No batch runner found (orbyx-batch)"); return 2; }
    QStringList command;
    if (runner.endsWith(".py")) {
        const QString python = QStandardPaths::findExecutable("python3");
        if (python.isEmpty()) { qWarning("python3 not found for the batch runner"); return 2; }
        command << python << runner;
    } else {
        command << runner;
    }
    command << manifest << "--orbyx" << cli;
    const QString state = value("--state");
    if (!state.isEmpty()) command << "--state" << state;
    const QString lang = value("--lang");
    if (!lang.isEmpty()) command << "--lang" << lang;
    if (args.contains("--resume")) command << "--resume";
    QProcess process;
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    const QString program = command.takeFirst();
    process.start(program, command);
    if (!process.waitForStarted(10000)) return 2;
    process.waitForFinished(-1);
    return process.exitStatus() == QProcess::NormalExit ? process.exitCode() : 2;
}
}
int main(int argc, char **argv) {
    QApplication app(argc,argv); app.setApplicationName("ORBYX"); app.setApplicationVersion("0.1.0");
    const auto args=app.arguments();
    // Headless queue execution: run the durable batch and exit, no window.
    if(args.contains("--queue")) return runQueue(args);
    QImageReader::setAllocationLimit(256);
    Window window;
    auto argument=[&](const QString &name){const int i=args.indexOf(name);return i>=0&&i+1<args.size()?args.at(i+1):QString{};};
    const QStringList dimensions=argument("--size").split('x');
    const QSize size=dimensions.size()==2?QSize(dimensions[0].toInt(),dimensions[1].toInt()):QSize{};
    window.previewSettings(argument("--lang"),argument("--theme"),size);
    window.show();
    window.loadArguments(args);
    const int screenshot=args.indexOf("--screenshot");
    if (args.contains("--run-and-exit")) { /* worker completion controls the exit code */ }
    else if (screenshot>=0 && screenshot+1<args.size()) {
        const QString path=args.at(screenshot+1);
        if(args.contains("--demo")) {
            auto *capture=new QTimer(&app);capture->setInterval(50);
            auto *deadline=new QTimer(&app);deadline->setSingleShot(true);
            QObject::connect(deadline,&QTimer::timeout,&app,[&app]{app.exit(2);});deadline->start(30000);
            QObject::connect(capture,&QTimer::timeout,&app,[&app,&window,path,capture]{
                if(!window.property("demoFinished").toBool())return;
                capture->stop();
                app.exit(window.property("demoSucceeded").toBool()&&window.grab().save(path)?0:2);
            });capture->start();
        } else QTimer::singleShot(250,&app,[&app,&window,path]{if(!window.grab().save(path)) app.exit(2); else app.quit();});
    } else if (args.contains("--smoke-test")) {
        if(args.contains("--demo")) {
            auto *finished=new QTimer(&app);finished->setInterval(50);
            QObject::connect(finished,&QTimer::timeout,&app,[&app,&window]{
                if(window.property("demoFinished").toBool())app.exit(window.property("demoSucceeded").toBool()?0:2);
            });finished->start();QTimer::singleShot(30000,&app,[&app]{app.exit(2);});
        } else QTimer::singleShot(150,&app,&QApplication::quit);
    }
    return app.exec();
}
