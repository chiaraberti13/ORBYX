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
#include <iostream>
#include <stdexcept>

void requireTest(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
template<class F> bool until(F predicate, int timeout = 15000) {
    QElapsedTimer timer; timer.start();
    while (!predicate() && timer.elapsed() < timeout) QTest::qWait(10);
    return predicate();
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
    try {
        requireTest(argc == 2, "Expected fixture executable");
        const QString fixture = temporary.path()+"/fixture";
        requireTest(QProcess::execute(argv[1], {"--fixture",fixture}) == 0, "Fixture failed");
        for (const QString lang : {"en","it"}) {
            Window w; w.previewSettings(lang,"dark",{1280,1000}); w.show();
            const QString result = temporary.path()+"/output-"+lang;
            w.loadArguments({"--input",fixture+"/input","--config",fixture+"/profile.json","--output",result,"--satellite","L2","--cpu"});
            auto *table=w.findChild<QTableWidget*>("queueTable");
            auto *add=w.findChild<QPushButton*>("queueAdd");
            auto *remove=w.findChild<QPushButton*>("queueRemove");
            auto *run=w.findChild<QPushButton*>("queueRun");
            requireTest(table && add && remove && run, "Queue controls missing");
            QTest::mouseClick(add,Qt::LeftButton); QTest::mouseClick(add,Qt::LeftButton);
            requireTest(table->rowCount()==2,"Add clicks failed");
            table->setCurrentCell(1,0); QTest::mouseClick(remove,Qt::LeftButton);
            requireTest(table->rowCount()==1,"Remove click failed");
            QTest::mouseClick(run,Qt::LeftButton);
            requireTest(until([&]{return run->isEnabled();}),"Queue stuck busy");
            if(table->item(0,2)->text()!=(lang=="it"?"Completato":"Done"))std::cerr<<w.findChild<QTextEdit*>()->toPlainText().toStdString()<<'\n';
            requireTest(table->item(0,2)->text()==(lang=="it"?"Completato":"Done"),"Queue completion missing");
            const QString file=result+"/MSUGS_VIS-1_Aligned/MSUGS-VIS-1.png";
            const auto before=hashFile(file);
            run->setFocus(); QTest::keyClick(run,Qt::Key_Space);
            requireTest(until([&]{return run->isEnabled();}),"Resume stuck busy");
            requireTest(hashFile(file)==before,"Resume overwrote image");
            requireTest(table->item(0,2)->text()==(lang=="it"?"Completato":"Done"),"Verified resume did not retain completion");
            w.close();
        }
        // Launch a genuine exec failure: the executable exists, its interpreter does not.
        const QString fakeDir=temporary.path()+"/fake"; QDir().mkpath(fakeDir);
        QFile fake(fakeDir+"/orbyx-batch"); requireTest(fake.open(QIODevice::WriteOnly),"Fake runner unavailable");
        fake.write("#!/nonexistent-orbyx-test-interpreter\n");fake.close();
        fake.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner|QFileDevice::ExeOwner);
        const auto oldPath=qgetenv("PATH"); qputenv("PATH",fakeDir.toUtf8()+":"+oldPath);
        Window w;w.previewSettings("en","dark",{1280,1000});w.show();
        w.loadArguments({"--input",fixture+"/input","--config",fixture+"/profile.json","--output",temporary.path()+"/exec-fail","--satellite","L2","--cpu"});
        QTest::mouseClick(w.findChild<QPushButton*>("queueAdd"),Qt::LeftButton);
        auto *run=w.findChild<QPushButton*>("queueRun");QTest::mouseClick(run,Qt::LeftButton);
        requireTest(until([&]{return run->isEnabled();}),"FailedToStart left GUI disabled");
        requireTest(w.findChild<QTableWidget*>("queueTable")->item(0,2)->text()=="Error","Exec failure marked successful");
        qputenv("PATH",oldPath); w.close();
        // Drive the production demo button in both languages; verify the displayed
        // image and the source snapshots, rather than a separate renderer.
        for(const QString lang : {"en","it"}) {
            Window demo;demo.previewSettings(lang,"dark",{1280,1000});demo.show();
            auto *button=demo.findChild<QPushButton*>("demoButton");
            requireTest(button&&button->isVisible()&&!button->accessibleName().isEmpty(),"Demo button inaccessible");
            QTest::mouseClick(button,Qt::LeftButton);
            requireTest(until([&]{return button->isEnabled();},30000),"Demo stuck busy");
            const auto log=demo.findChild<QTextEdit*>()->toPlainText();
            requireTest(log.contains("1428x1428")&&!log.contains("ERROR:"),"Real six-image demo failed");
            requireTest(!demo.findChild<QLabel*>("canvas")->pixmap().isNull(),"Demo result not displayed");
            requireTest(demo.findChild<QProgressBar*>()->value()==100,"Demo was not completed");
            demo.close();
        }

        std::cout<<"Visible queue EN/IT, mouse, keyboard resume, output preservation and exec failure passed\n";
        return 0;
    } catch (const std::exception &e) { std::cerr<<e.what()<<'\n';return 1; }
}
