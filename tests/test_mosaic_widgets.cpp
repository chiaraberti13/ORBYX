// SPDX-License-Identifier: GPL-3.0-only
#include "../src/gui/mosaic_dialog.hpp"
#include <QTest>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QAbstractButton>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <fstream>
void check(bool ok,const char* m){if(!ok)throw std::runtime_error(m);}
void waitFor(MosaicDialog& dialog){QElapsedTimer timer;timer.start();while(dialog.isBusy()&&timer.elapsed()<30000)QTest::qWait(10);check(!dialog.isBusy(),"Editor worker stuck");}
int main(int argc,char** argv){
    QApplication app(argc,argv);app.setApplicationName("ORBYX-Mosaic-Test");QTemporaryDir temp;
    qputenv("XDG_DATA_HOME",(temp.path()+"/data").toUtf8());
    try{
        auto root=std::filesystem::path(temp.path().toStdString());
        cv::Mat input(80,100,CV_16UC3,cv::Scalar(1025,4099,10003));
        for(const char* name:{"one.png","two.png","three.png"})check(cv::imwrite((root/name).string(),input),"Fixture write");
        cv::Mat autoScene(420,680,CV_8UC3,cv::Scalar(8,8,8));cv::RNG random(0x4155544f);
        for(int i=0;i<350;++i)cv::circle(autoScene,{random.uniform(5,675),random.uniform(5,415)},random.uniform(2,8),{double(random.uniform(35,255)),double(random.uniform(35,255)),double(random.uniform(35,255))},-1,cv::LINE_AA);
        for(int i=0;i<60;++i)cv::line(autoScene,{random.uniform(0,680),random.uniform(0,420)},{random.uniform(0,680),random.uniform(0,420)},{double(random.uniform(20,240)),double(random.uniform(20,240)),double(random.uniform(20,240))},1,cv::LINE_AA);
        cv::putText(autoScene,"ORBYX",{230,210},cv::FONT_HERSHEY_SIMPLEX,1.4,{255,255,255},3,cv::LINE_AA);
        cv::imwrite((root/"auto-a.png").string(),autoScene(cv::Rect(0,0,410,340)));
        cv::imwrite((root/"auto-b.png").string(),autoScene(cv::Rect(230,65,410,340)));
        const auto generated=root/"generated";std::filesystem::create_directories(generated/"MSUGS_VIS-1_Aligned");std::filesystem::create_directories(generated/"MSUGS_VIS-2_Aligned");
        for(int vis=1;vis<=2;++vis)for(int channel=1;channel<=3;++channel)
            cv::imwrite((generated/("MSUGS_VIS-"+std::to_string(vis)+"_Aligned")/("MSUGS-VIS-"+std::to_string(channel)+".png")).string(),cv::Mat(80,100,CV_16UC1,cv::Scalar(vis*5000+channel*1000)));
        const auto generatedProfile=root/"generated-profile.json";
        std::ofstream(generatedProfile)<<R"({"L2VIS1":{"ROI":{"x":5,"y":6,"width":60,"height":50}},"L2VIS2":{"ROI":{"x":10,"y":8,"width":55,"height":45}}})";
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
            canvas->setFocus();const auto nudgeX=dialog.state()[2].x,nudgeY=dialog.state()[2].y;
            QTest::keyClick(canvas,Qt::Key_Right);QTest::keyClick(canvas,Qt::Key_Down,Qt::ShiftModifier);waitFor(dialog);
            check(dialog.state()[2].x==nudgeX+1&&dialog.state()[2].y==nudgeY+10&&dialog.property("mergeStrategy").toString()=="manual","Keyboard fine/coarse alignment failed");
            dialog.undo();waitFor(dialog);check(dialog.state()[2].y==nudgeY,"Keyboard alignment is not undoable");
            dialog.redo();waitFor(dialog);check(dialog.state()[2].y==nudgeY+10,"Keyboard alignment redo failed");
            auto inspectionState=dialog.state();inspectionState[2].x=120;inspectionState[2].y=20;inspectionState[2].exposure=.5;dialog.setState(inspectionState);waitFor(dialog);
            const auto beforeBlink=orbyx::mosaicProject(dialog.state());auto* blink=dialog.findChild<QCheckBox*>("mosaicBlink");
            check(blink,"Blink seam inspection control missing");blink->setChecked(true);
            QElapsedTimer blinkDeadline;blinkDeadline.start();while(!dialog.property("blinkHidden").toBool()&&blinkDeadline.elapsed()<2500)QTest::qWait(20);
            check(dialog.property("blinkHidden").toBool(),"Blink seam inspection did not alternate the selected layer");blink->setChecked(false);waitFor(dialog);
            check(!dialog.property("blinkHidden").toBool()&&orbyx::mosaicProject(dialog.state())==beforeBlink,"Blink seam inspection changed editable or export state");
            auto* inspection=dialog.findChild<QComboBox*>("mosaicInspection");check(inspection&&inspection->count()==4,"Seam inspection modes missing");
            inspection->setCurrentIndex(1);waitFor(dialog);
            check(dialog.property("differenceComparison").toInt()==1&&dialog.property("differencePixels").toInt()>0&&!canvas->pixmap().isNull(),"Difference seam inspection did not compare the selected adjacent pair");
            inspection->setCurrentIndex(2);waitFor(dialog);
            check(dialog.property("inspectionRendered").toInt()==2&&dialog.property("differenceComparison").toInt()==1&&!canvas->pixmap().isNull(),"Checkerboard seam inspection did not compare the selected adjacent pair");
            auto* swipe=dialog.findChild<QSlider*>("mosaicSwipe");check(swipe&&!swipe->isEnabled(),"Swipe control enabled outside its inspection mode");
            inspection->setCurrentIndex(3);swipe->setValue(35);waitFor(dialog);
            check(swipe->isEnabled()&&dialog.property("inspectionRendered").toInt()==3&&dialog.property("swipeRendered").toInt()==35,"Swipe seam inspection did not render the reviewed position");
            inspection->setCurrentIndex(0);waitFor(dialog);
            check(orbyx::mosaicProject(dialog.state())==beforeBlink,"Difference seam inspection changed editable or export state");
            QTest::mouseClick(dialog.findChild<QPushButton*>("mosaicZoomSeam"),Qt::LeftButton);
            check(dialog.property("seamZoomed").toBool()&&orbyx::mosaicProject(dialog.state())==beforeBlink,"Dedicated seam zoom failed or changed editable state");
            const double controlScale=.98,controlAngle=2*CV_PI/180.,controlC=std::cos(controlAngle)*controlScale,controlS=std::sin(controlAngle)*controlScale;
            std::vector<orbyx::ControlPointPair> points;for(const cv::Point2d point:std::vector<cv::Point2d>{{5,5},{70,8},{8,60},{80,65},{40,30}})points.push_back({point,{controlC*point.x-controlS*point.y+12,controlS*point.x+controlC*point.y+4}});points.push_back({{30,50},{900,-800}});
            const auto movingBefore=dialog.state()[2];auto solved=orbyx::alignFromControlPoints(movingBefore,dialog.state()[1],points);
            ControlPointsDialog pointDialog(it,movingBefore,dialog.state()[1]);pointDialog.setPairs(points);check(pointDialog.solve(),"Control-point dialog did not solve reviewed pairs");check(pointDialog.findChild<QLabel*>("controlPointSummary")->text().contains("5/6"),"Control-point dialog omitted inlier/residual evidence");
            dialog.applyControlPoints(solved,2,1);waitFor(dialog);
            check(dialog.property("mergeStrategy").toString()=="control-points"&&dialog.property("controlPointInliers").toInt()==5&&dialog.state()[2].x!=movingBefore.x,"Control-point solution was not applied to the selected layer");
            dialog.undo();waitFor(dialog);check(dialog.state()[2].x==movingBefore.x&&dialog.state()[2].y==movingBefore.y,"Control-point alignment is not undoable");
            dialog.redo();waitFor(dialog);check(dialog.state()[2].x==solved.layer.x,"Control-point alignment redo failed");
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
            MosaicDialog profileRoi(it);profileRoi.show();profileRoi.appendGeneratedProducts(QString::fromStdString(generated.string()),QString::fromStdString(generatedProfile.string()),"L2",true);waitFor(profileRoi);
            check(profileRoi.state().size()==2&&profileRoi.property("loadedProfileRoi").toBool(),"Profile ROI output was not opened as editable layers");
            check(profileRoi.state()[0].crop==cv::Rect(5,6,60,50)&&profileRoi.state()[1].crop==cv::Rect(10,8,55,45)&&profileRoi.state()[1].x==60,"Editable profile ROI geometry differs from historical merge");
            check(profileRoi.findChild<QLabel*>("mosaicStatus")->text().contains("ROI"),"Editable profile ROI explanation missing");
            auto editedRoi=profileRoi.state();editedRoi[1].x+=17;profileRoi.setState(editedRoi);
            QTest::mouseClick(profileRoi.findChild<QPushButton*>("mosaicRestoreRoi"),Qt::LeftButton);waitFor(profileRoi);
            check(profileRoi.state()[1].x==60&&profileRoi.property("mergeStrategy").toString()=="profile-roi","Profile ROI could not be restored after a manual edit");
            profileRoi.saveProject(temp.path()+(it?"/profile-roi-it.json":"/profile-roi-en.json"));
            MosaicDialog automatic(it);automatic.show();automatic.appendFiles({temp.path()+"/auto-a.png",temp.path()+"/auto-b.png"});waitFor(automatic);
            auto* automaticButton=automatic.findChild<QPushButton*>("mosaicAutoOverlap");check(automaticButton&&automaticButton->isEnabled(),"Automatic overlap control missing");
            automatic.findChild<QListWidget*>("mosaicLayers")->setCurrentRow(0);
            bool sawCancellation=false;QTimer cancelAnswer;
            QObject::connect(&cancelAnswer,&QTimer::timeout,[&]{for(auto* widget:QApplication::topLevelWidgets())if(auto* box=qobject_cast<QMessageBox*>(widget)){sawCancellation=box->text().contains("overlap");box->button(QMessageBox::Cancel)->click();cancelAnswer.stop();}});cancelAnswer.start(10);
            automatic.autoAlign(true);waitFor(automatic);cancelAnswer.stop();
            check(sawCancellation&&!automatic.property("autoOverlapApplied").toBool(),"Automatic proposal cancellation was not honoured");
            check(automatic.state()[1].x==0&&automatic.state()[1].y==0,"Cancelled automatic proposal changed geometry");
            bool sawEvidence=false;QTimer applyAnswer;
            QObject::connect(&applyAnswer,&QTimer::timeout,[&]{for(auto* widget:QApplication::topLevelWidgets())if(auto* box=qobject_cast<QMessageBox*>(widget)){sawEvidence=box->text().contains("inlier")&&box->text().contains("confidence");box->button(QMessageBox::Apply)->click();applyAnswer.stop();}});applyAnswer.start(10);
            automatic.autoAlign(true);waitFor(automatic);applyAnswer.stop();
            check(sawEvidence,"Automatic proposal did not show registration evidence");
            check(std::abs(automatic.state()[1].x-230)<=2&&std::abs(automatic.state()[1].y-65)<=2,"GUI automatic overlap proposal incorrect");
            check(automatic.property("autoOverlapApplied").toBool()&&automatic.property("autoOverlapAccepted").toInt()==1,"Automatic overlap application evidence missing");
            check(automatic.findChild<QLabel*>("mosaicStatus")->text().contains(it?"applicata":"applied"),"Automatic overlap review message missing");
            automatic.undo();waitFor(automatic);check(automatic.state()[1].x==0&&automatic.state()[1].y==0,"Automatic overlap is not undoable");
            automatic.redo();waitFor(automatic);check(std::abs(automatic.state()[1].x-230)<=2,"Automatic overlap redo failed");
            automatic.saveProject(temp.path()+(it?"/automatic-it.json":"/automatic-en.json"));
            if(it&&argc>1){MosaicDialog demo(it);demo.appendDemo(QString::fromLocal8Bit(argv[1]));waitFor(demo);check(demo.state().size()==2,"Real six-JPEG editable demo missing");auto real=orbyx::renderMosaic(demo.state());check(real.image.size()==cv::Size(1428,1428),"Editable real-demo canvas incorrect");demo.show();QTest::qWait(30);if(qEnvironmentVariableIsSet("ORBYX_MOSAIC_SCREENSHOT"))demo.grab().save(qEnvironmentVariable("ORBYX_MOSAIC_SCREENSHOT"));demo.saveProject(temp.path()+"/demo.json");demo.close();}
            dialog.close();restored.close();channels.close();profileRoi.close();automatic.close();
        }
        std::cout<<"EN/IT multi-layer geometry, drag, undo, colour, project and full-resolution export passed\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
