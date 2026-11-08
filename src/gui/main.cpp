// SPDX-License-Identifier: GPL-3.0-only
// ORBYX desktop workspace. Processing lives exclusively in the shared core.
#include "orbyx/engine.hpp"

#include <QAbstractItemView>
#include <QApplication>
#include <QCloseEvent>
#include <QFile>
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
 {"L4 has no bundled calibration; calculate one before alignment.", "L4 non dispone di una calibrazione inclusa; calcolane una prima di allineare."},
 {"Zoom in", "Ingrandisci"}, {"Zoom out", "Riduci"},
 {"Processing queue", "Coda di elaborazione"},
 {"Add to queue", "Aggiungi alla coda"}, {"Remove", "Rimuovi"}, {"Run queue", "Avvia coda"},
 {"Job", "Lavoro"}, {"Status", "Stato"}, {"Queued", "In coda"}, {"Running…", "In esecuzione…"},
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
 {"Profile duplicated", "Profilo duplicato"}, {"Could not duplicate the profile", "Impossibile duplicare il profilo"}
};

class Window final : public QMainWindow {
    QSettings settings{"ORBYX", "Desktop"};
    bool busy = false;
    bool it = false;
    bool dark = true;
    bool quitAfterRun = false;
    QString lastOutput;
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
    QPushButton *queueAdd{}, *queueRemove{}, *queueRun{}, *profileDup{};
    QProcess *queueProcess{};
    int queueCounter = 0;

