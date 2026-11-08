// SPDX-License-Identifier: GPL-3.0-only
// ORBYX desktop workspace. Processing lives exclusively in the shared core.
#include "orbyx/engine.hpp"

#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMap>
#include <QMessageBox>
#include <QPixmap>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSplitter>
#include <QStandardPaths>
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
 {"Zoom in", "Ingrandisci"}, {"Zoom out", "Riduci"}
};

class Window final : public QMainWindow {
    QSettings settings{"ORBYX", "Desktop"};
    bool busy = false;
    bool it = false;
    bool dark = true;
    QString lastOutput;
    QPixmap image;
    double zoom = 1.0;
    QLabel *subtitle{}, *eyebrow{}, *inputLabel{}, *configLabel{}, *outputLabel{}, *satLabel{}, *modeLabel{}, *engineLabel{}, *hint{}, *previewTitle{}, *empty{}, *status{}, *activityTitle{}, *languageLabel{}, *themeLabel{}, *previewNote{};
    QGroupBox *setup{};
    QLineEdit *input{}, *config{}, *output{};
    QPushButton *inputBrowse{}, *configBrowse{}, *outputBrowse{}, *start{}, *fit{}, *plus{}, *minus{}, *clear{};
    QComboBox *satellite{}, *mode{}, *engine{}, *language{}, *theme{}, *source{};
    QProgressBar *progress{};
    QScrollArea *scroll{};
    QLabel *canvas{};
    QTextEdit *log{};

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
        previewTitle->setText(trText("Image preview")); empty->setText(trText("Choose a folder to preview its first channel. Scroll to pan; use + / − to zoom."));
        previewNote->setText(trText("Full-resolution exports are unchanged; preview is limited to 2048 pixels."));
        fit->setText(trText("Fit")); plus->setAccessibleName(trText("Zoom in")); minus->setAccessibleName(trText("Zoom out"));
        activityTitle->setText(trText("Activity")); clear->setText(trText("Clear")); languageLabel->setText(trText("Language")); themeLabel->setText(trText("Theme"));
        log->setAccessibleName(trText("Technical diagnostics from the processing engine"));
        satellite->setAccessibleName(trText("Satellite")); mode->setAccessibleName(trText("Operation")); engine->setAccessibleName(trText("Processing engine"));
        language->setAccessibleName(trText("Language")); theme->setAccessibleName(trText("Theme")); source->setAccessibleName(trText("Image preview"));
        inputLabel->setBuddy(input); configLabel->setBuddy(config); outputLabel->setBuddy(output); satLabel->setBuddy(satellite); modeLabel->setBuddy(mode); engineLabel->setBuddy(engine);
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
            "QPushButton#primary {background:#46cabc;color:#062c2c;border:0;font-weight:700;padding:13px;}"
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
            const QStringList paths = {"MSUGS_VIS_Aligned/MSUGS-VIS-1.png", "MSUGS_VIS-1_Aligned/MSUGS-VIS-1.png", "MSUGS_VIS1/MSUGS-VIS-1.png", "MSUGS-VIS-1.png"};
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
            QMessageBox::warning(this, trText("Missing settings"), trText("Select an input folder, calibration profile and new output folder.")); return;
        }
        orbyx::RunOptions options;
        options.input = input->text().toStdString(); options.config = config->text().toStdString(); options.output = output->text().toStdString();
        options.language = it ? "it" : "en";
        options.satellite = satellite->currentText().toStdString(); options.cpu = engine->currentIndex() == 1; options.calibrate = mode->currentIndex() == 1;
        lastOutput = output->text(); busy = true;
        for (auto *w : {static_cast<QWidget*>(setup), static_cast<QWidget*>(start)}) w->setEnabled(false);
        progress->setRange(0,0); status->setText(trText("Processing…")); applyText();
        settings.setValue("input", input->text()); settings.setValue("config", config->text()); settings.setValue("satellite", satellite->currentText());
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
                if (!error.isEmpty()) { log->append(error.toHtmlEscaped()); QMessageBox box(QMessageBox::Warning,trText("Processing failed"),diagnostic(error),QMessageBox::Ok,this); box.setTextFormat(Qt::PlainText); box.setDetailedText(error); box.exec(); }
                else { source->setCurrentIndex(1); loadPreview(); }
            }, Qt::QueuedConnection);
        });
        connect(worker,&QThread::finished,worker,&QObject::deleteLater); worker->start();
    }
