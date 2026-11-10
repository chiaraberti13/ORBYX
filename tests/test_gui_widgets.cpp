// SPDX-License-Identifier: GPL-3.0-only
// Exercise the actual production widgets, including QProcess failure recovery.
#define main orbyx_desktop_entry
#include "../src/gui/main.cpp"
#undef main
#include <QTest>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QCryptographicHash>
#include <QDialogButtonBox>
#include <QPointer>
#include <opencv2/imgcodecs.hpp>
#include <iostream>
#include <stdexcept>

void requireTest(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
template<class F> bool until(F predicate, int timeout = 15000) {
    QElapsedTimer timer; timer.start();
    while (!predicate() && timer.elapsed() < timeout) QTest::qWait(10);
    return predicate();
}
void clickVisible(QPushButton *button) {
    requireTest(button,"Button missing");
    QTest::qWait(20);
    for(auto *parent=button->parentWidget();parent;parent=parent->parentWidget())
        if(auto *area=qobject_cast<QScrollArea*>(parent))area->ensureWidgetVisible(button,5,5);
    QTest::qWait(10);
    if(!button->isVisible()||!button->visibleRegion().contains(button->rect().center()))
        std::cerr<<"Clipped button: "<<button->objectName().toStdString()<<std::endl;
    requireTest(button->isVisible()&&button->visibleRegion().contains(button->rect().center()),"Button clipped outside the visible viewport");
    QTest::mouseClick(button,Qt::LeftButton);
}
QByteArray hashFile(const QString &path) {
    QFile file(path); requireTest(file.open(QIODevice::ReadOnly), "Output missing");
    return QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256);
}
int main(int argc, char **argv) {
    QTemporaryDir temporary;
    requireTest(temporary.isValid(), "Temporary directory unavailable");
    qputenv("XDG_CONFIG_HOME", (temporary.path()+"/config").toUtf8());
    qputenv("XDG_DATA_HOME", (temporary.path()+"/data").toUtf8());
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc,argv); app.setApplicationName("ORBYX");
    bool expectedDialogs=false,unexpectedDialog=false;
    QTimer watchdog;watchdog.setInterval(50);
    QObject::connect(&watchdog,&QTimer::timeout,[&] {
        if(expectedDialogs)return;
        if(auto *box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
            std::cerr<<"Unexpected modal: "<<box->text().toStdString()<<std::endl;
            unexpectedDialog=true;box->reject();
        }
    });watchdog.start();
    try {
        requireTest(argc == 2, "Expected fixture executable");
        const QString fixture = temporary.path()+"/fixture";
        requireTest(QProcess::execute(argv[1], {"--fixture",fixture}) == 0, "Fixture failed");
        for (const QString lang : {"en","it"}) {
            std::cout<<"Queue widgets: "<<lang.toStdString()<<std::endl;
            Window w; w.previewSettings(lang,"dark",lang=="it"?QSize(960,680):QSize(1280,860)); w.show();
            auto *finalButton=w.findChild<QPushButton*>("finalImageButton");
            requireTest(finalButton&&finalButton->isVisible()&&!finalButton->accessibleName().isEmpty(),"Final-image guided action is not accessible");
            bool compositorOpened=false;
            QTimer closeCompositor;closeCompositor.setInterval(20);
            QObject::connect(&closeCompositor,&QTimer::timeout,[&]{if(auto* dialog=dynamic_cast<MosaicDialog*>(QApplication::activeModalWidget())){compositorOpened=true;dialog->reject();}});
            closeCompositor.start();clickVisible(w.findChild<QPushButton*>("mosaicButton"));closeCompositor.stop();
            requireTest(compositorOpened,"Main window compositor button did not open the production dialog");
            const QString result = temporary.path()+"/output-"+lang;
            w.loadArguments({"--input",fixture+"/input","--config",fixture+"/profile.json","--output",result,"--satellite","L2","--cpu"});
            auto *table=w.findChild<QTableWidget*>("queueTable");
            auto *add=w.findChild<QPushButton*>("queueAdd");
            auto *remove=w.findChild<QPushButton*>("queueRemove");
            auto *run=w.findChild<QPushButton*>("queueRun");
            requireTest(table && add && remove && run, "Queue controls missing");
            while(table->rowCount()){table->setCurrentCell(table->rowCount()-1,0);clickVisible(remove);}
            clickVisible(add); clickVisible(add);
            requireTest(table->rowCount()==2,"Add clicks failed");
            table->setCurrentCell(1,0); clickVisible(remove);
            requireTest(table->rowCount()==1,"Remove click failed");
            clickVisible(run);
            requireTest(until([&]{return run->isEnabled();}),"Queue stuck busy");
            if(table->item(0,2)->text()!=(lang=="it"?"Completato":"Done"))std::cerr<<w.findChild<QTextEdit*>()->toPlainText().toStdString()<<'\n';
            requireTest(table->item(0,2)->text()==(lang=="it"?"Completato":"Done"),"Queue completion missing");
            const QString file=result+"/MSUGS_VIS-1_Aligned/MSUGS-VIS-1.png";
            const auto before=hashFile(file);
            run->setFocus(); QTest::keyClick(run,Qt::Key_Space);
            requireTest(until([&]{return run->isEnabled();}),"Resume stuck busy");
            requireTest(hashFile(file)==before,"Resume overwrote image");
            requireTest(table->item(0,2)->text()==(lang=="it"?"Completato":"Done"),"Verified resume did not retain completion");
            const QString roiProfile=temporary.path()+"/roi-profile-"+lang+".json";
            QFile sourceProfile(fixture+"/profile.json");requireTest(sourceProfile.open(QIODevice::ReadOnly),"ROI profile source unreadable");
            auto roiDocument=QJsonDocument::fromJson(sourceProfile.readAll()).object();sourceProfile.close();
            for(const QString vis:{"L2VIS1","L2VIS2"}){auto section=roiDocument[vis].toObject();section["ROI"]=QJsonObject{{"x",1},{"y",0},{"width",20},{"height",vis.endsWith('1')?16:10}};roiDocument[vis]=section;}
            QFile roiFile(roiProfile);requireTest(roiFile.open(QIODevice::WriteOnly|QIODevice::NewOnly),"ROI profile fixture creation failed");requireTest(roiFile.write(QJsonDocument(roiDocument).toJson())>0,"ROI profile fixture write failed");roiFile.close();
            const QString directResult=temporary.path()+"/direct-output-"+lang;
            w.loadArguments({"--input",fixture+"/input","--config",roiProfile,"--output",directResult,"--satellite","L2","--cpu"});
            auto* start=w.findChild<QPushButton*>("start");clickVisible(start);requireTest(until([&]{return start->isEnabled();}),"Direct processing stuck busy");
            requireTest(QFileInfo::exists(directResult+"/MSUGS_VIS_Aligned/MSUGS-VIS-1.png")&&QFileInfo::exists(directResult+"/MSUGS_VIS-1_Aligned/MSUGS-VIS-1.png"),"GUI did not retain editable products beside historical ROI output");
            bool roiEditorOpened=false;QTimer inspectRoiEditor;inspectRoiEditor.setInterval(20);
            QObject::connect(&inspectRoiEditor,&QTimer::timeout,[&]{if(auto* dialog=dynamic_cast<MosaicDialog*>(QApplication::activeModalWidget()))if(!dialog->isBusy()&&dialog->state().size()==2){roiEditorOpened=dialog->property("loadedProfileRoi").toBool()&&dialog->state()[1].x==20;dialog->saveProject(temporary.path()+"/main-roi-"+lang+".json");dialog->reject();}});
            inspectRoiEditor.start();clickVisible(w.findChild<QPushButton*>("mosaicButton"));inspectRoiEditor.stop();requireTest(roiEditorOpened,"Main window did not open historical ROI as editable layers");
            const QString guidedResult=temporary.path()+"/guided-output-"+lang;
            w.loadArguments({"--input",fixture+"/input","--config",roiProfile,"--output",guidedResult,"--satellite","L2","--cpu"});
            bool guidedEditorOpened=false;QTimer driveGuided;driveGuided.setInterval(30);
            QObject::connect(&driveGuided,&QTimer::timeout,[&]{
                if(auto *wizard=dynamic_cast<FinalImageWizard*>(QApplication::activeModalWidget())) {
                    if(wizard->currentId()<4)QTest::mouseClick(wizard->button(QWizard::NextButton),Qt::LeftButton);
                    else {auto *confirm=wizard->findChild<QCheckBox*>("finalConfirm");if(!confirm->isChecked())QTest::mouseClick(confirm,Qt::LeftButton);QTest::mouseClick(wizard->button(QWizard::FinishButton),Qt::LeftButton);}
                } else if(auto *dialog=dynamic_cast<MosaicDialog*>(QApplication::activeModalWidget())) {
                    if(!dialog->isBusy()&&dialog->state().size()==2){guidedEditorOpened=dialog->property("loadedProfileRoi").toBool();dialog->saveProject(temporary.path()+"/guided-project-"+lang+".json");dialog->reject();}
                }
            });
            driveGuided.start();clickVisible(finalButton);requireTest(until([&]{return guidedEditorOpened;},30000),"Guided final-image action did not hand off to the editable ROI compositor");driveGuided.stop();
            requireTest(QFileInfo::exists(guidedResult+"/MSUGS_VIS_Aligned/MSUGS-VIS-1.png"),"Guided final-image action omitted historical output");
            // M2-08: the guided wizard persists a resumable project and restores it,
            // so the flow can be continued later without re-entering every setting.
            {
                FinalImageWizard saver(lang=="it","/orbyx/in","/orbyx/profile.json","/orbyx/out","L3",true);
                saver.saveProject();
                FinalImageWizard resumer(lang=="it","","","","L9",false);
                requireTest(resumer.hasSavedProject(),"Guided wizard did not persist a resumable project");
                requireTest(resumer.resumeFromSaved(),"Guided wizard could not resume the saved project");
                requireTest(resumer.inputPath()=="/orbyx/in"&&resumer.profilePath()=="/orbyx/profile.json"&&resumer.outputPath()=="/orbyx/out"&&resumer.satelliteId()=="L3"&&resumer.cpuOnly()&&resumer.naturalColour(),"Resumed guided wizard restored the wrong settings");
            }
            // Fill the actual source/target/save dialogs and dismiss the success box.
            const auto originalProfile=hashFile(fixture+"/profile.json");
            const QString copied=temporary.path()+"/profile-"+lang+".json";
            int answers=0;bool timedOut=false;QPointer<QWidget> handled;
            QTimer responder;responder.setInterval(20);
            QElapsedTimer deadline;deadline.start();
            QObject::connect(&responder,&QTimer::timeout,[&] {
                auto *dialog=QApplication::activeModalWidget();
                if(!dialog)return;
                if(deadline.elapsed()>15000) {timedOut=true; if(auto *d=qobject_cast<QDialog*>(dialog))d->reject();return;}
                if(handled==dialog)return;
                handled=dialog;
                if(auto *input=qobject_cast<QInputDialog*>(dialog)) {
                    input->setTextValue(answers++==0?"L2":"L9");input->accept();
                } else if(auto *file=qobject_cast<QFileDialog*>(dialog)) {
                    file->setDirectory(temporary.path());file->selectFile(QFileInfo(copied).fileName());
                    auto *edit=file->findChild<QLineEdit*>("fileNameEdit");
                    if(!edit){timedOut=true;file->reject();return;}
                    edit->setText(QFileInfo(copied).fileName());
                    // Invoke the real QFileDialog accept override; QDialog::accept
                    // would bypass QFileDialog's selection and validation logic.
                    QMetaObject::invokeMethod(file,"accept",Qt::QueuedConnection);
                } else if(auto *message=qobject_cast<QMessageBox*>(dialog))message->accept();
            });
            expectedDialogs=true;responder.start();clickVisible(w.findChild<QPushButton*>("profileDup"));responder.stop();expectedDialogs=false;
            requireTest(!timedOut&&answers==2&&QFileInfo::exists(copied),"Profile dialog did not save a new profile");
            requireTest(hashFile(fixture+"/profile.json")==originalProfile,"Profile dialog changed original");
            QFile profile(copied);requireTest(profile.open(QIODevice::ReadOnly),"Copied profile unreadable");
            const auto document=QJsonDocument::fromJson(profile.readAll()).object();
            requireTest(document.value("L2")==document.value("L9"),"Profile dialog copied wrong satellite");
            requireTest(w.findChild<QLineEdit*>("config")->text()==copied,"GUI did not select copied profile");
            w.close();
            Window reopened;reopened.previewSettings(lang,"dark",{1280,860});
            auto *restored=reopened.findChild<QTableWidget*>("queueTable");
            requireTest(restored->rowCount()==1,"Queued jobs were lost on restart");
            requireTest(restored->item(0,2)->text()==(lang=="it"?"Da verificare":"Needs verification"),"Restart trusted stale completion without digest verification");
            requireTest(hashFile(file)==before,"Queue restore modified product");reopened.close();
        }
        for(const bool italian:{false,true}) {
            CalibrationWizard wizard(italian,fixture+"/input",fixture+"/profile.json",temporary.path()+"/wizard-output", "L4");wizard.show();
            QTest::qWait(20);
            QTest::mouseClick(wizard.button(QWizard::NextButton),Qt::LeftButton);
            requireTest(wizard.currentId()==1,"Wizard rejected complete acquisition");
            requireTest(wizard.scale()==.125&&wizard.satelliteId()=="L4","Wizard lost calibration settings");
            QTest::mouseClick(wizard.button(QWizard::NextButton),Qt::LeftButton);
            requireTest(wizard.currentId()==2&&!wizard.button(QWizard::FinishButton)->isEnabled(),"Wizard omitted explicit review");
            QTest::mouseClick(wizard.findChild<QCheckBox*>("wizardConfirm"),Qt::LeftButton);
            QTest::mouseClick(wizard.button(QWizard::FinishButton),Qt::LeftButton);
            requireTest(wizard.result()==QDialog::Accepted,"Wizard did not accept checked settings");
            CalibrationWizard invalid(italian,fixture+"/input",fixture+"/profile.json",fixture+"/input", "L4");invalid.show();
            QTest::mouseClick(invalid.button(QWizard::NextButton),Qt::LeftButton);
            requireTest(invalid.currentId()==0&&!invalid.property("validationError").toString().isEmpty(),"Wizard allowed existing/input destination");invalid.reject();
        }
        for(const bool italian:{false,true}) {
            FinalImageWizard wizard(italian,fixture+"/input",fixture+"/profile.json",
                                    temporary.path()+QString("/final-wizard-%1").arg(italian?"it":"en"),"L2",true);
            wizard.show();QTest::qWait(20);
            requireTest(wizard.pageIds().size()==5&&wizard.button(QWizard::FinishButton)->text().contains(italian?"editor":"editor",Qt::CaseInsensitive),"Final-image wizard does not expose five guided stages");
            QTest::mouseClick(wizard.button(QWizard::NextButton),Qt::LeftButton);
            requireTest(wizard.currentId()==1,"Final-image wizard rejected a complete SatDump export");
            QTest::mouseClick(wizard.button(QWizard::NextButton),Qt::LeftButton);
            requireTest(wizard.currentId()==2&&!wizard.findChild<QLabel*>("finalJoinSummary")->text().isEmpty(),"Final-image wizard omitted join guidance");
            QTest::mouseClick(wizard.button(QWizard::NextButton),Qt::LeftButton);
            requireTest(wizard.currentId()==3,"Final-image wizard omitted colour stage");
            wizard.findChild<QComboBox*>("finalColour")->setCurrentIndex(1);
            QTest::mouseClick(wizard.button(QWizard::NextButton),Qt::LeftButton);
            requireTest(wizard.currentId()==4&&!wizard.button(QWizard::FinishButton)->isEnabled(),"Final-image wizard omitted explicit export review");
            QTest::mouseClick(wizard.findChild<QCheckBox*>("finalConfirm"),Qt::LeftButton);
            QTest::mouseClick(wizard.button(QWizard::FinishButton),Qt::LeftButton);
            requireTest(wizard.result()==QDialog::Accepted&&wizard.cpuOnly()&&!wizard.naturalColour(),"Final-image wizard lost reviewed settings");
            FinalImageWizard invalid(italian,fixture+"/input",fixture+"/profile.json",fixture+"/input","L2",false);
            invalid.show();QTest::mouseClick(invalid.button(QWizard::NextButton),Qt::LeftButton);
            requireTest(invalid.currentId()==0&&!invalid.property("validationError").toString().isEmpty(),"Final-image wizard allowed a destructive destination");invalid.reject();
        }
        {
            // The native-size synthetic fixture gets explicitly synthetic but
            // structurally valid SatDump CBOR to exercise the real inspector process.
            const auto parent=temporary.path()+"/incoming";const auto acquisition=parent+"/capture";
            QDir().mkpath(acquisition);
            for(int vis=1;vis<=2;++vis) {
                auto dest=QDir(acquisition).filePath(QString("MSUGS_VIS%1").arg(vis));QDir().mkpath(dest);
                for(int c=1;c<=3;++c)QFile::copy(fixture+QString("/input/MSUGS_VIS%1/MSUGS-VIS-%2.png").arg(vis).arg(c),dest+QString("/MSUGS-VIS-%1.png").arg(c));
                QByteArray meta=QByteArray::fromHex("a466696d6167657383a3696269745f6465707468086466696c656f4d535547532d5649532d312e706e67646e616d656131a3696269745f6465707468086466696c656f4d535547532d5649532d322e706e67646e616d656132a3696269745f6465707468086466696c656f4d535547532d5649532d332e706e67646e616d6561336a696e737472756d656e74696d737567735f766973647479706565696d61676569736174656c6c697465624c32");
                QFile file(dest+"/product.cbor");requireTest(file.open(QIODevice::WriteOnly),"Cannot create test CBOR");file.write(meta);
            }
            int manual=0,automatic=0;
            ImportDialog importer(false,[&](const QJsonObject& product,bool autoQueue){requireTest(product["status"]=="ready","Importer sent invalid product");requireTest(product["fingerprint"].toString().size()==64,"Missing acquisition fingerprint");if(autoQueue)++automatic;else ++manual;return true;});
            importer.show();importer.findChild<QLineEdit*>("watchFolder")->setText(parent);
            clickVisible(importer.findChild<QPushButton*>("watchScan"));requireTest(until([&]{return !importer.scanning();}),"Import inspection stuck");
            auto *table=importer.findChild<QTableWidget*>("importProducts");requireTest(table->rowCount()==1,"Native product not discovered");
            table->setCurrentCell(0,0);clickVisible(importer.findChild<QPushButton*>("watchImport"));requireTest(manual==1&&automatic==0,"Manual import not explicit");
            importer.findChild<QCheckBox*>("watchAutomatic")->setChecked(true);importer.scanOnce();requireTest(until([&]{return !importer.scanning();}),"Automatic scan stuck");requireTest(automatic==1,"Opt-in automatic import failed");
            importer.scanOnce();requireTest(until([&]{return !importer.scanning();}),"Duplicate scan stuck");requireTest(automatic==1,"Duplicate acquisition queued twice");importer.reject();
        }
        for(const bool italian:{false,true}) {
            bool busy=false;UpdateDialog updates(italian,[&](bool running){busy=running;});updates.show();
            clickVisible(updates.findChild<QPushButton*>("updateInstall"));
            requireTest(!busy&&!updates.findChild<QPlainTextEdit*>("updateLog")->toPlainText().isEmpty(),"Update dialog accepted missing signature settings");updates.close();
        }
        // Launch a genuine exec failure: the executable exists, its interpreter does not.
        const QString fakeDir=temporary.path()+"/fake"; QDir().mkpath(fakeDir);
        QFile fake(fakeDir+"/orbyx-batch"); requireTest(fake.open(QIODevice::WriteOnly),"Fake runner unavailable");
        fake.write("#!/nonexistent-orbyx-test-interpreter\n");fake.close();
        fake.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner|QFileDevice::ExeOwner);
        const auto oldPath=qgetenv("PATH"); qputenv("PATH",fakeDir.toUtf8()+":"+oldPath);
        Window w;w.previewSettings("en","dark",{1280,1000});w.show();
        w.loadArguments({"--input",fixture+"/input","--config",fixture+"/profile.json","--output",temporary.path()+"/exec-fail","--satellite","L2","--cpu"});
        auto *failureTable=w.findChild<QTableWidget*>("queueTable");
        while(failureTable->rowCount()){failureTable->setCurrentCell(failureTable->rowCount()-1,0);clickVisible(w.findChild<QPushButton*>("queueRemove"));}
        clickVisible(w.findChild<QPushButton*>("queueAdd"));
        auto *run=w.findChild<QPushButton*>("queueRun");clickVisible(run);
        requireTest(until([&]{return run->isEnabled();}),"FailedToStart left GUI disabled");
        requireTest(w.findChild<QTableWidget*>("queueTable")->item(0,2)->text()=="Error","Exec failure marked successful");
        requireTest(fake.open(QIODevice::WriteOnly|QIODevice::Truncate),"Fake runner rewrite failed");
        fake.write(QByteArray("#!/bin/sh\nwhile [ \"$#\" -gt 0 ]; do if [ \"$1\" = --state ]; then shift; state=$1; fi; shift; done\nprintf '%s' '{\"version\":1,\"jobs\":{\"job-1\":{\"status\":\"running\"}}}' > \"$state\"\n").replace("job-1",w.findChild<QTableWidget*>("queueTable")->item(0,0)->data(Qt::UserRole).toMap()["id"].toString().toUtf8()));fake.close();
        clickVisible(run);
        requireTest(until([&]{return run->isEnabled();}),"Nonterminal state left GUI disabled");
        requireTest(w.findChild<QTableWidget*>("queueTable")->item(0,2)->text()=="Error","Running state marked successful");
        qputenv("PATH",oldPath); w.close();
        // Drive the production demo button in both languages; verify the displayed
        // image and the source snapshots, rather than a separate renderer.
        for(const QString lang : {"en","it"}) {
            Window demo;demo.previewSettings(lang,"dark",{1280,1000});demo.show();
            auto *button=demo.findChild<QPushButton*>("demoButton");
            requireTest(button&&button->isVisible()&&!button->accessibleName().isEmpty(),"Demo button inaccessible");
            clickVisible(button);
            requireTest(until([&]{return button->isEnabled();},60000),"Demo stuck busy");
            const auto log=demo.findChild<QTextEdit*>()->toPlainText();
            requireTest(log.contains("1428x1428")&&!log.contains("ERROR:"),"Real six-image demo failed");
            requireTest(!demo.findChild<QLabel*>("canvas")->pixmap().isNull(),"Demo result not displayed");
            requireTest(demo.findChild<QProgressBar*>()->value()==100,"Demo was not completed");
            demo.close();
        }
        // Manual image simulation: the dedicated button exists and is accessible,
        // and running the pipeline on an explicitly chosen folder (the bundled
        // demo images) produces the same composite — no RDAS data, no .json.
        {
            Window sim;sim.previewSettings("it","dark",{1280,1000});sim.show();
            auto *simButton=sim.findChild<QPushButton*>("simulateButton");
            requireTest(simButton&&simButton->isVisible()&&!simButton->accessibleName().isEmpty(),"Simulation button inaccessible");
            const QString folder=sim.demoFolderForTests();
            requireTest(!folder.isEmpty(),"Bundled demo folder not found for manual simulation");
            sim.runImageSimulation(folder);
            requireTest(until([&]{return simButton->isEnabled();},60000),"Manual simulation stuck busy");
            const auto simLog=sim.findChild<QTextEdit*>()->toPlainText();
            requireTest(simLog.contains("1428x1428")&&!simLog.contains("ERROR:"),"Manual image simulation failed");
            requireTest(!sim.findChild<QLabel*>("canvas")->pixmap().isNull(),"Manual simulation result not displayed");
            requireTest(sim.findChild<QProgressBar*>()->value()==100,"Manual simulation was not completed");
            sim.close();
        }

        {
            Window first,second;first.previewSettings("en","dark",{1280,860});second.previewSettings("en","dark",{1280,860});first.show();second.show();
            first.loadArguments({"--input",fixture+"/input","--config",fixture+"/profile.json","--output",temporary.path()+"/persist-first","--satellite","L2","--cpu"});
            second.loadArguments({"--input",fixture+"/input","--config",fixture+"/profile.json","--output",temporary.path()+"/persist-second","--satellite","L2","--cpu"});
            int before=second.findChild<QTableWidget*>("queueTable")->rowCount();
            clickVisible(first.findChild<QPushButton*>("queueAdd"));clickVisible(second.findChild<QPushButton*>("queueAdd"));
            requireTest(second.findChild<QTableWidget*>("queueTable")->rowCount()==before,"Concurrent window silently overwrote queue");
            requireTest(!second.findChild<QPushButton*>("queueAdd")->isEnabled(),"Stale queue remained writable");
            Window check;requireTest(check.findChild<QTableWidget*>("queueTable")->rowCount()==before+1,"Saved queue was lost after revision conflict");check.close();second.close();first.close();
        }
        {
            const auto base=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            const auto stored=base+"/desktop-queue.json",backup=base+"/desktop-queue.backup";
            requireTest(QFile::rename(stored,backup),"Cannot isolate legacy migration test");
            Window legacy;auto *table=legacy.findChild<QTableWidget*>("queueTable");
            requireTest(table->rowCount()==1,"Previous queue manifest was not migrated");
            requireTest(!QFileInfo::exists(stored),"Startup wrote queue storage implicitly");legacy.close();
            requireTest(QFile::rename(backup,stored),"Cannot restore queue after migration test");
            const auto original=hashFile(stored);
            requireTest(QFile::rename(stored,backup),"Cannot isolate corrupt queue test");
            QFile corrupt(stored);requireTest(corrupt.open(QIODevice::WriteOnly),"Cannot write malformed queue fixture");corrupt.write("{broken");corrupt.close();
            Window rejected;requireTest(!rejected.findChild<QPushButton*>("queueAdd")->isEnabled(),"Corrupt queue became writable");
            requireTest(rejected.findChild<QTableWidget*>("queueTable")->rowCount()==0,"Corrupt queue produced jobs");rejected.close();
            QFile retained(stored);requireTest(retained.open(QIODevice::ReadOnly)&&retained.readAll()=="{broken","Corrupt source was overwritten");retained.close();
            QFile::remove(stored);requireTest(QFile::rename(backup,stored)&&hashFile(stored)==original,"Queue backup changed");
        }
        const QDir sample(QDir(QCoreApplication::applicationDirPath()).filePath("share/orbyx/demo-jpeg"));
        QFile sums(sample.filePath("SHA256SUMS"));requireTest(sums.open(QIODevice::ReadOnly),"Demo source hashes unavailable");
        int sources=0;
        for(const auto &line:sums.readAll().split('\n')) {
            if(line.isEmpty())continue;
            requireTest(hashFile(sample.filePath(QString::fromUtf8(line.mid(66)))).toHex()==line.left(64),"Demo modified source JPEG");++sources;
        }
        requireTest(sources==6,"Demo did not verify all six JPEGs");
        const auto reject=[&](const QString &input,const QString &output) {
            try {orbyx::runImageDemo(input.toStdString(),output.toStdString());}catch(const std::exception&){return true;}
            return false;
        };
        const auto existing=temporary.path()+"/existing";QDir().mkpath(existing);
        QFile sentinel(existing+"/keep");requireTest(sentinel.open(QIODevice::WriteOnly),"Sentinel unavailable");sentinel.write("keep");sentinel.close();
        requireTest(reject(sample.path(),existing)&&QFileInfo::exists(existing+"/keep"),"Demo overwrote an existing output");
        const auto broken=temporary.path()+"/broken";QDir().mkpath(broken);
        for(const QString side:{"sx","dx"})for(int c=1;c<=3;++c)
            cv::imwrite(QDir(broken).filePath(side+"_MSUGS-VIS-"+QString::number(c)+".jpg").toStdString(),cv::Mat::zeros(2048,714,CV_8U));
        const auto failed=temporary.path()+"/failed-demo";
        requireTest(reject(broken,failed)&&!QFileInfo::exists(failed),"Unregisterable demo did not fail and roll back");
        requireTest(reject(temporary.path()+"/missing",failed)&&!QFileInfo::exists(failed),"Missing demo inputs left partial output");
        requireTest(!unexpectedDialog,"Unexpected modal dialog during widget tests");
        std::cout<<"Visible queue/profile/demo EN/IT, mouse, keyboard resume, output preservation, exec failure and nonterminal state passed\n";
        return 0;
    } catch (const std::exception &e) { std::cerr<<e.what()<<'\n';return 1; }
}
