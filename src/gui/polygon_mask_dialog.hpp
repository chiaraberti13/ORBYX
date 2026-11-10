// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "orbyx/compositor.hpp"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <stdexcept>

class PolygonMaskCanvas final:public QLabel {
    QImage background_;double ratio_=1;bool include_=false;std::vector<cv::Point2d> points_;
public:
    explicit PolygonMaskCanvas(QWidget* parent=nullptr):QLabel(parent){setObjectName("polygonMaskCanvas");setAlignment(Qt::AlignLeft|Qt::AlignTop);setCursor(Qt::CrossCursor);}
    void setImage(const QImage& image,double ratio){background_=image;ratio_=ratio;setFixedSize(image.size());update();}
    void setInclude(bool include){include_=include;update();}
    void undoPoint(){if(!points_.empty())points_.pop_back();update();}
    void clearPoints(){points_.clear();update();}
    const std::vector<cv::Point2d>& points()const{return points_;}
protected:
    void mousePressEvent(QMouseEvent* event)override{
        if(event->button()!=Qt::LeftButton||background_.isNull())return;
        const auto position=event->position();if(position.x()<0||position.y()<0||position.x()>=width()||position.y()>=height())return;
        points_.push_back({std::clamp(position.x()/ratio_,0.,double(background_.width())/ratio_),std::clamp(position.y()/ratio_,0.,double(background_.height())/ratio_)});update();
    }
    void paintEvent(QPaintEvent* event)override{
        QLabel::paintEvent(event);QPainter painter(this);painter.drawImage(0,0,background_);if(points_.empty())return;
        const QColor colour=include_?QColor("#34d058"):QColor("#ff4d5a");painter.setRenderHint(QPainter::Antialiasing);painter.setPen(QPen(colour,2));painter.setBrush(QColor(colour.red(),colour.green(),colour.blue(),45));QPolygonF polygon;
        for(const auto& point:points_)polygon<<QPointF(point.x*ratio_,point.y*ratio_);if(polygon.size()>=3)painter.drawPolygon(polygon);else if(polygon.size()==2)painter.drawLine(polygon[0],polygon[1]);for(const auto& point:polygon)painter.drawEllipse(point,4,4);
    }
};

class PolygonMaskDialog final:public QDialog {
    bool it_{};QComboBox* mode_{};PolygonMaskCanvas* canvas_{};QLabel* count_{};QPushButton* apply_{};
    QString t(const char* en,const char* italian)const{return QString::fromUtf8(it_?italian:en);}
    void refresh(){count_->setText(t("Vertices: ","Vertici: ")+QString::number(canvas_->points().size())+t(" (minimum 3)"," (minimo 3)"));apply_->setEnabled(canvas_->points().size()>=3);}
public:
    PolygonMaskDialog(bool italian,const orbyx::MosaicLayer& layer,QWidget* parent=nullptr):QDialog(parent),it_(italian){
        setObjectName("polygonMaskDialog");setWindowTitle(t("Draw polygon mask","Disegna maschera poligonale"));resize(900,700);auto* layout=new QVBoxLayout(this);
        auto* note=new QLabel(t("Click at least three vertices around the area. Include keeps the union of included areas; Exclude hides this area and reveals lower layers. Source pixels are never changed.","Fai clic su almeno tre vertici intorno all’area. Includi conserva l’unione delle aree incluse; Escludi nasconde quest’area e mostra i livelli inferiori. I pixel sorgente non vengono mai modificati."));note->setWordWrap(true);layout->addWidget(note);
        mode_=new QComboBox;mode_->setObjectName("polygonMaskMode");mode_->addItems({t("Exclude area","Escludi area"),t("Include area","Includi area")});mode_->setAccessibleName(t("Polygon mask mode","Modalità maschera poligonale"));layout->addWidget(mode_);
        auto image=orbyx::readCompositePng(layer.image);const cv::Rect crop=layer.crop.area()>0?layer.crop:cv::Rect(0,0,image.cols,image.rows);if((crop&cv::Rect(0,0,image.cols,image.rows))!=crop)throw std::runtime_error("Polygon mask crop outside source image");cv::Mat selected=image(crop),preview;
        double low=0,high=0;cv::minMaxLoc(selected.reshape(1),&low,&high);if(high>low)selected.convertTo(preview,CV_8U,255./(high-low),-low*255./(high-low));else selected.convertTo(preview,CV_8U,image.depth()==CV_16U?1./257.:1.);
        const double ratio=std::min({1.,820./preview.cols,540./preview.rows});if(ratio<1)cv::resize(preview,preview,{},ratio,ratio,cv::INTER_AREA);if(preview.channels()==1)cv::cvtColor(preview,preview,cv::COLOR_GRAY2BGR);
        QImage shown(preview.data,preview.cols,preview.rows,int(preview.step),QImage::Format_BGR888);canvas_=new PolygonMaskCanvas;canvas_->setImage(shown.copy(),ratio);canvas_->setAccessibleName(t("Layer preview; click to add polygon vertices","Anteprima livello; fai clic per aggiungere i vertici del poligono"));auto* scroll=new QScrollArea;scroll->setWidget(canvas_);scroll->setWidgetResizable(false);layout->addWidget(scroll,1);
        count_=new QLabel;count_->setObjectName("polygonMaskCount");layout->addWidget(count_);auto* actions=new QHBoxLayout;auto* undo=new QPushButton(t("Undo last vertex","Annulla ultimo vertice"));undo->setObjectName("polygonMaskUndo");auto* clear=new QPushButton(t("Clear vertices","Cancella vertici"));clear->setObjectName("polygonMaskClear");actions->addWidget(undo);actions->addWidget(clear);actions->addStretch();layout->addLayout(actions);
        auto* buttons=new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Apply);apply_=buttons->button(QDialogButtonBox::Apply);apply_->setObjectName("polygonMaskApply");layout->addWidget(buttons);connect(mode_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int index){canvas_->setInclude(index==1);});connect(undo,&QPushButton::clicked,this,[this]{canvas_->undoPoint();refresh();});connect(clear,&QPushButton::clicked,this,[this]{canvas_->clearPoints();refresh();});connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);connect(apply_,&QPushButton::clicked,this,[this]{if(canvas_->points().size()>=3)accept();});
        canvas_->installEventFilter(this);refresh();
    }
    bool eventFilter(QObject* watched,QEvent* event)override{const bool result=QDialog::eventFilter(watched,event);if(watched==canvas_&&event->type()==QEvent::MouseButtonRelease)refresh();return result;}
    orbyx::MosaicMask mask()const{if(canvas_->points().size()<3)throw std::runtime_error("Polygon mask requires at least three vertices");return {mode_->currentIndex()==1,canvas_->points()};}
};
