// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "orbyx/compositor.hpp"
#include "orbyx/engine.hpp"
#include "control_points_dialog.hpp"
#include "polygon_mask_dialog.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QPixmap>
#include <QPolygonF>
#include <QPushButton>
#include <QSaveFile>
#include <QSlider>
#include <QScrollArea>
#include <QScrollBar>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QVBoxLayout>
#include <QDir>
#include <opencv2/imgproc.hpp>
#include <functional>
#include <memory>
#include <algorithm>
#include <cmath>
#include <stdexcept>

class MosaicCanvas final:public QLabel {
    QPointF pressed;
public:
    std::function<void(QPointF)> moved;
    std::function<void(QPointF)> nudged;
    using QLabel::QLabel;
protected:
    void mousePressEvent(QMouseEvent* e)override {setFocus(Qt::MouseFocusReason);pressed=e->position();}
    void mouseReleaseEvent(QMouseEvent* e)override {if(e->button()==Qt::LeftButton&&moved)moved(e->position()-pressed);}
    void keyPressEvent(QKeyEvent* e)override {
        QPointF delta;
        const double step=e->modifiers().testFlag(Qt::ShiftModifier)?10.:1.;
        if(e->key()==Qt::Key_Left)delta.setX(-step);else if(e->key()==Qt::Key_Right)delta.setX(step);
        else if(e->key()==Qt::Key_Up)delta.setY(-step);else if(e->key()==Qt::Key_Down)delta.setY(step);
        else {QLabel::keyPressEvent(e);return;}
        if(nudged)nudged(delta);e->accept();
    }
};

