// SPDX-License-Identifier: GPL-3.0-only
#include "../src/gui/mosaic_dialog.hpp"
#include <QTest>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <opencv2/imgcodecs.hpp>
#include <iostream>
void check(bool ok,const char* m){if(!ok)throw std::runtime_error(m);}
void waitFor(MosaicDialog& dialog){QElapsedTimer timer;timer.start();while(dialog.isBusy()&&timer.elapsed()<30000)QTest::qWait(10);check(!dialog.isBusy(),"Editor worker stuck");}
int main(int argc,char** argv){
    QApplication app(argc,argv);app.setApplicationName("ORBYX-Mosaic-Test");QTemporaryDir temp;
    qputenv("XDG_DATA_HOME",(temp.path()+"/data").toUtf8());
    try{
        auto root=std::filesystem::path(temp.path().toStdString());
        cv::Mat input(80,100,CV_16UC3,cv::Scalar(1025,4099,10003));
        for(const char* name:{"one.png","two.png","three.png"})check(cv::imwrite((root/name).string(),input),"Fixture write");
        for(bool it:{false,true}){
            MosaicDialog dialog(it);dialog.resize(960,680);dialog.show();
            dialog.appendFiles({temp.path()+"/one.png",temp.path()+"/two.png",temp.path()+"/three.png"});waitFor(dialog);
            check(dialog.state().size()==3,"Editor assumes exactly two layers");
            auto* list=dialog.findChild<QListWidget*>("mosaicLayers");check(list&&list->count()==3,"Layer list missing");
            auto state=dialog.state();state[1].x=100;state[2].y=80;dialog.setState(state);waitFor(dialog);
            auto* x=dialog.findChild<QDoubleSpinBox*>("mosaic_x");auto* gamma=dialog.findChild<QDoubleSpinBox*>("mosaic_gamma");
            check(x&&gamma,"Geometry/colour controls missing");x->setValue(-15);waitFor(dialog);
            check(dialog.state()[2].x==-15,"Manual geometry not applied");
            dialog.undo();waitFor(dialog);check(dialog.state()[2].x==0,"Undo geometry failed");
            dialog.redo();waitFor(dialog);check(dialog.state()[2].x==-15,"Redo geometry failed");
            gamma->setValue(1.25);waitFor(dialog);dialog.applyColoursToAll();waitFor(dialog);
            for(const auto& layer:dialog.state())check(layer.gamma==1.25,"Colour application not applied to all portions");
            auto* canvas=dynamic_cast<MosaicCanvas*>(dialog.findChild<QLabel*>("mosaicCanvas"));check(canvas&&!canvas->pixmap().isNull(),"Composite preview missing");
            const auto oldX=dialog.state()[2].x;
            QTest::mousePress(canvas,Qt::LeftButton,{},QPoint(10,10));QTest::mouseRelease(canvas,Qt::LeftButton,{},QPoint(20,15));waitFor(dialog);
            check(dialog.state()[2].x==oldX+10,"Drag editing did not move selected layer");
            dialog.reorder(-1);waitFor(dialog);check(list->currentRow()==1,"Layer reordering failed");
            QString project=temp.path()+(it?"/it.json":"/en.json");dialog.saveProject(project);
            auto expected=orbyx::renderMosaic(dialog.state()).image;
            MosaicDialog restored(it);restored.loadProject(project);waitFor(restored);
            check(restored.state().size()==3,"Project reopening lost layers");
            const auto output=temp.path()+(it?"/it.png":"/en.png");restored.exportTo(output);waitFor(restored);
            auto actual=cv::imread(output.toStdString(),cv::IMREAD_UNCHANGED);
            check(!actual.empty()&&actual.type()==CV_16UC3&&cv::norm(actual,expected,cv::NORM_INF)==0,"GUI export differs from shared native-depth renderer");
            restored.exportTo(output);waitFor(restored);check(restored.findChild<QLabel*>("mosaicStatus")->text().contains(it?"Errore":"Error"),"Existing export was not rejected");
            check(cv::norm(cv::imread(output.toStdString(),cv::IMREAD_UNCHANGED),expected,cv::NORM_INF)==0,"Existing export modified");
            // Compose three actual aligned grayscale files inside the GUI.
            auto folder=root/(it?"vis-it":"vis-en");std::filesystem::create_directory(folder);
            for(int i=1;i<=3;++i)cv::imwrite((folder/("MSUGS-VIS-"+std::to_string(i)+".png")).string(),cv::Mat(80,100,CV_16UC1,cv::Scalar(i*4001)));
            MosaicDialog channels(it);channels.appendChannels(QString::fromStdString(folder.string()),true);waitFor(channels);
            check(channels.state().size()==1,"GUI colour composition did not create a portion");
            check(orbyx::readCompositePng(channels.state()[0].image).type()==CV_16UC3,"Natural Color in editor lost native depth");
            channels.saveProject(temp.path()+(it?"/channels-it.json":"/channels-en.json"));
            if(it&&argc>1){MosaicDialog demo(it);demo.appendDemo(QString::fromLocal8Bit(argv[1]));waitFor(demo);check(demo.state().size()==2,"Real six-JPEG editable demo missing");auto real=orbyx::renderMosaic(demo.state());check(real.image.size()==cv::Size(1428,1428),"Editable real-demo canvas incorrect");demo.show();QTest::qWait(30);if(qEnvironmentVariableIsSet("ORBYX_MOSAIC_SCREENSHOT"))demo.grab().save(qEnvironmentVariable("ORBYX_MOSAIC_SCREENSHOT"));demo.saveProject(temp.path()+"/demo.json");demo.close();}
            dialog.close();restored.close();channels.close();
        }
        std::cout<<"EN/IT multi-layer geometry, drag, undo, colour, project and full-resolution export passed\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