protected:
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
    Window() {
        it = settings.value("language", "it").toString() == "it"; dark = settings.value("dark",true).toBool();
        setWindowTitle("ORBYX"); resize(1280,860); setMinimumSize(960,680);
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
        outputLabel=label(); output=new QLineEdit; outputBrowse=new QPushButton; form->addWidget(outputLabel); form->addLayout(pathRow(output,outputBrowse));
        satLabel=label(); satellite=new QComboBox; satellite->addItems({"L2","L3","L4","L5","M1","M2"}); satellite->setCurrentText(settings.value("satellite","L2").toString()); form->addWidget(satLabel); form->addWidget(satellite);
        modeLabel=label(); mode=new QComboBox; mode->addItems({"Align channels","Calculate calibration"}); form->addWidget(modeLabel); form->addWidget(mode);
        engineLabel=label(); engine=new QComboBox; engine->addItems({"Automatic CPU / OpenCL","CPU only"}); form->addWidget(engineLabel); form->addWidget(engine);
        hint=label("muted"); hint->setWordWrap(true); hint->setMinimumHeight(48); hint->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum); form->addWidget(hint); leftLayout->addWidget(setup);
        start=new QPushButton; start->setObjectName("primary"); start->setShortcut(QKeySequence("Ctrl+Return")); leftLayout->addWidget(start);
        status=label("muted"); leftLayout->addWidget(status); progress=new QProgressBar; progress->setTextVisible(false); progress->setValue(0); leftLayout->addWidget(progress); leftLayout->addStretch();
        auto *setupScroll = new QScrollArea; setupScroll->setWidgetResizable(true); setupScroll->setFrameShape(QFrame::NoFrame); setupScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); setupScroll->setMinimumWidth(390); setupScroll->setMaximumWidth(490); setupScroll->setWidget(left); splitter->addWidget(setupScroll);
        auto *right = new QFrame; right->setObjectName("panel"); auto *preview = new QVBoxLayout(right); preview->setContentsMargins(16,16,16,16); auto *tools = new QHBoxLayout;
        previewTitle=label("section"); tools->addWidget(previewTitle,1); source=new QComboBox; source->addItems({"Input","Output"}); fit=new QPushButton; minus=new QPushButton("−"); plus=new QPushButton("+");
        plus->setShortcut(QKeySequence("Ctrl++")); minus->setShortcut(QKeySequence("Ctrl+-"));
        tools->addWidget(source); tools->addWidget(minus); tools->addWidget(plus); tools->addWidget(fit); preview->addLayout(tools);
        empty=label("muted"); empty->setAlignment(Qt::AlignCenter); empty->setWordWrap(true); preview->addWidget(empty);
        scroll=new QScrollArea; canvas=label("canvas"); canvas->setAlignment(Qt::AlignCenter); scroll->setWidget(canvas); scroll->setAlignment(Qt::AlignCenter); canvas->hide(); preview->addWidget(scroll,1);
        previewNote=label("muted"); previewNote->setWordWrap(true); preview->addWidget(previewNote); splitter->addWidget(right); splitter->setStretchFactor(1,1);
        auto *activity = new QHBoxLayout; activityTitle=label("section"); clear=new QPushButton; activity->addWidget(activityTitle,1); activity->addWidget(clear); page->addLayout(activity);
        log=new QTextEdit; log->setReadOnly(true); log->setMinimumHeight(70); log->setMaximumHeight(90); page->addWidget(log);
        connect(language,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int i){it=i==0;settings.setValue("language",it?"it":"en");applyText();});
        connect(theme,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int i){dark=i==0;settings.setValue("dark",dark);applyTheme();});
        connect(mode,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{applyText();});
        connect(satellite,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{applyText();});
        connect(inputBrowse,&QPushButton::clicked,this,[this]{const auto p=QFileDialog::getExistingDirectory(this,trText("Choose RDAS folder"),input->text());if(!p.isEmpty()){input->setText(p); if(output->text().isEmpty())output->setText(p+"-orbyx");loadPreview();}});
        connect(configBrowse,&QPushButton::clicked,this,[this]{const auto p=QFileDialog::getOpenFileName(this,trText("Choose calibration profile"),config->text(),"JSON (*.json)");if(!p.isEmpty())config->setText(p);});
        connect(outputBrowse,&QPushButton::clicked,this,[this]{const auto p=QFileDialog::getExistingDirectory(this,trText("Select output parent folder"),QFileInfo(output->text()).absolutePath());if(!p.isEmpty())output->setText(QDir(p).filePath("orbyx-output"));});
        connect(input,&QLineEdit::editingFinished,this,[this]{loadPreview();}); connect(source,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{loadPreview();});
        connect(start,&QPushButton::clicked,this,[this]{execute();}); connect(clear,&QPushButton::clicked,log,&QTextEdit::clear);
        connect(fit,&QPushButton::clicked,this,[this]{fitImage();}); connect(plus,&QPushButton::clicked,this,[this]{zoom=std::min(zoom*1.25,8.0);resizeImage();}); connect(minus,&QPushButton::clicked,this,[this]{zoom=std::max(zoom/1.25,0.02);resizeImage();});
        for (auto *w : {static_cast<QWidget*>(input),static_cast<QWidget*>(config),static_cast<QWidget*>(output),static_cast<QWidget*>(inputBrowse),static_cast<QWidget*>(configBrowse),static_cast<QWidget*>(outputBrowse),static_cast<QWidget*>(satellite),static_cast<QWidget*>(mode),static_cast<QWidget*>(engine)}) w->setFixedHeight(32);
        setup->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
        applyText(); applyTheme(); restoreGeometry(settings.value("geometry").toByteArray());
        // Preview is explicit: startup never reads a previously selected acquisition.
    }
};
}
int main(int argc, char **argv) {
    QApplication app(argc,argv); app.setApplicationName("ORBYX"); app.setApplicationVersion("0.1.0");
    QImageReader::setAllocationLimit(256);
    Window window;
    const auto args=app.arguments();
    auto argument=[&](const QString &name){const int i=args.indexOf(name);return i>=0&&i+1<args.size()?args.at(i+1):QString{};};
    const QStringList dimensions=argument("--size").split('x');
    const QSize size=dimensions.size()==2?QSize(dimensions[0].toInt(),dimensions[1].toInt()):QSize{};
    window.previewSettings(argument("--lang"),argument("--theme"),size);
    window.show();
    const int screenshot=args.indexOf("--screenshot");
    if (screenshot>=0 && screenshot+1<args.size()) {
        const QString path=args.at(screenshot+1);
        QTimer::singleShot(250,&app,[&app,&window,path]{if(!window.grab().save(path)) app.exit(2); else app.quit();});
    } else if (args.contains("--smoke-test")) QTimer::singleShot(150,&app,&QApplication::quit);
    return app.exec();
}
