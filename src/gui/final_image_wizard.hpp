// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QWizard>
#include <QWizardPage>

class FinalReviewPage final : public QWizardPage {
public:
    void requireConfirmation(QCheckBox *box) { registerField("finalConfirmed*", box); }
};

// A beginner-facing hand-off to the existing, verified processing and
// compositor paths. It prepares settings only; the main window owns execution.
class FinalImageWizard final : public QWizard {
    bool italian_{};
    QLineEdit *input_{}, *profile_{}, *output_{};
    QComboBox *satellite_{}, *engine_{}, *colour_{};
    QLabel *joinSummary_{}, *reviewSummary_{}, *importSummary_{};
    QCheckBox *confirmed_{}, *queueChoice_{};
    QString projectFile_; // Autosaved resumable wizard state (M2-08).

    QString text(const char *en, const char *it) const {
        return QString::fromUtf8(italian_ ? it : en);
    }
    void addPath(QVBoxLayout *layout, QLineEdit *edit, const QString &title,
                 bool file, bool destination) {
        auto *label = new QLabel(title);
        label->setBuddy(edit);
        edit->setAccessibleName(title);
        layout->addWidget(label);
        layout->addWidget(edit);
        auto *choose = new QPushButton(text("Choose…", "Seleziona…"));
        choose->setAccessibleName(title);
        layout->addWidget(choose);
        connect(choose, &QPushButton::clicked, this, [=] {
            QString selected = file
                ? QFileDialog::getOpenFileName(this, title, edit->text(), "JSON (*.json)")
                : QFileDialog::getExistingDirectory(this, title,
                      destination ? QFileInfo(edit->text()).absolutePath() : edit->text());
            if (!selected.isEmpty())
                edit->setText(destination ? QDir(selected).filePath("orbyx-final") : selected);
        });
    }
    bool profileHasRoi() const {
        QFile file(profile_->text());
        if (!file.open(QIODevice::ReadOnly) || file.size() > 1024 * 1024) return false;
        const auto root = QJsonDocument::fromJson(file.read(1024 * 1024 + 1)).object();
        for (const QString suffix : {"VIS1", "VIS2"}) {
            const auto roi = root.value(satellite_->currentText() + suffix).toObject()
                                 .value("ROI").toObject();
            if (roi.value("x").toInt(-1) < 0 || roi.value("y").toInt(-1) < 0 ||
                roi.value("width").toInt() <= 0 || roi.value("height").toInt() <= 0)
                return false;
        }
        return true;
    }
    void updateSummary() {
        const bool roi = profileHasRoi();
        joinSummary_->setText(roi
            ? text("A complete profile ROI was found. ORBYX will preserve the historical merge and open the same two crops as editable layers. You can still request visual alignment or move them manually.",
                   "È stata trovata una coppia ROI completa. ORBYX conserverà la fusione storica e aprirà gli stessi due ritagli come livelli modificabili. Potrai comunque richiedere l’allineamento visuale o spostarli manualmente.")
            : text("No complete profile ROI was found. ORBYX will open the aligned portions in full for visual or manual placement.",
                   "Non è stata trovata una coppia ROI completa. ORBYX aprirà le porzioni allineate intere per il posizionamento visuale o manuale."));
        reviewSummary_->setText(text("ORBYX will align the channels into a new folder, create the selected colour view and open the editable compositor. Originals remain unchanged. Export the final PNG from the compositor after checking every seam.",
                                            "ORBYX allineerà i canali in una nuova cartella, creerà la resa colore scelta e aprirà il compositore modificabile. Gli originali resteranno invariati. Esporta il PNG finale dal compositore dopo aver controllato ogni giunzione."));
    }
    // M2-08: recognize what the import folder contains without depending on SX/DX
    // names — any MSUGS_VIS* product (three channels) and already-aligned products.
    // This only reports what was found; it never modifies the SatDump export.
    void detectProducts() {
        if (!importSummary_) return;
        int vis = 0, aligned = 0;
        const QDir root(input_->text());
        if (!input_->text().trimmed().isEmpty() && root.exists()) {
            const auto hasThreeChannels = [](const QDir &dir) {
                for (int channel = 1; channel <= 3; ++channel) {
                    const QFileInfo image(dir.filePath(QString("MSUGS-VIS-%1.png").arg(channel)));
                    if (!image.isFile() || image.isSymLink() || image.size() <= 0) return false;
                }
                return true;
            };
            const auto names = root.entryList(QStringList() << "MSUGS_VIS*", QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString &name : names) {
                const QFileInfo info(root.filePath(name));
                if (info.isSymLink() || !hasThreeChannels(QDir(info.absoluteFilePath()))) continue;
                if (name.contains("Aligned")) ++aligned; else ++vis;
                if (vis + aligned >= 64) break;
            }
        }
        setProperty("detectedVisProducts", vis);
        setProperty("detectedAlignedProducts", aligned);
        importSummary_->setText(vis == 0 && aligned == 0
            ? text("No MSU-GS VIS products detected yet. Choose a SatDump export folder.",
                   "Nessun prodotto MSU-GS VIS rilevato. Scegli una cartella esportata da SatDump.")
            : text("Detected %1 VIS product(s) and %2 already-aligned product(s). Grouping does not depend on SX/DX names.",
                   "Rilevati %1 prodotti VIS e %2 prodotti già allineati. Il raggruppamento non dipende dai nomi SX/DX.").arg(vis).arg(aligned));
    }

public:
    FinalImageWizard(bool italian, const QString &input, const QString &profile,
                     const QString &output, const QString &satellite,
                     bool cpu, QWidget *parent = nullptr)
        : QWizard(parent), italian_(italian) {
        setObjectName("finalImageWizard");
        setWindowTitle(text("Create final image", "Crea immagine finale"));
        setMinimumSize(660, 560);
        setButtonText(NextButton, text("Next", "Avanti"));
        setButtonText(BackButton, text("Back", "Indietro"));
        setButtonText(CancelButton, text("Cancel", "Annulla"));
        setButtonText(FinishButton, text("Align and open editor", "Allinea e apri l’editor"));

        auto *importPage = new QWizardPage;
        importPage->setTitle(text("1. Import", "1. Importa"));
        auto *importLayout = new QVBoxLayout(importPage);
        input_ = new QLineEdit(input); input_->setObjectName("finalInput");
        profile_ = new QLineEdit(profile); profile_->setObjectName("finalProfile");
        output_ = new QLineEdit(output); output_->setObjectName("finalOutput");
        addPath(importLayout, input_, text("SatDump export folder", "Cartella esportata da SatDump"), false, false);
        addPath(importLayout, profile_, text("Calibration profile", "Profilo di calibrazione"), true, false);
        addPath(importLayout, output_, text("New working folder", "Nuova cartella di lavoro"), false, true);
        importSummary_ = new QLabel; importSummary_->setObjectName("finalImportSummary");
        importSummary_->setWordWrap(true); importLayout->addWidget(importSummary_); importLayout->addStretch();
        addPage(importPage);

        auto *alignPage = new QWizardPage;
        alignPage->setTitle(text("2. Align channels", "2. Allinea canali"));
        auto *alignLayout = new QVBoxLayout(alignPage);
        auto *satelliteLabel = new QLabel(text("Satellite", "Satellite"));
        satellite_ = new QComboBox; satellite_->setObjectName("finalSatellite");
        satellite_->addItems({"L2", "L3", "L4", "L5", "M1", "M2"});
        if (satellite_->findText(satellite) < 0) satellite_->addItem(satellite);
        satellite_->setCurrentText(satellite); satelliteLabel->setBuddy(satellite_);
        alignLayout->addWidget(satelliteLabel); alignLayout->addWidget(satellite_);
        auto *engineLabel = new QLabel(text("Processing engine", "Motore di elaborazione"));
        engine_ = new QComboBox; engine_->setObjectName("finalEngine");
        engine_->addItems({text("Automatic CPU / OpenCL", "CPU / OpenCL automatico"),
                           text("CPU only", "Solo CPU")});
        engine_->setCurrentIndex(cpu ? 1 : 0); engineLabel->setBuddy(engine_);
        alignLayout->addWidget(engineLabel); alignLayout->addWidget(engine_);
        auto *alignNote = new QLabel(text("The three channels of each detected VIS product are aligned without modifying the SatDump export.",
                                              "I tre canali di ogni prodotto VIS rilevato vengono allineati senza modificare l’esportazione SatDump."));
        alignNote->setWordWrap(true); alignLayout->addWidget(alignNote); alignLayout->addStretch();
        addPage(alignPage);

        auto *joinPage = new QWizardPage;
        joinPage->setTitle(text("3. Join portions", "3. Unisci porzioni"));
        auto *joinLayout = new QVBoxLayout(joinPage);
        joinSummary_ = new QLabel; joinSummary_->setObjectName("finalJoinSummary");
        joinSummary_->setWordWrap(true); joinLayout->addWidget(joinSummary_); joinLayout->addStretch();
        addPage(joinPage);

        auto *colourPage = new QWizardPage;
        colourPage->setTitle(text("4. Colour", "4. Colore"));
        auto *colourLayout = new QVBoxLayout(colourPage);
        auto *colourLabel = new QLabel(text("Initial colour rendering", "Resa colore iniziale"));
        colour_ = new QComboBox; colour_->setObjectName("finalColour");
        colour_->addItems({"MSU-GS Natural Color (RGB321)", "RGB123"});
        colourLabel->setBuddy(colour_); colourLayout->addWidget(colourLabel); colourLayout->addWidget(colour_);
        auto *colourNote = new QLabel(text("Colour controls remain editable per layer. This is a visual rendering, not certified radiometric calibration.",
                                               "I controlli colore restano modificabili per ogni livello. È una resa visuale, non una calibrazione radiometrica certificata."));
        colourNote->setWordWrap(true); colourLayout->addWidget(colourNote); colourLayout->addStretch();
        addPage(colourPage);

        auto *exportPage = new FinalReviewPage;
        exportPage->setTitle(text("5. Export", "5. Esporta"));
        auto *exportLayout = new QVBoxLayout(exportPage);
        reviewSummary_ = new QLabel; reviewSummary_->setObjectName("finalReviewSummary");
        reviewSummary_->setWordWrap(true); exportLayout->addWidget(reviewSummary_);
        confirmed_ = new QCheckBox(text("I checked the satellite and destination and will review the seams before export.",
                                             "Ho controllato satellite e destinazione e verificherò le giunzioni prima dell’esportazione."));
        confirmed_->setObjectName("finalConfirm"); exportLayout->addWidget(confirmed_);
        // M2-08: optionally enqueue the guided job on the durable queue instead of
        // running it immediately, so the beginner flow is resumable like the batch.
        queueChoice_ = new QCheckBox(text("Add this job to the durable queue instead of running it now.",
                                             "Aggiungi questo lavoro alla coda durevole invece di avviarlo subito."));
        queueChoice_->setObjectName("finalQueue"); exportLayout->addWidget(queueChoice_);
        exportPage->requireConfirmation(confirmed_); exportLayout->addStretch(); addPage(exportPage);

        connect(this, &QWizard::currentIdChanged, this, [this] { updateSummary(); });
        connect(satellite_, &QComboBox::currentTextChanged, this, [this] { updateSummary(); });
        updateSummary();

        // M2-08: persist the wizard state so the guided flow can be resumed. The
        // initial values were set before these connections, so constructing a wizard
        // never overwrites a saved session; only later edits and page changes do.
        projectFile_ = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/final-wizard.json";
        for (auto *edit : {input_, profile_, output_})
            connect(edit, &QLineEdit::textChanged, this, [this] { autosave(); });
        for (auto *combo : {satellite_, engine_, colour_})
            connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] { autosave(); });
        connect(queueChoice_, &QCheckBox::toggled, this, [this] { autosave(); });
        connect(this, &QWizard::currentIdChanged, this, [this] { autosave(); });

        // Refresh the import detection when the folder changes or the page is shown.
        connect(input_, &QLineEdit::textChanged, this, [this] { detectProducts(); });
        connect(this, &QWizard::currentIdChanged, this, [this] { detectProducts(); });
        detectProducts();
    }

    // Serialized, resumable wizard state. The main window owns execution; this only
    // records the chosen inputs, options and the reached step.
    QString wizardProject() const {
        QJsonObject root;
        root["schema"] = "orbyx-final-wizard/1";
        root["input"] = input_->text(); root["profile"] = profile_->text(); root["output"] = output_->text();
        root["satellite"] = satellite_->currentText(); root["cpu"] = engine_->currentIndex() == 1;
        root["natural"] = colour_->currentIndex() == 0; root["step"] = currentId();
        root["queue"] = queueChoice_->isChecked();
        return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
    }
    bool applyWizardProject(const QString &json) {
        const auto root = QJsonDocument::fromJson(json.toUtf8()).object();
        if (root.value("schema").toString() != "orbyx-final-wizard/1") return false;
        input_->setText(root.value("input").toString());
        profile_->setText(root.value("profile").toString());
        output_->setText(root.value("output").toString());
        const QString sat = root.value("satellite").toString();
        if (!sat.isEmpty()) { if (satellite_->findText(sat) < 0) satellite_->addItem(sat); satellite_->setCurrentText(sat); }
        engine_->setCurrentIndex(root.value("cpu").toBool() ? 1 : 0);
        colour_->setCurrentIndex(root.value("natural").toBool(true) ? 0 : 1);
        queueChoice_->setChecked(root.value("queue").toBool());
        setProperty("resumedStep", root.value("step").toInt());
        updateSummary();
        return true;
    }
    void saveProject() const { autosave(); }
    bool hasSavedProject() const {
        QFile file(projectFile_);
        if (!file.open(QIODevice::ReadOnly) || file.size() > 65536) return false;
        return QJsonDocument::fromJson(file.readAll()).object().value("schema").toString() == "orbyx-final-wizard/1";
    }
    bool resumeFromSaved() {
        QFile file(projectFile_);
        if (!file.open(QIODevice::ReadOnly) || file.size() > 65536) return false;
        return applyWizardProject(QString::fromUtf8(file.readAll()));
    }