class MosaicDialog final:public QDialog {
    bool it=false,busy=false,updating=false,dirty=false;
    QWidget* tools{};QListWidget* list{};MosaicCanvas* canvas{};QLabel* status{};
    QScrollArea* previewView{};QComboBox* colourMode{};QComboBox* inspectionMode{};QComboBox* blendMode{};QSlider* swipePosition{};QDoubleSpinBox* zoom{};QCheckBox* visible{};QCheckBox* blink{};
    QLabel* histogramLabel_{};QLabel* clippingLabel_{}; // Composite histogram and highlight/shadow clipping (M2-11).
    std::vector<QDoubleSpinBox*> controls;
    std::vector<orbyx::MosaicLayer> layers;
    std::vector<orbyx::MosaicLayer> profileRoiBaseline;
    std::vector<std::vector<orbyx::MosaicLayer>> undoStates,redoStates;
    QTimer previewTimer,blinkTimer;QImage previewImage;double previewRatio=1;bool blinkHidden=false;
    QString workingFolder,persistentNotice;std::vector<QPolygonF> outlines;
    QString t(const char* en,const char* italian)const{return QString::fromUtf8(it?italian:en);}
    QPushButton* button(const char* id,const QString& text,QLayout* layout,std::function<void()> action) {
        auto* b=new QPushButton(text);b->setObjectName(id);b->setAccessibleName(text);layout->addWidget(b);
        connect(b,&QPushButton::clicked,this,std::move(action));return b;
    }
    void remember(){dirty=true;undoStates.push_back(layers);if(undoStates.size()>50)undoStates.erase(undoStates.begin());redoStates.clear();}
    void schedule(){previewTimer.start();}
    void refreshList(int selected=-1) {
        updating=true;list->clear();
        for(size_t i=0;i<layers.size();++i) {
            auto label=QString::number(i+1)+" · "+QFileInfo(QString::fromStdString(layers[i].image.string())).fileName();
            if(!layers[i].visible)label+=" ("+t("hidden","nascosto")+")";
            list->addItem(label);
        }
        updating=false;list->setCurrentRow(selected<0?int(layers.size())-1:selected);readControls();schedule();
    }
    void readControls(){
        int row=list->currentRow();if(row<0||row>=int(layers.size()))return;
        updating=true;const auto& l=layers[size_t(row)];
        double values[]={l.x,l.y,l.scale,l.angle,l.opacity,l.feather,double(l.crop.x),double(l.crop.y),double(l.crop.width),double(l.crop.height),l.black,l.white,l.gamma,l.exposure,l.saturation,l.red,l.green,l.blue,double(l.exclude.x),double(l.exclude.y),double(l.exclude.width),double(l.exclude.height)};
        for(size_t i=0;i<controls.size();++i)controls[i]->setValue(values[i]);
        visible->setChecked(l.visible);blendMode->setCurrentIndex(l.blend==orbyx::MosaicBlend::Replace?0:l.blend==orbyx::MosaicBlend::Seam?2:l.blend==orbyx::MosaicBlend::Multiband?3:1);controls[4]->setEnabled(l.blend==orbyx::MosaicBlend::Feather);controls[5]->setEnabled(l.blend!=orbyx::MosaicBlend::Replace);updating=false;
    }
    void edit(){
        if(updating||busy)return;int row=list->currentRow();if(row<0)return;
        remember();auto& l=layers[size_t(row)];
        double* values[]={&l.x,&l.y,&l.scale,&l.angle,&l.opacity,&l.feather};
        for(int i=0;i<6;++i)*values[i]=controls[size_t(i)]->value();
        l.crop={int(controls[6]->value()),int(controls[7]->value()),int(controls[8]->value()),int(controls[9]->value())};
        double* colours[]={&l.black,&l.white,&l.gamma,&l.exposure,&l.saturation,&l.red,&l.green,&l.blue};
        for(int i=0;i<8;++i)*colours[i]=controls[size_t(i+10)]->value();
        l.exclude={int(controls[18]->value()),int(controls[19]->value()),int(controls[20]->value()),int(controls[21]->value())};
        l.visible=visible->isChecked();l.blend=blendMode->currentIndex()==0?orbyx::MosaicBlend::Replace:blendMode->currentIndex()==2?orbyx::MosaicBlend::Seam:blendMode->currentIndex()==3?orbyx::MosaicBlend::Multiband:orbyx::MosaicBlend::Feather;controls[4]->setEnabled(l.blend==orbyx::MosaicBlend::Feather);controls[5]->setEnabled(l.blend!=orbyx::MosaicBlend::Replace);schedule();
    }
    void record(const QString& text){
        QDir().mkpath(workingFolder);QFile file(workingFolder+"/compositor.log");
        QFile::setPermissions(workingFolder,QFileDevice::ReadOwner|QFileDevice::WriteOwner|QFileDevice::ExeOwner);
        if(file.open(QIODevice::WriteOnly|QIODevice::Append)){file.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner);file.write((QDateTime::currentDateTimeUtc().toString(Qt::ISODate)+" "+text+"\n").toUtf8());}
    }
    void run(std::function<void()> operation,std::function<void()> success){
        if(busy)return;busy=true;tools->setEnabled(false);canvas->setEnabled(false);previewTimer.stop();
        status->setText(t("Processing…","Elaborazione…"));
        auto error=std::make_shared<QString>();auto* thread=QThread::create([operation=std::move(operation),error]{
            try{operation();}catch(const std::exception& e){*error=QString::fromUtf8(e.what());}
        });
        thread->setParent(this);
        connect(thread,&QThread::finished,this,[this,error,success=std::move(success),thread]{
            busy=false;tools->setEnabled(true);canvas->setEnabled(true);thread->deleteLater();
            if(!error->isEmpty()){record("ERROR: "+*error);status->setText(t("Error: ","Errore: ")+*error);return;}
            record("Operation completed");status->setText(t("Ready","Pronto"));success();
        });thread->start();
    }
    void fitPreview(){if(previewImage.isNull()||!previewView)return;double factor=std::min({1.,double(previewView->viewport()->width()-12)/previewImage.width(),double(previewView->viewport()->height()-12)/previewImage.height()});zoom->setValue(std::clamp(factor*100.,25.,400.));display();}
    void zoomToOverlap(){
        const int selected=list->currentRow();if(previewImage.isNull()||selected<0||layers.size()<2||selected>=int(outlines.size()))return;
        const int comparison=selected>0?selected-1:selected+1;if(comparison>=int(outlines.size()))return;
        const auto overlap=outlines[size_t(selected)].intersected(outlines[size_t(comparison)]);const auto area=overlap.boundingRect();
        if(area.isEmpty()||area.width()<1||area.height()<1){status->setText(t("The selected adjacent pair has no visible overlap.","La coppia adiacente selezionata non ha una sovrapposizione visibile."));setProperty("seamZoomed",false);return;}
        const double factor=std::min(double(previewView->viewport()->width()-24)/area.width(),double(previewView->viewport()->height()-24)/area.height());
        zoom->setValue(std::clamp(factor*100.,25.,400.));display();
        const auto centre=area.center()*zoom->value()/100.;previewView->horizontalScrollBar()->setValue(int(centre.x()-previewView->viewport()->width()/2.));previewView->verticalScrollBar()->setValue(int(centre.y()-previewView->viewport()->height()/2.));
        setProperty("seamZoomed",true);status->setText(t("Overlap centred for seam inspection.","Sovrapposizione centrata per il controllo della giunzione."));
    }
    void display(){
        if(previewImage.isNull())return;auto p=QPixmap::fromImage(previewImage);
        p=p.scaled(std::max(1,int(p.width()*zoom->value()/100.)),std::max(1,int(p.height()*zoom->value()/100.)),Qt::KeepAspectRatio,Qt::SmoothTransformation);
        const int row=list->currentRow();
        if(row>=0&&row<int(outlines.size())&&layers[size_t(row)].visible&&!(blink&&blink->isChecked()&&blinkHidden)){QPainter painter(&p);painter.setPen(QPen(QColor("#00d4ff"),2,Qt::DashLine));QPolygonF polygon;for(const auto& point:outlines[size_t(row)])polygon<<point*(zoom->value()/100.);painter.drawPolygon(polygon);}
        canvas->setPixmap(p);canvas->resize(p.size());
    }
    // M2-11: composite luminance histogram and highlight/shadow clipping within the
    // covered area, so the user can judge exposure/colour before export.
    void updateHistogram(const orbyx::MosaicResult& rendered){
        if(!histogramLabel_||!clippingLabel_)return;
        const cv::Mat& img=rendered.image;
        if(img.empty()||img.type()!=CV_8UC3){histogramLabel_->clear();clippingLabel_->clear();return;}
        cv::Mat coverage=cv::Mat::zeros(img.size(),CV_8U);
        for(const auto& poly:rendered.outlines){std::vector<cv::Point> pts;for(const auto& pt:poly)pts.emplace_back(cvRound(pt.x),cvRound(pt.y));if(pts.size()>=3)cv::fillConvexPoly(coverage,pts,cv::Scalar(255));}
        const long covered=cv::countNonZero(coverage);
        long highlight=0,shadow=0;int hist[256]={0};int maxBin=1;
        cv::Mat gray;cv::cvtColor(img,gray,cv::COLOR_BGR2GRAY);
        for(int y=0;y<img.rows;++y){const auto* c=coverage.ptr<uchar>(y);const auto* p=img.ptr<cv::Vec3b>(y);const auto* g=gray.ptr<uchar>(y);
            for(int x=0;x<img.cols;++x)if(c[x]){const auto& v=p[x];if(v[0]>=255||v[1]>=255||v[2]>=255)++highlight;if(v[0]==0&&v[1]==0&&v[2]==0)++shadow;if(++hist[g[x]]>maxBin)maxBin=hist[g[x]];}}
        setProperty("coveredPixels",qlonglong(covered));setProperty("highlightClippedPixels",qlonglong(highlight));setProperty("shadowClippedPixels",qlonglong(shadow));
        QImage bars(256,74,QImage::Format_RGB888);bars.fill(QColor(16,20,28));
        {QPainter painter(&bars);painter.setPen(Qt::NoPen);painter.setBrush(QColor(90,200,160));
         for(int b=0;b<256;++b){const int h=int(std::lround(70.0*hist[b]/maxBin));if(h>0)painter.drawRect(b,74-h,1,h);}
         painter.setPen(QColor(120,160,255));painter.drawLine(0,0,0,73);painter.setPen(QColor(255,120,120));painter.drawLine(255,0,255,73);}
        histogramLabel_->setPixmap(QPixmap::fromImage(bars));
        const double hp=covered?100.0*highlight/covered:0.,sp=covered?100.0*shadow/covered:0.;
        clippingLabel_->setText(t("Clipping — highlights %1% · shadows %2%","Clipping — alte luci %1% · ombre %2%").arg(hp,0,'f',2).arg(sp,0,'f',2));
    }
    void preview(){
        if(layers.empty()){canvas->clear();previewImage={};status->setText(t("Add portions or aligned channels to begin.","Aggiungi porzioni o canali allineati per iniziare."));return;}
        auto state=layers;const int selected=list->currentRow();
        if(blink&&blink->isChecked()&&selected>=0&&selected<int(state.size()))state[size_t(selected)].visible=!blinkHidden;
        const int inspection=inspectionMode?inspectionMode->currentIndex():0;
        const int swipe=swipePosition?swipePosition->value():50;
        const bool compare=inspection>0&&state.size()>1&&selected>=0;
        struct PreviewResult{orbyx::MosaicResult mosaic;int comparison=-1;int differencePixels=0;int inspection=0;};auto result=std::make_shared<PreviewResult>();
        run([state,result,selected,inspection,compare,swipe]{
            result->inspection=inspection;
            if(!compare){result->mosaic=orbyx::renderMosaic(state,1200);return;}
            const int comparison=selected>0?selected-1:selected+1;result->comparison=comparison;
            auto pair=state;for(auto& layer:pair)layer.visible=false;
            pair[size_t(selected)].visible=true;pair[size_t(comparison)].visible=true;
            auto selectedOnly=pair,comparisonOnly=pair;
            selectedOnly[size_t(comparison)].opacity=0;comparisonOnly[size_t(selected)].opacity=0;
            auto a=orbyx::renderMosaic(selectedOnly,1200),b=orbyx::renderMosaic(comparisonOnly,1200);
            if(a.image.size()!=b.image.size())throw std::runtime_error("Difference preview geometry mismatch");
            cv::Mat maskA=cv::Mat::zeros(a.image.size(),CV_8U),maskB=maskA.clone();
            auto polygon=[](const std::array<cv::Point2d,4>& points){std::vector<cv::Point> out;for(const auto& point:points)out.emplace_back(cvRound(point.x),cvRound(point.y));return out;};
            cv::fillConvexPoly(maskA,polygon(a.outlines[size_t(selected)]),cv::Scalar(255));
            cv::fillConvexPoly(maskB,polygon(a.outlines[size_t(comparison)]),cv::Scalar(255));
            cv::bitwise_and(maskA,maskB,maskA);
            if(inspection==1){
                cv::Mat delta,gray,scaled=cv::Mat::zeros(a.image.size(),CV_8U),heat;
                cv::absdiff(a.image,b.image,delta);cv::cvtColor(delta,gray,cv::COLOR_BGR2GRAY);
                if(cv::countNonZero(maskA)>0){double low=0,high=0;cv::minMaxLoc(gray,&low,&high,nullptr,nullptr,maskA);if(high>low)cv::normalize(gray,scaled,0,255,cv::NORM_MINMAX,CV_8U,maskA);else gray.copyTo(scaled,maskA);}
                result->differencePixels=cv::countNonZero(scaled);
                cv::applyColorMap(scaled,heat,cv::COLORMAP_TURBO);heat.setTo(cv::Scalar::all(0),maskA==0);a.image=heat;
            }else{
                cv::Mat combined;cv::add(a.image,b.image,combined);
                const int divider=combined.cols*swipe/100;
                for(int y=0;y<combined.rows;++y){auto* destination=combined.ptr<cv::Vec3b>(y);const auto* fromA=a.image.ptr<cv::Vec3b>(y);const auto* fromB=b.image.ptr<cv::Vec3b>(y);const auto* overlap=maskA.ptr<uchar>(y);for(int x=0;x<combined.cols;++x)if(overlap[x])destination[x]=inspection==2?(((x/32+y/32)&1)?fromA[x]:fromB[x]):(x<divider?fromA[x]:fromB[x]);}
                if(inspection==3)cv::line(combined,{divider,0},{divider,combined.rows-1},cv::Scalar(0,212,255),1,cv::LINE_AA);
                a.image=combined;
            }
            result->mosaic=std::move(a);
        },[this,result]{
            auto& rendered=result->mosaic;auto& m=rendered.image;QImage image(m.data,m.cols,m.rows,int(m.step),QImage::Format_BGR888);
            const bool first=previewImage.isNull();previewImage=image.copy();previewRatio=rendered.pixelsPerSourcePixel;outlines.clear();for(const auto& points:rendered.outlines){QPolygonF polygon;for(const auto& point:points)polygon<<QPointF(point.x,point.y);outlines.push_back(polygon);}if(first)fitPreview();else display();
            setProperty("differenceComparison",result->comparison);
            setProperty("differencePixels",result->differencePixels);
            setProperty("inspectionRendered",result->inspection);
            setProperty("swipeRendered",swipePosition?swipePosition->value():50);
            setProperty("seamFallbacks",rendered.seamFallbacks);
            if(result->inspection==0)updateHistogram(rendered); // Composite view only.
            if(rendered.seamFallbacks>0)status->setText(t("No continuous optimized seam was found for one or more layers; the saved feather settings were used.","Non è stata trovata una giunzione ottimizzata continua per uno o più livelli; sono state usate le impostazioni di sfumatura salvate."));
            else status->setText(persistentNotice.isEmpty()?t("Preview only: export uses original resolution and 8/16-bit depth. Drag to move the selected layer.","Solo anteprima: l’esportazione usa risoluzione originale e profondità 8/16 bit. Trascina per spostare il livello selezionato."):persistentNotice);
        });
    }
    void addFiles(){
        auto files=QFileDialog::getOpenFileNames(this,t("Add PNG portions","Aggiungi porzioni PNG"),{},"PNG (*.png)");if(files.isEmpty())return;
        appendFiles(files);
    }
    void colourFolder(){
        auto folder=QFileDialog::getExistingDirectory(this,t("Select one aligned VIS product (three channels)","Seleziona un prodotto VIS allineato (tre canali)"));
        if(folder.isEmpty())return;
        const int mode=colourMode->currentIndex();
        if(mode<=1)appendChannels(folder,mode==0);else appendSingleChannel(folder,mode-2);
    }
    void saveProjectDialog(){
        if(layers.empty())return;auto name=QFileDialog::getSaveFileName(this,t("Save editable project","Salva progetto modificabile"),"mosaic.orbyx.json","ORBYX (*.json)");if(name.isEmpty())return;
        if(!name.endsWith(".json"))name+=".json";
        try{saveProject(name);status->setText(t("Project saved. Keep the source files at their original locations.","Progetto salvato. Conserva i file sorgenti nelle posizioni originali."));}
        catch(const std::exception& e){status->setText(QString::fromUtf8(e.what()));}
    }
    void loadProjectDialog(){
        if(dirty){auto answer=QMessageBox::question(this,t("Unsaved project","Progetto non salvato"),t("Save your changes before opening another project?","Salvare le modifiche prima di aprire un altro progetto?"),QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Cancel);if(answer==QMessageBox::Cancel)return;if(answer==QMessageBox::Save){saveProjectDialog();if(dirty)return;}}
        auto name=QFileDialog::getOpenFileName(this,t("Open project","Apri progetto"),{},"ORBYX (*.json)");if(name.isEmpty())return;
        try{loadProject(name);}catch(const std::exception& e){status->setText(QString::fromUtf8(e.what()));}
    }
    void exportDialog(){
        if(layers.empty())return;auto name=QFileDialog::getSaveFileName(this,t("Export new mosaic PNG","Esporta nuovo PNG del mosaico"),"mosaic.png","PNG (*.png)");if(name.isEmpty())return;
        if(!name.endsWith(".png"))name+=".png";exportTo(name);
    }
    void restoreProfileRoi(){
        if(busy)return;
        if(profileRoiBaseline.empty()){
            persistentNotice=t("No profile ROI starting point is available for this project.","Per questo progetto non è disponibile un punto di partenza ROI del profilo.");
            status->setText(persistentNotice);return;
        }
        remember();layers=profileRoiBaseline;setProperty("mergeStrategy","profile-roi");
        persistentNotice=t("Profile ROI restored. The historical geometry is again the editable starting point.","ROI del profilo ripristinata. La geometria storica è di nuovo il punto di partenza modificabile.");
        refreshList();status->setText(persistentNotice);record("Profile ROI baseline restored");
    }
    // M2-07: compare the historical profile-ROI geometry with the current strategy
    // (visual/manual) side by side, then keep one explicitly. Nothing is applied or
    // exported without the user's choice, and the ROI baseline is never altered.
    // M2-12: quality indicators for the current composition. Sets properties only;
    // qualityDialog() shows them. Evidence only — never declares an image perfect.
    orbyx::MosaicQuality computeQuality(){
        const orbyx::MosaicQuality q=orbyx::mosaicQuality(layers);
        setProperty("qualityCovered",qulonglong(q.coveredPixels));
        setProperty("qualityEmptyFraction",q.emptyFraction);
        setProperty("qualityOverlapFraction",q.overlapFraction);
        setProperty("qualityHighlightClipped",q.highlightClipped);
        setProperty("qualityShadowClipped",q.shadowClipped);
        setProperty("qualityExtremeTransforms",q.extremeTransforms);
        setProperty("qualitySeamDifference",q.seamDifference);
        return q;
    }
    // Lower is better: gaps, visible seams and clipping are the concerns.
    static double qualityConcern(const orbyx::MosaicQuality& q){return q.emptyFraction+q.seamDifference+q.highlightClipped+q.shadowClipped;}
    void qualityDialog(){
        if(busy||layers.empty())return;
        orbyx::MosaicQuality q;
        try{q=computeQuality();}catch(const std::exception& e){status->setText(QString::fromUtf8(e.what()));return;}
        const QString strategy=property("mergeStrategy").toString();
        QString recommendation=t("No profile ROI to compare; review the evidence and choose.","Nessuna ROI del profilo da confrontare; valuta le evidenze e scegli.");
        setProperty("qualityRecommended",QString());
        if(!profileRoiBaseline.empty()){
            // Compare the historical ROI strategy with the current one as evidence.
            try{
                const orbyx::MosaicQuality roi=orbyx::mosaicQuality(profileRoiBaseline);
                const double concernRoi=qualityConcern(roi),concernCur=qualityConcern(q);
                QString pick;if(std::abs(concernRoi-concernCur)<0.02)pick="ambiguous";else pick=concernRoi<concernCur?"profile-roi":"current";
                setProperty("qualityRecommended",pick);
                recommendation=pick=="ambiguous"
                    ? t("ROI and current are close: ambiguous — compare them and choose.","ROI e stato attuale sono simili: ambiguo — confrontali e scegli.")
                    : pick=="profile-roi"
                        ? t("On this evidence the profile ROI looks preferable; confirm before keeping it.","Su queste evidenze la ROI del profilo sembra preferibile; conferma prima di tenerla.")
                        : t("On this evidence the current strategy looks preferable; confirm before keeping it.","Su queste evidenze lo stato attuale sembra preferibile; conferma prima di tenerlo.");
            }catch(const std::exception&){}
        }
        const QString body=t("Strategy: %1\nEmpty/gap area: %2%\nOverlap coverage: %3%\nSeam difference: %4%\nHighlight clipping: %5%\nShadow clipping: %6%\nExtreme transforms: %7\n\n%8\n\nEvidence only — ORBYX does not declare an image perfect and does not change your project.",
                             "Strategia: %1\nAree vuote/mancanti: %2%\nCopertura sovrapposizione: %3%\nDifferenza lungo la giunzione: %4%\nClipping alte luci: %5%\nClipping ombre: %6%\nTrasformazioni estreme: %7\n\n%8\n\nSolo evidenze — ORBYX non dichiara perfetta un'immagine e non modifica il progetto.")
            .arg(strategy.isEmpty()?t("manual","manuale"):strategy)
            .arg(q.emptyFraction*100,0,'f',2).arg(q.overlapFraction*100,0,'f',1).arg(q.seamDifference*100,0,'f',2).arg(q.highlightClipped*100,0,'f',2).arg(q.shadowClipped*100,0,'f',2).arg(q.extremeTransforms).arg(recommendation);
        record(QString("Quality assessment: empty=%1 overlap=%2 seam=%3 highlightClip=%4 shadowClip=%5 extreme=%6 recommended=%7").arg(q.emptyFraction).arg(q.overlapFraction).arg(q.seamDifference).arg(q.highlightClipped).arg(q.shadowClipped).arg(q.extremeTransforms).arg(property("qualityRecommended").toString()));
        QMessageBox::information(this,t("Quality assessment","Valutazione qualità"),body);
    }
    void compareStrategies(){
        if(busy)return;
        if(profileRoiBaseline.empty()){
            persistentNotice=t("No profile ROI baseline is available to compare.","Non è disponibile una ROI del profilo da confrontare.");
            status->setText(persistentNotice);return;
        }
        cv::Mat combined;
        try{combined=strategyComparisonImage(1200);}
        catch(const std::exception& e){status->setText(QString::fromUtf8(e.what()));return;}
        QImage image(combined.data,combined.cols,combined.rows,int(combined.step),QImage::Format_BGR888);
        previewImage=image.copy();outlines.clear();fitPreview();
        setProperty("strategyComparisonShown",true);record("Strategy comparison shown: profile ROI vs current");
        QMessageBox box(QMessageBox::Question,t("Compare ROI / current","Confronta ROI / stato attuale"),
            t("Left: profile ROI. Right: current strategy. Keep which result?","Sinistra: ROI del profilo. Destra: strategia attuale. Quale risultato mantenere?"),QMessageBox::NoButton,this);
        auto* keepRoi=box.addButton(t("Keep profile ROI","Mantieni ROI del profilo"),QMessageBox::AcceptRole);
        box.addButton(t("Keep current","Mantieni stato attuale"),QMessageBox::RejectRole);box.setDefaultButton(keepRoi);box.exec();
        if(box.clickedButton()==keepRoi)restoreProfileRoi();
        else{status->setText(t("Kept the current strategy; nothing was changed.","Mantenuta la strategia attuale; nulla è stato modificato."));schedule();}
    }
    void controlPointsDialog(){
        const int moving=list->currentRow();if(busy||moving<0||layers.size()<2){status->setText(t("Select a moving layer and add at least two portions.","Seleziona un livello mobile e aggiungi almeno due porzioni."));return;}
        const int reference=moving>0?moving-1:moving+1;ControlPointsDialog dialog(it,layers[size_t(moving)],layers[size_t(reference)],this);
        if(dialog.exec()!=QDialog::Accepted)return;applyControlPoints(dialog.solution(),moving,reference);
    }
    void polygonMaskDialog(){const int row=list->currentRow();if(busy||row<0){status->setText(t("Select one layer first.","Seleziona prima un livello."));return;}try{PolygonMaskDialog dialog(it,layers[size_t(row)],this);if(dialog.exec()==QDialog::Accepted)applyPolygonMask(dialog.mask(),row);}catch(const std::exception& error){status->setText(t("Error: ","Errore: ")+QString::fromUtf8(error.what()));}}
    void visualCompensationDialog(){
        const int moving=list->currentRow();if(busy||moving<0||layers.size()<2){status->setText(t("Select a layer and add at least two portions.","Seleziona un livello e aggiungi almeno due porzioni."));return;}const int reference=moving>0?moving-1:moving+1;const auto state=layers;auto proposal=std::make_shared<orbyx::VisualCompensation>();
        run([state,moving,reference,proposal]{*proposal=orbyx::proposeVisualCompensation(state,size_t(moving),size_t(reference));},[this,proposal,moving,reference]{const auto message=t("Apply this visual-only match?\nSamples: %1\nExposure: %2 EV\nRGB multipliers: %3 / %4 / %5\n\nOriginal images and SatDump channels are not changed.","Applicare questa corrispondenza solo visuale?\nCampioni: %1\nEsposizione: %2 EV\nMoltiplicatori RGB: %3 / %4 / %5\n\nLe immagini originali e i canali SatDump non vengono modificati.").arg(proposal->samples).arg(proposal->exposureDelta,0,'f',3).arg(proposal->redMultiplier,0,'f',3).arg(proposal->greenMultiplier,0,'f',3).arg(proposal->blueMultiplier,0,'f',3);if(QMessageBox::question(this,t("Visual colour match","Corrispondenza colore visuale"),message,QMessageBox::Apply|QMessageBox::Cancel,QMessageBox::Cancel)==QMessageBox::Apply)applyVisualCompensation(*proposal,moving,reference);});
    }
