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
        cv::Mat bordered(180,240,CV_16UC3,cv::Scalar(1200,1200,1200)),borderContent(120,170,CV_16UC3);cv::randu(borderContent,cv::Scalar::all(4000),cv::Scalar::all(50000));borderContent.copyTo(bordered(cv::Rect(30,35,170,120)));
        check(cv::imwrite((root/"bordered.png").string(),bordered),"Bordered fixture write");
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
            auto* blendMode=dialog.findChild<QComboBox*>("mosaicBlendMode");auto* opacity=dialog.findChild<QDoubleSpinBox*>("mosaic_opacity");auto* featherControl=dialog.findChild<QDoubleSpinBox*>("mosaic_feather");
            check(blendMode&&blendMode->count()==3&&opacity&&featherControl,"Explicit blending modes missing");blendMode->setCurrentIndex(0);waitFor(dialog);
            check(dialog.state()[2].blend==orbyx::MosaicBlend::Replace&&!opacity->isEnabled()&&!featherControl->isEnabled(),"No-blending mode was not applied explicitly");
            blendMode->setCurrentIndex(1);waitFor(dialog);check(dialog.state()[2].blend==orbyx::MosaicBlend::Feather&&opacity->isEnabled()&&featherControl->isEnabled(),"Feather mode did not restore visual controls");
            blendMode->setCurrentIndex(2);waitFor(dialog);check(dialog.state()[2].blend==orbyx::MosaicBlend::Seam&&!opacity->isEnabled()&&featherControl->isEnabled(),"Minimum-cost seam mode was not applied explicitly");
            blendMode->setCurrentIndex(1);waitFor(dialog);
            gamma->setValue(1.25);waitFor(dialog);dialog.applyColoursToAll();waitFor(dialog);
            for(const auto& layer:dialog.state())check(layer.gamma==1.25,"Colour application not applied to all portions");
            check(dialog.findChild<QPushButton*>("mosaicMatchColour"),"Visual colour-match action missing");const auto beforeCompensation=dialog.state()[2];orbyx::VisualCompensation compensation;compensation.layer=beforeCompensation;compensation.layer.exposure+=.25;compensation.layer.red*=1.1;compensation.samples=120;compensation.exposureDelta=.25;compensation.redMultiplier=1.1;
            dialog.applyVisualCompensation(compensation,2,1);waitFor(dialog);check(dialog.state()[2].exposure==beforeCompensation.exposure+.25&&dialog.property("mergeStrategy").toString()=="visual-compensation"&&dialog.property("compensationSamples").toULongLong()==120,"Visual compensation was not applied explicitly");dialog.undo();waitFor(dialog);check(dialog.state()[2].exposure==beforeCompensation.exposure&&dialog.state()[2].red==beforeCompensation.red,"Visual compensation is not undoable");
            auto* canvas=dynamic_cast<MosaicCanvas*>(dialog.findChild<QLabel*>("mosaicCanvas"));check(canvas&&!canvas->pixmap().isNull(),"Composite preview missing");
            // Non-destructive exclude mask control: editing the spin boxes updates
            // the selected layer, keeps the preview valid and is stored in the project.
            auto* excludeX=dialog.findChild<QDoubleSpinBox*>("mosaic_excludeX");auto* excludeY=dialog.findChild<QDoubleSpinBox*>("mosaic_excludeY");
            auto* excludeW=dialog.findChild<QDoubleSpinBox*>("mosaic_excludeW");auto* excludeH=dialog.findChild<QDoubleSpinBox*>("mosaic_excludeH");
            check(excludeX&&excludeY&&excludeW&&excludeH,"Exclude mask controls missing");
            excludeX->setValue(10);excludeY->setValue(10);excludeW->setValue(20);excludeH->setValue(15);waitFor(dialog);
            check(dialog.state()[2].exclude==cv::Rect(10,10,20,15),"Exclude mask control not applied to the selected layer");
            check(!canvas->pixmap().isNull(),"Preview missing after exclude mask");
            check(orbyx::mosaicProject(dialog.state()).find("exclude")!=std::string::npos,"Exclude mask not stored in the project");
            excludeW->setValue(0);excludeH->setValue(0);waitFor(dialog);
            check(dialog.state()[2].exclude.width==0&&dialog.state()[2].exclude.height==0,"Exclude mask was not cleared");
            PolygonMaskDialog polygonDialog(it,dialog.state()[2]);polygonDialog.show();auto* polygonCanvas=dynamic_cast<PolygonMaskCanvas*>(polygonDialog.findChild<QLabel*>("polygonMaskCanvas"));auto* polygonApply=polygonDialog.findChild<QPushButton*>("polygonMaskApply");auto* polygonMode=polygonDialog.findChild<QComboBox*>("polygonMaskMode");
            check(polygonCanvas&&polygonApply&&polygonMode&&!polygonApply->isEnabled(),"Visual polygon mask editor missing");QTest::mouseClick(polygonCanvas,Qt::LeftButton,{},QPoint(10,10));QTest::mouseClick(polygonCanvas,Qt::LeftButton,{},QPoint(70,10));QTest::mouseClick(polygonCanvas,Qt::LeftButton,{},QPoint(10,60));QTest::qWait(10);check(polygonApply->isEnabled(),"Polygon mask editor did not accept three visual vertices");polygonMode->setCurrentIndex(1);const auto visualMask=polygonDialog.mask();check(visualMask.include&&visualMask.points.size()==3,"Polygon mask editor lost mode or vertices");polygonDialog.close();
            dialog.applyPolygonMask(visualMask,2);waitFor(dialog);check(dialog.state()[2].masks.size()==1&&dialog.property("mergeStrategy").toString()=="manual-mask","Visual polygon mask was not applied");dialog.undo();waitFor(dialog);check(dialog.state()[2].masks.empty(),"Polygon mask is not undoable");dialog.redo();waitFor(dialog);check(dialog.state()[2].masks.size()==1,"Polygon mask redo failed");dialog.clearPolygonMasks(2);waitFor(dialog);check(dialog.state()[2].masks.empty(),"Polygon masks could not be cleared");dialog.undo();waitFor(dialog);check(dialog.state()[2].masks.size()==1,"Clearing polygon masks is not undoable");
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
            std::vector<orbyx::ControlPointPair> clusteredPoints;for(const cv::Point2d point:std::vector<cv::Point2d>{{5,5},{12,5},{5,12},{12,12}})clusteredPoints.push_back({point,{point.x+2,point.y+3}});
            ControlPointsDialog clusteredDialog(it,movingBefore,dialog.state()[1]);clusteredDialog.setPairs(clusteredPoints);check(clusteredDialog.solve()&&clusteredDialog.property("spatialWarning").toBool(),"Concentrated control points did not warn the user");check(clusteredDialog.findChild<QLabel*>("controlPointSummary")->text().contains(it?"concentrati":"concentrated"),"Spatial concentration warning is missing from the summary");
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
            // M2-07: a comparable ROI / current preview is available; it renders both
            // strategies side by side without altering the ROI baseline or the layers.
            auto* compareButton=profileRoi.findChild<QPushButton*>("mosaicCompareStrategies");
            check(compareButton&&compareButton->isVisible()&&!compareButton->accessibleName().isEmpty(),"Compare ROI/current control missing");
            auto forCompare=profileRoi.state();forCompare[1].x+=23;profileRoi.setState(forCompare);waitFor(profileRoi);
            const auto currentOnly=orbyx::renderMosaic(profileRoi.state(),1200).image;
            const auto comparison=profileRoi.strategyComparisonImage(1200);
            check(comparison.type()==CV_8UC3&&comparison.rows>0&&comparison.cols>=currentOnly.cols+10,"Strategy comparison is not a side-by-side of two strategies");
            const int leftWidth=comparison.cols-10-currentOnly.cols;check(leftWidth>0,"Comparison left (ROI) strategy missing");
            const int sharedRows=std::min(comparison.rows,currentOnly.rows);
            cv::Mat leftHalf=comparison(cv::Rect(0,0,leftWidth,sharedRows)),rightHalf=comparison(cv::Rect(comparison.cols-currentOnly.cols,0,currentOnly.cols,sharedRows));
            check(cv::countNonZero(leftHalf.reshape(1))>0&&cv::countNonZero(rightHalf.reshape(1))>0,"A strategy half of the comparison is blank");
            check(profileRoi.state()[1].x==83,"Comparison altered the current layers");
            if(qEnvironmentVariableIsSet("ORBYX_COMPARE_SCREENSHOT"))cv::imwrite(qEnvironmentVariable("ORBYX_COMPARE_SCREENSHOT").toStdString(),comparison);
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
            MosaicDialog cropEditor(it);cropEditor.show();cropEditor.appendFiles({temp.path()+"/bordered.png"});waitFor(cropEditor);
            auto cropState=cropEditor.state();cropState[0].x=13;cropState[0].y=27;cropState[0].scale=1.5;cropState[0].angle=30;cropEditor.setState(cropState);waitFor(cropEditor);
            auto* cropButton=cropEditor.findChild<QPushButton*>("mosaicCropProposal");check(cropButton&&cropButton->isEnabled(),"Uniform-border crop proposal control missing");
            const auto expectedCrop=orbyx::proposeUniformBorderCrop(cropEditor.state()[0]);const auto unchangedCrop=orbyx::mosaicProject(cropEditor.state());
            bool sawCropCancel=false;QTimer cancelCrop;
            QObject::connect(&cancelCrop,&QTimer::timeout,[&]{for(auto* widget:QApplication::topLevelWidgets())if(auto* box=qobject_cast<QMessageBox*>(widget)){sawCropCancel=box->text().contains("%");box->button(QMessageBox::Cancel)->click();cancelCrop.stop();}});cancelCrop.start(10);
            cropEditor.proposeCrop(true);waitFor(cropEditor);cancelCrop.stop();
            check(sawCropCancel&&!cropEditor.property("cropProposalApplied").toBool()&&orbyx::mosaicProject(cropEditor.state())==unchangedCrop,"Crop proposal cancellation changed the layer");
            bool sawCropEvidence=false;QTimer applyCrop;
            QObject::connect(&applyCrop,&QTimer::timeout,[&]{for(auto* widget:QApplication::topLevelWidgets())if(auto* box=qobject_cast<QMessageBox*>(widget)){sawCropEvidence=box->text().contains("%");box->button(QMessageBox::Apply)->click();applyCrop.stop();}});applyCrop.start(10);
            cropEditor.proposeCrop(true);waitFor(cropEditor);applyCrop.stop();
            const double cropDx=expectedCrop.proposed.x-expectedCrop.original.x,cropDy=expectedCrop.proposed.y-expectedCrop.original.y,cropAngle=30*CV_PI/180.;
            check(sawCropEvidence&&cropEditor.state()[0].crop==expectedCrop.proposed&&cropEditor.property("mergeStrategy").toString()=="manual-crop","Confirmed crop proposal was not applied");
            check(std::abs(cropEditor.state()[0].x-(13+1.5*(std::cos(cropAngle)*cropDx-std::sin(cropAngle)*cropDy)))<1e-8&&std::abs(cropEditor.state()[0].y-(27+1.5*(std::sin(cropAngle)*cropDx+std::cos(cropAngle)*cropDy)))<1e-8,"Crop proposal moved retained scene content");
            cropEditor.undo();waitFor(cropEditor);check(cropEditor.state()[0].crop.area()==0&&cropEditor.state()[0].x==13&&cropEditor.state()[0].y==27,"Crop proposal is not undoable");
            cropEditor.saveProject(temp.path()+(it?"/crop-it.json":"/crop-en.json"));
            if(it&&argc>1){MosaicDialog demo(it);demo.appendDemo(QString::fromLocal8Bit(argv[1]));waitFor(demo);check(demo.state().size()==2,"Real six-JPEG editable demo missing");auto real=orbyx::renderMosaic(demo.state());check(real.image.size()==cv::Size(1428,1428),"Editable real-demo canvas incorrect");demo.show();QTest::qWait(30);if(qEnvironmentVariableIsSet("ORBYX_MOSAIC_SCREENSHOT"))demo.grab().save(qEnvironmentVariable("ORBYX_MOSAIC_SCREENSHOT"));demo.saveProject(temp.path()+"/demo.json");demo.close();}
            dialog.close();restored.close();channels.close();profileRoi.close();automatic.close();cropEditor.close();
        }
        std::cout<<"EN/IT multi-layer geometry, drag, undo, colour, project and full-resolution export passed\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