private:
    void autosave() const {
        if (projectFile_.isEmpty()) return;
        QDir().mkpath(QFileInfo(projectFile_).absolutePath());
        QFile file(projectFile_);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) file.write(wizardProject().toUtf8());
    }

public:

    bool validateCurrentPage() override {
        if (currentId() == 0) {
            QString error;
            const QFileInfo in(input_->text()), profile(profile_->text()), out(output_->text());
            if (!in.isDir() || in.isSymLink())
                error = text("Choose a local SatDump export folder.", "Scegli una cartella locale esportata da SatDump.");
            for (const QString vis : {"MSUGS_VIS1", "MSUGS_VIS2"}) {
                const QFileInfo folder(QDir(input_->text()).filePath(vis));
                if (!folder.isDir() || folder.isSymLink()) error = text("The SatDump export is incomplete.", "L’esportazione SatDump è incompleta.");
                for (int channel = 1; channel <= 3; ++channel) {
                    const QFileInfo image(QDir(folder.absoluteFilePath()).filePath(QString("MSUGS-VIS-%1.png").arg(channel)));
                    if (!image.isFile() || image.isSymLink() || image.size() <= 0)
                        error = text("The SatDump export is incomplete.", "L’esportazione SatDump è incompleta.");
                }
            }
            QFile json(profile.absoluteFilePath());
            if (!profile.isFile() || profile.isSymLink() || !json.open(QIODevice::ReadOnly) ||
                json.size() > 1024 * 1024 || !QJsonDocument::fromJson(json.read(1024 * 1024 + 1)).isObject())
                error = text("Choose a valid JSON calibration profile.", "Scegli un profilo JSON di calibrazione valido.");
            const QString destination = QDir::cleanPath(out.absoluteFilePath());
            const QString origin = QDir::cleanPath(in.canonicalFilePath());
            if (output_->text().trimmed().isEmpty() || out.exists() || out.isSymLink() ||
                !QFileInfo(out.absolutePath()).isDir() || destination == origin || destination.startsWith(origin + "/"))
                error = text("Choose a new working folder outside the input.", "Scegli una nuova cartella di lavoro esterna all’origine.");
            if (!error.isEmpty()) {
                setProperty("validationError", error); currentPage()->setSubTitle(error); return false;
            }
            setProperty("validationError", {}); currentPage()->setSubTitle({});
        }
        return QWizard::validateCurrentPage();
    }

    QString inputPath() const { return input_->text(); }
    QString profilePath() const { return profile_->text(); }
    QString outputPath() const { return output_->text(); }
    QString satelliteId() const { return satellite_->currentText(); }
    bool cpuOnly() const { return engine_->currentIndex() == 1; }
    bool naturalColour() const { return colour_->currentIndex() == 0; }
    bool queueInsteadOfRun() const { return queueChoice_->isChecked(); }
};