protected:
    void done(int result)override{
        if(busy)return;
        if(dirty){auto answer=QMessageBox::question(this,t("Unsaved project","Progetto non salvato"),t("Save the editable project before closing? Exporting a PNG does not save the layers.","Salvare il progetto modificabile prima di chiudere? Esportare un PNG non salva i livelli."),QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Cancel);
            if(answer==QMessageBox::Cancel)return;if(answer==QMessageBox::Save){saveProjectDialog();if(dirty)return;}}
        QDialog::done(result);
    }
    void closeEvent(QCloseEvent* e)override{if(busy)e->ignore();else QDialog::closeEvent(e);}
public:
    explicit MosaicDialog(bool italian,QWidget* parent=nullptr):QDialog(parent),it(italian){
        setObjectName("mosaicDialog");setWindowTitle(t("Portion and colour compositor","Compositore porzioni e colore"));resize(1180,820);
        workingFolder=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/compositor/"+QUuid::createUuid().toString(QUuid::WithoutBraces);
        auto* outer=new QVBoxLayout(this);auto* body=new QHBoxLayout;outer->addLayout(body,1);
        tools=new QWidget;auto* panel=new QVBoxLayout(tools);auto* scroll=new QScrollArea;scroll->setWidgetResizable(true);scroll->setWidget(tools);scroll->setMinimumWidth(335);scroll->setMaximumWidth(400);body->addWidget(scroll);
        auto* note=new QLabel(t("Multiple portions, any direction. First align each portion’s channels. Automatic overlap proposes editable geometry; it is not georeferencing.","Più porzioni, in ogni direzione. Prima allinea i canali di ciascuna porzione. La sovrapposizione automatica propone una geometria modificabile; non è una georeferenziazione."));note->setWordWrap(true);panel->addWidget(note);
        button("mosaicDemo",t("Add editable demo portions","Aggiungi porzioni demo modificabili"),panel,[this]{appendDemo();});
        button("mosaicAdd",t("Add PNG portions…","Aggiungi porzioni PNG…"),panel,[this]{addFiles();});
        colourMode=new QComboBox;colourMode->setObjectName("mosaicColourMode");colourMode->addItems({"MSU-GS Natural Color (RGB321)","RGB123",t("Single channel — CH1 (grayscale)","Canale singolo — CH1 (scala di grigi)"),t("Single channel — CH2 (grayscale)","Canale singolo — CH2 (scala di grigi)"),t("Single channel — CH3 (grayscale)","Canale singolo — CH3 (scala di grigi)")});colourMode->setAccessibleName(t("Channel colour composition","Composizione colore dei canali"));panel->addWidget(colourMode);
        button("mosaicChannels",t("Add 3 aligned channels…","Aggiungi 3 canali allineati…"),panel,[this]{colourFolder();});
        list=new QListWidget;list->setObjectName("mosaicLayers");list->setAccessibleName(t("Layers, bottom to top","Livelli, dal basso verso l’alto"));list->setMaximumHeight(140);panel->addWidget(list);
        connect(list,&QListWidget::currentRowChanged,this,[this]{if(!updating){readControls();display();}});
        button("mosaicRecommend",t("Recommended strategy…","Strategia consigliata…"),panel,[this]{recommendStrategy();});
        button("mosaicAutoOverlap",t("Automatically align overlaps…","Allinea automaticamente le sovrapposizioni…"),panel,[this]{autoAlign();});
        button("mosaicRestoreRoi",t("Restore profile ROI","Ripristina ROI del profilo"),panel,[this]{restoreProfileRoi();});
        button("mosaicCompareStrategies",t("Compare ROI / current…","Confronta ROI / stato attuale…"),panel,[this]{compareStrategies();});
        blink=new QCheckBox(t("Blink selected layer (preview only)","Lampeggia livello selezionato (solo anteprima)"));blink->setObjectName("mosaicBlink");blink->setAccessibleName(blink->text());panel->addWidget(blink);
        connect(blink,&QCheckBox::toggled,this,[this](bool enabled){blinkHidden=false;setProperty("blinkHidden",false);if(enabled)blinkTimer.start();else blinkTimer.stop();schedule();});
        inspectionMode=new QComboBox;inspectionMode->setObjectName("mosaicInspection");inspectionMode->addItems({t("Composite preview","Anteprima composita"),t("Difference: selected vs adjacent","Differenza: selezionato e adiacente"),t("Checkerboard: selected vs adjacent","Scacchiera: selezionato e adiacente"),t("Swipe: selected vs adjacent","Tendina: selezionato e adiacente")});inspectionMode->setAccessibleName(t("Seam inspection mode","Modalità ispezione giunzione"));panel->addWidget(inspectionMode);
        swipePosition=new QSlider(Qt::Horizontal);swipePosition->setObjectName("mosaicSwipe");swipePosition->setRange(0,100);swipePosition->setValue(50);swipePosition->setAccessibleName(t("Swipe comparison position","Posizione confronto a tendina"));swipePosition->setEnabled(false);panel->addWidget(swipePosition);
        connect(inspectionMode,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int mode){swipePosition->setEnabled(mode==3);schedule();});
        connect(swipePosition,&QSlider::valueChanged,this,[this]{if(inspectionMode->currentIndex()==3)schedule();});
        button("mosaicZoomSeam",t("Zoom to selected overlap","Zoom sulla sovrapposizione selezionata"),panel,[this]{zoomToOverlap();});
        button("mosaicControlPoints",t("Align with control points…","Allinea con punti di controllo…"),panel,[this]{controlPointsDialog();});
        button("mosaicCropProposal",t("Propose uniform-border crop…","Proponi ritaglio bordi uniformi…"),panel,[this]{proposeCrop();});
        button("mosaicPolygonMask",t("Draw polygon mask…","Disegna maschera poligonale…"),panel,[this]{polygonMaskDialog();});
        button("mosaicClearPolygonMasks",t("Clear selected polygon masks","Cancella maschere poligonali selezionate"),panel,[this]{clearPolygonMasks(list->currentRow());});
        auto* ordering=new QHBoxLayout;panel->addLayout(ordering);
        button("mosaicDown",t("Lower","Abbassa"),ordering,[this]{reorder(-1);});button("mosaicUp",t("Raise","Alza"),ordering,[this]{reorder(1);});
        button("mosaicRemove",t("Remove","Rimuovi"),panel,[this]{int row=list->currentRow();if(row<0)return;remember();layers.erase(layers.begin()+row);refreshList();});
        auto* history=new QHBoxLayout;panel->addLayout(history);
        button("mosaicUndo",t("Undo","Annulla"),history,[this]{undo();});button("mosaicRedo",t("Redo","Ripeti"),history,[this]{redo();});
        visible=new QCheckBox(t("Visible layer","Livello visibile"));visible->setObjectName("mosaicVisible");panel->addWidget(visible);connect(visible,&QCheckBox::toggled,this,[this]{edit();});
        blendMode=new QComboBox;blendMode->setObjectName("mosaicBlendMode");blendMode->addItems({t("No blending (upper layer replaces)","Nessun blending (il livello superiore sostituisce)"),t("Feather (visual transition)","Sfumatura (transizione visuale)"),t("Minimum-cost seam (visual)","Giunzione a costo minimo (visuale)"),t("Multiband blend (visual)","Blending multibanda (visuale)")});blendMode->setAccessibleName(t("Selected layer blending mode","Modalità blending del livello selezionato"));panel->addWidget(blendMode);connect(blendMode,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{edit();});
        auto* form=new QFormLayout;panel->addLayout(form);
        struct Field{const char* id;const char* en;const char* italian;double min,max,step;int decimals;};
        const Field fields[]={
            {"x","X (pixels)","X (pixel)",-1000000,1000000,1,2},{"y","Y (pixels)","Y (pixel)",-1000000,1000000,1,2},
            {"scale","Scale","Scala",.01,100,.01,4},{"angle","Rotation (degrees)","Rotazione (gradi)",-360,360,.1,2},
            {"opacity","Opacity","Opacità",0,1,.05,3},{"feather","Edge feather (pixels)","Sfumatura bordi (pixel)",0,10000,1,1},
            {"cropX","Crop X","Ritaglio X",0,100000,1,0},{"cropY","Crop Y","Ritaglio Y",0,100000,1,0},
            {"cropW","Crop width (0 = full)","Larghezza ritaglio (0 = intera)",0,100000,1,0},{"cropH","Crop height (0 = full)","Altezza ritaglio (0 = intera)",0,100000,1,0},
            {"black","Black level (0–1)","Livello nero (0–1)",0,.999,.001,4},{"white","White level (0–1)","Livello bianco (0–1)",.001,1,.001,4},
            {"gamma","Gamma","Gamma",.1,10,.05,3},{"exposure","Exposure (EV)","Esposizione (EV)",-10,10,.1,3},
            {"saturation","Saturation","Saturazione",0,4,.05,3},{"red","Red gain","Guadagno rosso",0,4,.05,3},
            {"green","Green gain","Guadagno verde",0,4,.05,3},{"blue","Blue gain","Guadagno blu",0,4,.05,3},
            {"excludeX","Exclude mask X","Maschera esclusa X",0,100000,1,0},{"excludeY","Exclude mask Y","Maschera esclusa Y",0,100000,1,0},
            {"excludeW","Exclude mask width (0 = none)","Larghezza maschera esclusa (0 = nessuna)",0,100000,1,0},{"excludeH","Exclude mask height (0 = none)","Altezza maschera esclusa (0 = nessuna)",0,100000,1,0}};
        for(const auto& field:fields){auto* spin=new QDoubleSpinBox;spin->setObjectName(QString("mosaic_")+field.id);spin->setRange(field.min,field.max);spin->setSingleStep(field.step);spin->setDecimals(field.decimals);spin->setKeyboardTracking(false);spin->setAccessibleName(t(field.en,field.italian));form->addRow(t(field.en,field.italian),spin);controls.push_back(spin);connect(spin,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[this]{edit();});}
        button("mosaicResetColour",t("Reset selected colours","Ripristina colori selezionati"),panel,[this]{resetColour();});
        button("mosaicAllColour",t("Apply selected colours to all","Applica colori selezionati a tutti"),panel,[this]{applyColoursToAll();});
        button("mosaicMatchColour",t("Match selected layer to adjacent…","Uniforma il livello selezionato all’adiacente…"),panel,[this]{visualCompensationDialog();});
        button("mosaicSave",t("Save editable project…","Salva progetto modificabile…"),panel,[this]{saveProjectDialog();});
        button("mosaicOpen",t("Open project…","Apri progetto…"),panel,[this]{loadProjectDialog();});
        button("mosaicQuality",t("Quality assessment…","Valutazione qualità…"),panel,[this]{qualityDialog();});
        button("mosaicExport",t("Export single image…","Esporta immagine unica…"),panel,[this]{exportDialog();});
        auto* right=new QVBoxLayout;body->addLayout(right,1);
        auto* zoomRow=new QHBoxLayout;right->addLayout(zoomRow);zoomRow->addWidget(new QLabel(t("Preview zoom (%)","Zoom anteprima (%)")));
        zoom=new QDoubleSpinBox;zoom->setRange(25,400);zoom->setValue(100);zoom->setAccessibleName(t("Preview zoom","Zoom anteprima"));zoomRow->addWidget(zoom);connect(zoom,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[this]{display();});button("mosaicFit",t("Fit","Adatta"),zoomRow,[this]{fitPreview();});zoomRow->addStretch();
        histogramLabel_=new QLabel;histogramLabel_->setObjectName("mosaicHistogram");histogramLabel_->setFixedHeight(82);histogramLabel_->setAccessibleName(t("Composite luminance histogram","Istogramma di luminanza della composizione"));right->addWidget(histogramLabel_);
        clippingLabel_=new QLabel;clippingLabel_->setObjectName("mosaicClipping");clippingLabel_->setTextFormat(Qt::PlainText);clippingLabel_->setAccessibleName(t("Highlight and shadow clipping","Clipping alte luci e ombre"));right->addWidget(clippingLabel_);
        auto* view=new QScrollArea;previewView=view;canvas=new MosaicCanvas;canvas->setObjectName("mosaicCanvas");canvas->setAlignment(Qt::AlignTop|Qt::AlignLeft);canvas->setFocusPolicy(Qt::StrongFocus);canvas->setAccessibleName(t("Composite preview; drag or use arrow keys to move the selected layer","Anteprima composizione; trascina o usa le frecce per spostare il livello selezionato"));view->setWidget(canvas);right->addWidget(view,1);
        canvas->moved=[this](QPointF delta){int row=list->currentRow();if(busy||row<0||previewImage.isNull())return;remember();auto& l=layers[size_t(row)];l.x+=delta.x()/(previewRatio*zoom->value()/100.);l.y+=delta.y()/(previewRatio*zoom->value()/100.);readControls();schedule();};
        canvas->nudged=[this](QPointF delta){int row=list->currentRow();if(busy||row<0)return;remember();auto& l=layers[size_t(row)];l.x+=delta.x();l.y+=delta.y();setProperty("mergeStrategy","manual");readControls();schedule();};
        status=new QLabel;status->setTextFormat(Qt::PlainText);status->setObjectName("mosaicStatus");status->setWordWrap(true);outer->addWidget(status);
        button("mosaicLog",t("Open compositor log","Apri log compositore"),outer,[this]{record("Log opened");QDesktopServices::openUrl(QUrl::fromLocalFile(workingFolder+"/compositor.log"));});
        auto* close=new QPushButton(t("Close","Chiudi"));outer->addWidget(close);connect(close,&QPushButton::clicked,this,&QDialog::reject);
        previewTimer.setSingleShot(true);previewTimer.setInterval(350);connect(&previewTimer,&QTimer::timeout,this,[this]{preview();});
        blinkTimer.setInterval(700);connect(&blinkTimer,&QTimer::timeout,this,[this]{if(busy)return;blinkHidden=!blinkHidden;setProperty("blinkHidden",blinkHidden);schedule();});
        status->setText(t("Add portions or aligned channels to begin.","Aggiungi porzioni o canali allineati per iniziare."));
    }
    bool isBusy()const{return busy||previewTimer.isActive();}
    const std::vector<orbyx::MosaicLayer>& state()const{return layers;}
    // Side-by-side comparison image: profile ROI (left) vs current strategy (right).
    // Pure and bounded; both renders are explicit 8-bit previews. Throws when no ROI
    // baseline exists. Used by compareStrategies() and exercised directly by tests.
    cv::Mat strategyComparisonImage(int edge=1200)const{
        if(profileRoiBaseline.empty())throw std::runtime_error("No profile ROI baseline to compare");
        if(layers.empty())throw std::runtime_error("No current layers to compare");
        const cv::Mat roi=orbyx::renderMosaic(profileRoiBaseline,edge).image;
        const cv::Mat cur=orbyx::renderMosaic(layers,edge).image;
        const int gap=10,height=std::max(roi.rows,cur.rows);
        cv::Mat combined=cv::Mat::zeros(height,roi.cols+gap+cur.cols,CV_8UC3);
        roi.copyTo(combined(cv::Rect(0,0,roi.cols,roi.rows)));
        cur.copyTo(combined(cv::Rect(roi.cols+gap,0,cur.cols,cur.rows)));
        cv::line(combined,{roi.cols+gap/2,0},{roi.cols+gap/2,height-1},cv::Scalar(0,212,255),2,cv::LINE_AA);
        const auto label=[&](const std::string& text,int x){cv::putText(combined,text,{x,24},cv::FONT_HERSHEY_SIMPLEX,0.7,cv::Scalar(0,0,0),3,cv::LINE_AA);cv::putText(combined,text,{x,24},cv::FONT_HERSHEY_SIMPLEX,0.7,cv::Scalar(255,255,255),1,cv::LINE_AA);};
        label(it?"ROI profilo":"Profile ROI",8);label(it?"Attuale":"Current",roi.cols+gap+8);
        return combined;
    }
    void applyControlPoints(const orbyx::ControlPointSolution& solution,int moving,int reference){
        if(busy||moving<0||reference<0||moving>=int(layers.size())||reference>=int(layers.size())||moving==reference)throw std::runtime_error("Invalid control-point layer selection");
        remember();layers[size_t(moving)]=solution.layer;setProperty("mergeStrategy","control-points");setProperty("controlPointInliers",int(solution.inliers.size()));setProperty("controlPointMedian",solution.medianError);setProperty("controlPointMaximum",solution.maxError);setProperty("controlPointConcentrated",solution.spatiallyConcentrated);
        persistentNotice=t("Control-point transform applied. Review residuals and every seam before export.","Trasformazione da punti di controllo applicata. Verifica residui e ogni giunzione prima dell’esportazione.");refreshList(moving);status->setText(persistentNotice);record(QString("Control points: %1 inliers, median %2 px, max %3 px").arg(solution.inliers.size()).arg(solution.medianError,0,'f',3).arg(solution.maxError,0,'f',3));
    }
    void applyCropProposal(const orbyx::CropProposal& proposal,int row){
        if(busy||row<0||row>=int(layers.size())||proposal.original.width<=0||proposal.original.height<=0||proposal.proposed.width<=0||proposal.proposed.height<=0||(proposal.proposed&proposal.original)!=proposal.proposed)throw std::runtime_error("Invalid crop proposal");
        auto& layer=layers[size_t(row)];const cv::Rect current=layer.crop.area()>0?layer.crop:proposal.original;
        if(current!=proposal.original)throw std::runtime_error("Crop proposal no longer matches the selected layer");
        const double dx=proposal.proposed.x-proposal.original.x,dy=proposal.proposed.y-proposal.original.y;
        const double radians=layer.angle*CV_PI/180.,cosine=std::cos(radians),sine=std::sin(radians);
        remember();layer.x+=layer.scale*(cosine*dx-sine*dy);layer.y+=layer.scale*(sine*dx+cosine*dy);layer.crop=proposal.proposed;
        setProperty("mergeStrategy","manual-crop");setProperty("cropProposalApplied",true);setProperty("cropRetainedPercent",proposal.retainedFraction*100.);
        persistentNotice=t("Crop proposal applied without moving the retained content. Review the borders and undo if needed.","Proposta di ritaglio applicata senza spostare il contenuto conservato. Controlla i bordi e annulla se necessario.");
        refreshList(row);status->setText(persistentNotice);record(QString("Uniform-border crop applied: x=%1 y=%2 w=%3 h=%4 retained=%5%").arg(proposal.proposed.x).arg(proposal.proposed.y).arg(proposal.proposed.width).arg(proposal.proposed.height).arg(proposal.retainedFraction*100.,0,'f',1));
    }
    void applyPolygonMask(const orbyx::MosaicMask& mask,int row){if(busy||row<0||row>=int(layers.size())||mask.points.size()<3)throw std::runtime_error("Invalid polygon mask");remember();layers[size_t(row)].masks.push_back(mask);setProperty("mergeStrategy","manual-mask");setProperty("polygonMaskCount",int(layers[size_t(row)].masks.size()));persistentNotice=t("Polygon mask added. Review the preview; the source image is unchanged.","Maschera poligonale aggiunta. Controlla l’anteprima; l’immagine sorgente è invariata.");refreshList(row);status->setText(persistentNotice);record(QString("Polygon mask added: mode=%1 vertices=%2").arg(mask.include?"include":"exclude").arg(mask.points.size()));}
    void clearPolygonMasks(int row){if(busy||row<0||row>=int(layers.size())||layers[size_t(row)].masks.empty())return;remember();layers[size_t(row)].masks.clear();setProperty("mergeStrategy","manual-mask");setProperty("polygonMaskCount",0);persistentNotice=t("Polygon masks cleared from the selected layer.","Maschere poligonali cancellate dal livello selezionato.");refreshList(row);status->setText(persistentNotice);record("Polygon masks cleared");}
    void applyVisualCompensation(const orbyx::VisualCompensation& proposal,int moving,int reference){if(busy||moving<0||reference<0||moving>=int(layers.size())||reference>=int(layers.size())||moving==reference||proposal.samples<64)throw std::runtime_error("Invalid visual compensation");auto& layer=layers[size_t(moving)];if(proposal.layer.image!=layer.image||proposal.layer.x!=layer.x||proposal.layer.y!=layer.y||proposal.layer.scale!=layer.scale||proposal.layer.angle!=layer.angle||proposal.layer.crop!=layer.crop)throw std::runtime_error("Stale visual compensation proposal");remember();layer.exposure=proposal.layer.exposure;layer.red=proposal.layer.red;layer.green=proposal.layer.green;layer.blue=proposal.layer.blue;setProperty("mergeStrategy","visual-compensation");setProperty("compensationSamples",qulonglong(proposal.samples));setProperty("compensationExposure",proposal.exposureDelta);persistentNotice=t("Visual colour match applied. Review the overlap; source data is unchanged.","Corrispondenza colore visuale applicata. Controlla la sovrapposizione; i dati sorgente sono invariati.");refreshList(moving);status->setText(persistentNotice);record(QString("Visual compensation: samples=%1 exposure=%2 RGB=%3/%4/%5").arg(proposal.samples).arg(proposal.exposureDelta,0,'f',3).arg(proposal.redMultiplier,0,'f',3).arg(proposal.greenMultiplier,0,'f',3).arg(proposal.blueMultiplier,0,'f',3));}
    void setState(const std::vector<orbyx::MosaicLayer>& state){if(busy)return;orbyx::mosaicProject(state);remember();layers=state;refreshList();}
    void appendDemo(QString sample={}){
        if(busy||layers.size()>30)return;
        if(sample.isEmpty()){const QDir app(QCoreApplication::applicationDirPath());for(const auto& candidate:{app.filePath("../share/orbyx/demo-jpeg"),app.filePath("share/orbyx/demo-jpeg"),app.filePath("../assets/demo-jpeg"),QDir::current().filePath("assets/demo-jpeg")})if(QFileInfo(QDir(candidate).filePath("sx_MSUGS-VIS-1.jpg")).isFile()){sample=candidate;break;}}
        QDir().mkpath(workingFolder);const auto destination=workingFolder+"/demo-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
        auto added=std::make_shared<std::vector<orbyx::MosaicLayer>>();
        run([sample,destination,added]{
            if(sample.isEmpty())throw std::runtime_error("Bundled demo images not found");
            orbyx::runImageDemo(sample.toStdString(),destination.toStdString());
            QFile report(destination+"/demo-report.json");if(!report.open(QIODevice::ReadOnly))throw std::runtime_error("Demo report missing");
            const auto data=QJsonDocument::fromJson(report.readAll()).object();const int down=data["seam"].toObject()["right_down_pixels"].toInt();
            for(int i=0;i<2;++i){orbyx::MosaicLayer l;l.image=(destination+(i==0?"/sx-natural.png":"/dx-natural.png")).toStdString();l.x=i*714;l.crop={0,45-(i==0?0:down),714,1428};l.black=8./255;l.white=188./255;l.gamma=1.25;added->push_back(l);}
        },[this,added]{remember();layers.insert(layers.end(),added->begin(),added->end());refreshList();});
    }
    void appendFiles(const QStringList& files){
        if(busy||files.isEmpty())return;if(layers.size()+size_t(files.size())>32){status->setText(t("Maximum 32 layers.","Massimo 32 livelli."));return;}
        auto added=std::make_shared<std::vector<orbyx::MosaicLayer>>();
        run([files,added]{for(const auto& f:files){orbyx::readCompositePng(f.toStdString());orbyx::MosaicLayer l;l.image=std::filesystem::absolute(f.toStdString());added->push_back(l);}},[this,added]{remember();layers.insert(layers.end(),added->begin(),added->end());refreshList();});
    }
    void appendChannels(const QString& folder,bool natural){
        if(busy||layers.size()>=32)return;QDir().mkpath(workingFolder);
        const auto file=workingFolder+"/"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".png";
        run([folder,file,natural]{std::array<std::filesystem::path,3> channels;for(int i=0;i<3;++i)channels[size_t(i)]=std::filesystem::path(folder.toStdString())/("MSUGS-VIS-"+std::to_string(i+1)+".png");if(natural)orbyx::composeNaturalColor(channels,file.toStdString());else orbyx::composeRgb(channels,file.toStdString());},[this,file]{remember();orbyx::MosaicLayer l;l.image=file.toStdString();layers.push_back(l);refreshList();});
    }
    // M2-11 single-channel preset: the chosen channel as a neutral gray layer.
    void appendSingleChannel(const QString& folder,int index){
        if(busy||layers.size()>=32||index<0||index>2)return;QDir().mkpath(workingFolder);
        const auto file=workingFolder+"/"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".png";
        run([folder,file,index]{std::array<std::filesystem::path,3> channels;for(int i=0;i<3;++i)channels[size_t(i)]=std::filesystem::path(folder.toStdString())/("MSUGS-VIS-"+std::to_string(i+1)+".png");orbyx::composeSingleChannel(channels,file.toStdString(),index);},[this,file]{remember();orbyx::MosaicLayer l;l.image=file.toStdString();layers.push_back(l);refreshList();});
    }
    void appendGeneratedProducts(const QString& outputRoot,const QString& profile,const QString& satellite,bool natural=true){
        if(busy||layers.size()>30)return;
        const QDir root(outputRoot);QStringList names;
        for(const auto& name:root.entryList({"MSUGS_VIS-*_Aligned"},QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name)){
            const QFileInfo folder(root.filePath(name));bool valid=!folder.isSymLink();
            for(int channel=1;channel<=3;++channel){const QFileInfo image(root.filePath(name+"/MSUGS-VIS-"+QString::number(channel)+".png"));valid=valid&&image.isFile()&&!image.isSymLink()&&image.size()>0;}
            if(valid)names.append(name);
            if(names.size()>=32)break;
        }
        if(names.isEmpty()){
            persistentNotice=t("No editable aligned VIS product was found in the selected output.","Nell’output selezionato non è stato trovato alcun prodotto VIS allineato modificabile.");status->setText(persistentNotice);return;
        }
        const auto destination=workingFolder+"/generated-"+QUuid::createUuid().toString(QUuid::WithoutBraces);QDir().mkpath(destination);
        struct Import {std::vector<orbyx::MosaicLayer> layers;bool profileRoi=false;};
        auto imported=std::make_shared<Import>();
        run([outputRoot,profile,satellite,natural,destination,names,imported]{
            const QDir root(outputRoot);const int first=names.indexOf("MSUGS_VIS-1_Aligned"),second=names.indexOf("MSUGS_VIS-2_Aligned");
            orbyx::EditableMergeLayout layout;
            if(first>=0&&second>=0){std::array<std::filesystem::path,2> representative{root.filePath(names[first]+"/MSUGS-VIS-1.png").toStdString(),root.filePath(names[second]+"/MSUGS-VIS-1.png").toStdString()};layout=orbyx::editableMergeLayout(profile.toStdString(),satellite.toStdString(),representative);imported->profileRoi=layout.profileRoi;}
            for(int vis=0;vis<names.size();++vis){
                std::array<std::filesystem::path,3> channels;
                for(int channel=0;channel<3;++channel)channels[size_t(channel)]=root.filePath(names[vis]+"/MSUGS-VIS-"+QString::number(channel+1)+".png").toStdString();
                const auto file=std::filesystem::path(destination.toStdString())/("vis-"+std::to_string(vis+1)+".png");
                if(natural)orbyx::composeNaturalColor(channels,file);else orbyx::composeRgb(channels,file);
                // The composed editor layer is no longer an RDAS source channel:
                // inspect it with the composite reader so small/derived products do
                // not inherit the native RDAS height constraint.
                const auto composed=orbyx::readCompositePng(file);orbyx::MosaicLayer layer;layer.image=file;layer.crop={0,0,composed.cols,composed.rows};
                if(imported->profileRoi&&(vis==first||vis==second)){const int pair=vis==first?0:1;const auto region=layout.regions[size_t(pair)];layer.crop={region.x,region.y,region.width,region.height};layer.x=pair?layout.regions[0].width:0;}
                imported->layers.push_back(layer);
            }
        },[this,imported]{
            remember();layers.insert(layers.end(),imported->layers.begin(),imported->layers.end());
            persistentNotice=imported->profileRoi?
                t("Profile ROI loaded for VIS1/VIS2; every selected product remains editable.","ROI del profilo caricata per VIS1/VIS2; tutti i prodotti selezionati restano modificabili."):
                t("Selected aligned VIS products loaded for visual or manual placement.","Prodotti VIS allineati selezionati caricati per il posizionamento visuale o manuale.");
            if(imported->profileRoi){profileRoiBaseline=imported->layers;setProperty("mergeStrategy","profile-roi");}
            else {profileRoiBaseline.clear();setProperty("mergeStrategy","manual");}
            setProperty("loadedProfileRoi",imported->profileRoi);refreshList();status->setText(persistentNotice);
        });
    }
    void autoAlign(bool confirm=true){
        if(busy||layers.size()<2){status->setText(t("Add at least two portions.","Aggiungi almeno due porzioni."));return;}
        const int selected=list->currentRow();const std::size_t anchor=selected>=0?size_t(selected):0;
        auto state=layers;auto proposal=std::make_shared<orbyx::AutoOverlapResult>();
        run([state,anchor,proposal]{*proposal=orbyx::autoAlignOverlaps(state,anchor);},[this,proposal,anchor,confirm]{
            if(proposal->accepted.empty()){
                persistentNotice=t("No reliable overlap found. Portions were not changed; use crop or manual controls.","Nessuna sovrapposizione affidabile trovata. Le porzioni non sono state modificate; usa ritaglio o controlli manuali.");setProperty("autoOverlapApplied",false);setProperty("autoOverlapAccepted",0);status->setText(persistentNotice);
                record("Automatic overlap: no reliable matches; no changes applied");return;
            }
            QStringList evidence;
            for(const auto& match:proposal->accepted)evidence<<t("Layer ","Livello ")+QString::number(match.moving+1)+" → "+QString::number(match.reference+1)+
                QString(" | %1/%2 inlier | overlap %3% | error %4 px | confidence %5%")
                    .arg(match.inliers).arg(match.candidateMatches).arg(match.overlapFraction*100,0,'f',1).arg(match.medianError,0,'f',2).arg(match.confidence*100,0,'f',0);
            QString summary=t("Automatic proposal:\n","Proposta automatica:\n")+evidence.join("\n");
            if(!proposal->unaligned.empty())summary+=t("\n\nUnchanged layers without a reliable match: ","\n\nLivelli non modificati senza corrispondenza affidabile: ");
            for(size_t i=0;i<proposal->unaligned.size();++i){if(i)summary+=", ";summary+=QString::number(proposal->unaligned[i]+1);}
            summary+=t("\n\nApply this editable proposal?","\n\nApplicare questa proposta modificabile?");
            if(confirm&&QMessageBox::question(this,t("Automatic overlap","Sovrapposizione automatica"),summary,QMessageBox::Apply|QMessageBox::Cancel,QMessageBox::Cancel)!=QMessageBox::Apply){
                persistentNotice=t("Automatic proposal cancelled; no changes applied.","Proposta automatica annullata; nessuna modifica applicata.");setProperty("autoOverlapApplied",false);setProperty("autoOverlapAccepted",0);status->setText(persistentNotice);record("Automatic overlap proposal cancelled");return;
            }
            remember();layers=proposal->layers;setProperty("mergeStrategy","visual-auto");record(summary);refreshList(int(anchor));
            setProperty("autoOverlapApplied",true);setProperty("autoOverlapAccepted",int(proposal->accepted.size()));setProperty("unalignedCount",int(proposal->unaligned.size()));
            if(proposal->unaligned.empty())
                persistentNotice=t("Automatic proposal applied. Review every seam and adjust it manually before export.","Proposta automatica applicata. Controlla ogni giunzione e correggila manualmente prima dell’esportazione.");
            else {
                QStringList leftovers;for(const auto index:proposal->unaligned)leftovers<<QString::number(index+1);
                // >2 portions: the coordinator never invents a position for a portion
                // without a reliable match; it routes it to assisted/manual control.
                persistentNotice=t("Automatic proposal applied. Layers without a reliable match were not moved: ","Proposta automatica applicata. I livelli senza corrispondenza affidabile non sono stati spostati: ")+leftovers.join(", ")+
                    t(". Use control points (assisted) or manual placement for them.",". Usa i punti di controllo (assistita) o il posizionamento manuale per questi.");
            }
            status->setText(persistentNotice);
        });
    }
    // M2-07 coordinator: recommend one of the four strategies for the current
    // portions (any count ≥ 2). A complete profile-ROI pair is recommended as the
    // historical starting point; otherwise the visual automatic cascade runs and,
    // for more than two portions, every portion left without a reliable match is
    // routed to assisted (control points) or manual placement — never invented.
    void recommendStrategy(){
        if(busy)return;
        if(layers.size()<2){status->setText(t("Add at least two portions.","Aggiungi almeno due porzioni."));return;}
        if(!profileRoiBaseline.empty()){
            setProperty("recommendedStrategy","profile-roi");
            persistentNotice=t("Recommended: profile ROI (historical geometry). Compare or restore it, or edit manually.","Consigliata: ROI del profilo (geometria storica). Confrontala o ripristinala, oppure modifica manualmente.");
            status->setText(persistentNotice);record("Recommended strategy: profile ROI");return;
        }
        setProperty("recommendedStrategy","visual-auto");
        record(QString("Recommended strategy: visual automatic for %1 portions (no complete profile ROI pair)").arg(layers.size()));
        autoAlign(true); // Confirmed visual cascade; handles more than two portions.
    }
    void proposeCrop(bool confirm=true){
        const int row=list->currentRow();if(busy||row<0||row>=int(layers.size())){status->setText(t("Select one layer first.","Seleziona prima un livello."));return;}
        const auto selected=layers[size_t(row)];auto proposal=std::make_shared<orbyx::CropProposal>();
        run([selected,proposal]{*proposal=orbyx::proposeUniformBorderCrop(selected);},[this,proposal,row,confirm]{
            const auto& before=proposal->original;const auto& after=proposal->proposed;
            QString summary=t("Detected low-detail outer borders. This is only a proposal; source pixels are not deleted.\n\nCurrent: x=%1, y=%2, width=%3, height=%4\nProposed: x=%5, y=%6, width=%7, height=%8\nRetained area: %9%\n\nApply this reversible crop?","Rilevati bordi esterni a basso dettaglio. Questa è solo una proposta; i pixel sorgente non vengono eliminati.\n\nAttuale: x=%1, y=%2, larghezza=%3, altezza=%4\nProposto: x=%5, y=%6, larghezza=%7, altezza=%8\nArea conservata: %9%\n\nApplicare questo ritaglio reversibile?")
                .arg(before.x).arg(before.y).arg(before.width).arg(before.height).arg(after.x).arg(after.y).arg(after.width).arg(after.height).arg(proposal->retainedFraction*100.,0,'f',1);
            if(confirm&&QMessageBox::question(this,t("Uniform-border crop proposal","Proposta ritaglio bordi uniformi"),summary,QMessageBox::Apply|QMessageBox::Cancel,QMessageBox::Cancel)!=QMessageBox::Apply){
                persistentNotice=t("Crop proposal cancelled; the layer was not changed.","Proposta di ritaglio annullata; il livello non è stato modificato.");setProperty("cropProposalApplied",false);status->setText(persistentNotice);record("Uniform-border crop proposal cancelled");return;
            }
            applyCropProposal(*proposal,row);
        });
    }
    void reorder(int direction){int row=list->currentRow(),next=row+direction;if(busy||row<0||next<0||next>=int(layers.size()))return;remember();std::swap(layers[size_t(row)],layers[size_t(next)]);refreshList(next);}
    void undo(){if(busy||undoStates.empty())return;dirty=true;redoStates.push_back(layers);layers=undoStates.back();undoStates.pop_back();refreshList();}
    void redo(){if(busy||redoStates.empty())return;dirty=true;undoStates.push_back(layers);layers=redoStates.back();redoStates.pop_back();refreshList();}
    void resetColour(){int row=list->currentRow();if(busy||row<0)return;remember();auto& l=layers[size_t(row)];l.black=0;l.white=1;l.gamma=1;l.exposure=0;l.saturation=1;l.red=l.green=l.blue=1;readControls();schedule();}
    void applyColoursToAll(){int row=list->currentRow();if(busy||row<0)return;remember();const auto source=layers[size_t(row)];for(auto& l:layers){l.black=source.black;l.white=source.white;l.gamma=source.gamma;l.exposure=source.exposure;l.saturation=source.saturation;l.red=source.red;l.green=source.green;l.blue=source.blue;}schedule();}
    void saveProject(const QString& name){
        if(!name.endsWith(".json")||QFileInfo(name).isSymLink())throw std::runtime_error("Project must be a regular JSON file");
        for(const auto& layer:layers)if(QFileInfo(name).absoluteFilePath()==QFileInfo(QString::fromStdString(layer.image.string())).absoluteFilePath())throw std::runtime_error("Project cannot replace a source image");
        auto text=QByteArray::fromStdString(orbyx::mosaicProject(layers));QSaveFile file(name);file.setDirectWriteFallback(false);
        if(!file.open(QIODevice::WriteOnly))throw std::runtime_error("Cannot create project file");file.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner);
        if(file.write(text)!=text.size()||!file.commit())throw std::runtime_error("Cannot save project atomically");
        dirty=false;
    }
    void loadProject(const QString& name){
        if(busy)return;QFileInfo info(name);if(!info.isFile()||info.isSymLink()||info.size()>1024*1024)throw std::runtime_error("Invalid project file");
        QFile file(name);if(!file.open(QIODevice::ReadOnly))throw std::runtime_error("Cannot read project");auto text=file.read(1024*1024+1);
        auto loaded=orbyx::parseMosaicProject(text.toStdString());remember();layers=std::move(loaded);dirty=false;refreshList();
    }
    void exportTo(const QString& file){
        if(busy||layers.empty())return;auto state=layers;
        run([state,file]{
            auto result=orbyx::renderMosaic(state);
            orbyx::writeCompositePng(result.image,file.toStdString());
            // M2-11: declare the colour profile and the reproducible non-destructive
            // steps beside the PNG, and record the exact bit depth (no silent 16->8).
            const int bitDepth=result.image.depth()==CV_16U?16:8;
            const std::string report=orbyx::mosaicReport(state,std::filesystem::path(file.toStdString()).filename().string(),bitDepth);
            QFile reportFile(file+".report.json");
            if(!reportFile.open(QIODevice::WriteOnly|QIODevice::NewOnly)||reportFile.write(QByteArray::fromStdString(report))<0)
                throw std::runtime_error("Failed to write the export report");
        },[this,file]{status->setText(t("Single image and report exported: ","Immagine unica e report esportati: ")+file);});
    }
};