    QString trText(const QString &s) const { return it ? italian.value(s, s) : s; }
    QLabel *label(const QString &name = {}) { auto *w = new QLabel; w->setObjectName(name); return w; }
    QHBoxLayout *pathRow(QLineEdit *edit, QPushButton *button) {
        auto *row = new QHBoxLayout; row->setSpacing(8); row->addWidget(edit, 1); row->addWidget(button); return row;
    }
    QString diagnostic(const QString &message) const {
        return QString::fromStdString(orbyx::translate(message.toStdString(), it ? "it" : "en"));
    }
    void applyText() {
        subtitle->setText(trText("Satellite imaging workspace")); eyebrow->setText(trText("ALIGN / CALIBRATE / EXPLORE"));
        setup->setTitle(trText("Processing setup"));
        inputLabel->setText(trText("RDAS input folder")); configLabel->setText(trText("Calibration profile")); outputLabel->setText(trText("New output folder"));
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
        hint->setText(trText(satellite->currentText() == "L4" && mode->currentIndex() == 0 ? "L4 has no bundled calibration; calculate one before alignment." : mode->currentIndex() == 1 ? "Calibration writes a new calibration.json in the output folder. Merge regions remain manual." : "Alignment preserves the original input and uses the selected calibration profile."));
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
        queue->setHorizontalHeaderLabels({trText("Job"), trText("Operation"), trText("Status")});
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
    void execute() {
        if (busy) return;
        if (input->text().isEmpty() || config->text().isEmpty() || output->text().isEmpty()) {
            if (quitAfterRun) { QApplication::exit(2); return; }
            QMessageBox::warning(this, trText("Missing settings"), trText("Select an input folder, calibration profile and new output folder.")); return;
        }
        orbyx::RunOptions options;
        options.input = input->text().toStdString(); options.config = config->text().toStdString(); options.output = output->text().toStdString();
        options.language = it ? "it" : "en";
        options.satellite = satellite->currentText().toStdString(); options.cpu = engine->currentIndex() == 1; options.calibrate = mode->currentIndex() == 1;
        lastOutput = output->text(); busy = true;
        for (auto *w : {static_cast<QWidget*>(setup), static_cast<QWidget*>(start)}) w->setEnabled(false);
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
            QMetaObject::invokeMethod(this, [this,error] {
                busy = false; setup->setEnabled(true); start->setEnabled(true); progress->setRange(0,100); progress->setValue(error.isEmpty()?100:0); applyText();
                status->setText(trText(error.isEmpty()?"Completed":"Processing failed"));
                if (quitAfterRun) { QApplication::exit(error.isEmpty()?0:2); return; }
                if (!error.isEmpty()) { log->append(error.toHtmlEscaped()); QMessageBox box(QMessageBox::Warning,trText("Processing failed"),diagnostic(error),QMessageBox::Ok,this); box.setTextFormat(Qt::PlainText); box.setDetailedText(error); box.exec(); }
                else { source->setCurrentIndex(1); loadPreview(); }
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
    void addToQueue() {
        if (busy) return;
        if (input->text().isEmpty() || config->text().isEmpty() || output->text().isEmpty()) {
            QMessageBox::warning(this, trText("Missing settings"),
                trText("Fill in input, calibration profile and a new output folder before queueing."));
            return;
        }
        QVariantMap job;
        job["id"] = QString("job-%1").arg(++queueCounter);
        job["satellite"] = satellite->currentText();
        job["input"] = input->text(); job["config"] = config->text(); job["output"] = output->text();
        job["mode"] = mode->currentIndex() == 1 ? "calibrate" : "generate";
        job["cpu"] = engine->currentIndex() == 1;
        const int row = queue->rowCount();
        queue->insertRow(row);
        auto *id = new QTableWidgetItem(job["id"].toString());
        id->setData(Qt::UserRole, job);
        queue->setItem(row, 0, id);
        queue->setItem(row, 1, new QTableWidgetItem(job["satellite"].toString() + " · " +
            trText(mode->currentIndex() == 1 ? "Calculate calibration" : "Align channels")));
        queue->setItem(row, 2, new QTableWidgetItem(trText("Queued")));
        queueRun->setEnabled(!busy);
    }
    void removeQueued() {
        const int row = queue->currentRow();
        if (row >= 0) queue->removeRow(row);
        queueRun->setEnabled(queue->rowCount() > 0 && !busy);
    }
    void setQueueBusy(bool running) {
        busy = running;
        setup->setEnabled(!running); start->setEnabled(!running);
        queueAdd->setEnabled(!running); queueRemove->setEnabled(!running);
        queueRun->setEnabled(!running && queue->rowCount() > 0);
    }
    void executeQueue() {
        if (busy || queueProcess || queue->rowCount() == 0) return;
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
            entry["cpu"] = job["cpu"].toBool();
            jobs.append(entry);
            queue->item(row, 2)->setText(trText("Running…"));
        }
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dir);
        const QString manifestPath = QDir(dir).filePath("queue.json");
        const QString statePath = QDir(dir).filePath("queue.state.json");
        QJsonObject manifest; manifest["jobs"] = jobs;
        QFile manifestFile(manifestPath);
        if (!manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate) ||
            manifestFile.write(QJsonDocument(manifest).toJson(QJsonDocument::Compact)) < 0) {
            QMessageBox::warning(this, trText("Processing failed"), trText("The batch runner is not available."));
            return;
        }
        manifestFile.close();
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
        queueProcess->start(python.isEmpty() ? runner : python, command);
    }
    void finishQueue(const QString &statePath, int code) {
        // Locale-independent: read the durable state the runner persisted.
        QMap<QString, QString> statuses;
        QFile stateFile(statePath);
        if (stateFile.open(QIODevice::ReadOnly)) {
            const auto jobs = QJsonDocument::fromJson(stateFile.readAll()).object().value("jobs").toObject();
            for (auto entry = jobs.begin(); entry != jobs.end(); ++entry)
                statuses[entry.key()] = entry.value().toObject().value("status").toString();
        }
        for (int row = 0; row < queue->rowCount(); ++row) {
            const QString state = statuses.value(queue->item(row, 0)->text());
            const char *label = state == "completed" ? "Done" : state == "skipped" ? "Skipped"
                : state == "failed" ? "Error" : state.isEmpty() ? "Error" : "Done";
            queue->item(row, 2)->setText(trText(label));
        }
        setQueueBusy(false);
        progress->setRange(0, 100); progress->setValue(code == 0 ? 100 : 0);
        status->setText(trText(code == 0 ? "Completed" : "Processing failed"));
        queueProcess->deleteLater(); queueProcess = nullptr;
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
    void previewSettings(const QString &locale,const QString &appearance,const QSize &size) {
        if (!locale.isEmpty()) { it=locale=="it"; language->setCurrentIndex(it?0:1); }
        if (!appearance.isEmpty()) { dark=appearance=="dark"; theme->setCurrentIndex(dark?0:1); }
        if (size.isValid()) resize(size);
        applyText(); applyTheme();
    }
    void loadArguments(const QStringList &args) {
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
        if (quitAfterRun) QTimer::singleShot(0,this,[this]{execute();});
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
        header->addWidget(languageLabel); header->addWidget(language); header->addSpacing(12); header->addWidget(themeLabel); header->addWidget(theme); page->addWidget(headerWidget);
        auto *splitter = new QSplitter; page->addWidget(splitter,1);
        auto *left = new QWidget; left->setObjectName("root"); left->setMinimumWidth(360); auto *leftLayout = new QVBoxLayout(left); leftLayout->setContentsMargins(0,0,16,0); leftLayout->setSpacing(8);
        setup=new QGroupBox; auto *form = new QVBoxLayout(setup); form->setSpacing(4); form->setContentsMargins(8,12,8,8);
        inputLabel=label(); input=new QLineEdit(settings.value("input").toString()); inputBrowse=new QPushButton; form->addWidget(inputLabel); form->addLayout(pathRow(input,inputBrowse));
        configLabel=label();
        QString defaultConfig = QDir(QApplication::applicationDirPath()).filePath("../share/orbyx/calibration.json");
        if (!QFileInfo::exists(defaultConfig)) defaultConfig=QDir::current().filePath("resources/calibration.json");
        if (!QFileInfo::exists(defaultConfig)) defaultConfig=QDir(QApplication::applicationDirPath()).filePath("../resources/calibration.json");
        config=new QLineEdit(settings.value("config",defaultConfig).toString()); configBrowse=new QPushButton; form->addWidget(configLabel); form->addLayout(pathRow(config,configBrowse));
        profileDup=new QPushButton; profileDup->setObjectName("profileDup"); form->addWidget(profileDup);
        outputLabel=label(); output=new QLineEdit; outputBrowse=new QPushButton; form->addWidget(outputLabel); form->addLayout(pathRow(output,outputBrowse));
        satLabel=label(); satellite=new QComboBox; satellite->addItems({"L2","L3","L4","L5","M1","M2"}); satellite->setCurrentText(settings.value("satellite","L2").toString()); form->addWidget(satLabel); form->addWidget(satellite);
        modeLabel=label(); mode=new QComboBox; mode->addItems({"Align channels","Calculate calibration"}); form->addWidget(modeLabel); form->addWidget(mode);
        engineLabel=label(); engine=new QComboBox; engine->addItems({"Automatic CPU / OpenCL","CPU only"}); form->addWidget(engineLabel); form->addWidget(engine);
        hint=label("muted"); hint->setWordWrap(true); hint->setMinimumHeight(48); hint->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum); form->addWidget(hint); leftLayout->addWidget(setup);
        start=new QPushButton; start->setObjectName("start"); start->setShortcut(QKeySequence("Ctrl+Return")); leftLayout->addWidget(start);
        status=label("muted"); leftLayout->addWidget(status); progress=new QProgressBar; progress->setTextVisible(false); progress->setValue(0); leftLayout->addWidget(progress);
        queueBox=new QGroupBox; auto *queueLayout=new QVBoxLayout(queueBox); queueLayout->setSpacing(6); queueLayout->setContentsMargins(8,12,8,8);
        queue=new QTableWidget(0,3); queue->verticalHeader()->setVisible(false); queue->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        queue->setSelectionBehavior(QAbstractItemView::SelectRows); queue->setSelectionMode(QAbstractItemView::SingleSelection); queue->setEditTriggers(QAbstractItemView::NoEditTriggers);
        queue->setMinimumHeight(96); queue->setMaximumHeight(170); queueLayout->addWidget(queue);
        auto *queueButtons=new QHBoxLayout; queueAdd=new QPushButton; queueRemove=new QPushButton; queueRun=new QPushButton; queueRun->setEnabled(false);
        queueButtons->addWidget(queueAdd); queueButtons->addWidget(queueRemove); queueButtons->addStretch(); queueButtons->addWidget(queueRun); queueLayout->addLayout(queueButtons);
        leftLayout->addWidget(queueBox); leftLayout->addStretch();
        auto *setupScroll = new QScrollArea; setupScroll->setWidgetResizable(true); setupScroll->setFrameShape(QFrame::NoFrame); setupScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); setupScroll->setMinimumWidth(390); setupScroll->setMaximumWidth(490); setupScroll->setWidget(left); splitter->addWidget(setupScroll);
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
        connect(input,&QLineEdit::editingFinished,this,[this]{loadPreview();}); connect(source,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{loadPreview();});
        connect(channel,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int i){settings.setValue("previewChannel",i);loadPreview();});
        connect(start,&QPushButton::clicked,this,[this]{execute();}); connect(clear,&QPushButton::clicked,log,&QTextEdit::clear);
        connect(queueAdd,&QPushButton::clicked,this,[this]{addToQueue();}); connect(queueRemove,&QPushButton::clicked,this,[this]{removeQueued();});
        connect(queueRun,&QPushButton::clicked,this,[this]{executeQueue();}); connect(profileDup,&QPushButton::clicked,this,[this]{duplicateProfileDialog();});
        connect(fit,&QPushButton::clicked,this,[this]{fitImage();}); connect(plus,&QPushButton::clicked,this,[this]{zoom=std::min(zoom*1.25,8.0);resizeImage();}); connect(minus,&QPushButton::clicked,this,[this]{zoom=std::max(zoom/1.25,0.02);resizeImage();});
        for (auto *w : {static_cast<QWidget*>(input),static_cast<QWidget*>(config),static_cast<QWidget*>(output),static_cast<QWidget*>(inputBrowse),static_cast<QWidget*>(configBrowse),static_cast<QWidget*>(outputBrowse),static_cast<QWidget*>(satellite),static_cast<QWidget*>(mode),static_cast<QWidget*>(engine)}) w->setFixedHeight(32);
        input->setObjectName("input"); config->setObjectName("config"); output->setObjectName("output");
        for (auto *edit : {input,config,output}) edit->setAcceptDrops(false);
        setup->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
        applyText(); applyTheme(); restoreGeometry(settings.value("geometry").toByteArray());
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
        QTimer::singleShot(250,&app,[&app,&window,path]{if(!window.grab().save(path)) app.exit(2); else app.quit();});
    } else if (args.contains("--smoke-test")) QTimer::singleShot(150,&app,&QApplication::quit);
    return app.exec();
}
