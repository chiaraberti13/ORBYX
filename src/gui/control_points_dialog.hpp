// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "orbyx/compositor.hpp"
#include <QAbstractItemView>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <optional>

class ControlPointsDialog final:public QDialog {
    bool it_{};orbyx::MosaicLayer moving_,reference_;QTableWidget* table_{};QLabel* summary_{};
    QPushButton *solveButton_{},*applyButton_{};std::optional<orbyx::ControlPointSolution> solution_;
    QString t(const char* en,const char* italian)const{return QString::fromUtf8(it_?italian:en);}
    QDoubleSpinBox* coordinate(double value=0){auto* spin=new QDoubleSpinBox;spin->setRange(-1000000,1000000);spin->setDecimals(3);spin->setValue(value);spin->setKeyboardTracking(false);connect(spin,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[this]{invalidate();});return spin;}
    void invalidate(){solution_.reset();applyButton_->setEnabled(false);setProperty("spatialWarning",false);summary_->setText(t("Edit at least three corresponding pairs, then calculate the transform.","Modifica almeno tre coppie corrispondenti, poi calcola la trasformazione."));for(int row=0;row<table_->rowCount();++row){table_->setItem(row,4,new QTableWidgetItem);}}
public:
    ControlPointsDialog(bool italian,const orbyx::MosaicLayer& moving,const orbyx::MosaicLayer& reference,QWidget* parent=nullptr):QDialog(parent),it_(italian),moving_(moving),reference_(reference){
        setObjectName("controlPointsDialog");setWindowTitle(t("Control-point alignment","Allineamento con punti di controllo"));resize(760,500);
        auto* layout=new QVBoxLayout(this);auto* note=new QLabel(t("Enter the same visible feature in moving and reference crop coordinates. Use well-spread points; red rows are rejected outliers.","Inserisci lo stesso dettaglio visibile nelle coordinate del ritaglio mobile e di riferimento. Usa punti ben distribuiti; le righe rosse sono outlier rifiutati."));note->setWordWrap(true);layout->addWidget(note);
        table_=new QTableWidget(0,5);table_->setObjectName("controlPointTable");table_->setHorizontalHeaderLabels({t("Moving X","Mobile X"),t("Moving Y","Mobile Y"),t("Reference X","Riferimento X"),t("Reference Y","Riferimento Y"),t("Residual (px)","Residuo (px)")});table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);table_->setSelectionBehavior(QAbstractItemView::SelectRows);layout->addWidget(table_,1);
        auto* rowButtons=new QHBoxLayout;auto* add=new QPushButton(t("Add pair","Aggiungi coppia"));add->setObjectName("controlPointAdd");auto* remove=new QPushButton(t("Remove selected","Rimuovi selezionata"));remove->setObjectName("controlPointRemove");solveButton_=new QPushButton(t("Calculate transform","Calcola trasformazione"));solveButton_->setObjectName("controlPointSolve");rowButtons->addWidget(add);rowButtons->addWidget(remove);rowButtons->addStretch();rowButtons->addWidget(solveButton_);layout->addLayout(rowButtons);
        summary_=new QLabel;summary_->setObjectName("controlPointSummary");summary_->setWordWrap(true);layout->addWidget(summary_);
        auto* buttons=new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Apply);applyButton_=buttons->button(QDialogButtonBox::Apply);applyButton_->setObjectName("controlPointApply");applyButton_->setEnabled(false);layout->addWidget(buttons);
        connect(add,&QPushButton::clicked,this,[this]{addPair({});});connect(remove,&QPushButton::clicked,this,[this]{int row=table_->currentRow();if(row>=0){table_->removeRow(row);invalidate();}});connect(solveButton_,&QPushButton::clicked,this,[this]{solve();});connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);connect(applyButton_,&QPushButton::clicked,this,[this]{if(solution_)accept();});
        for(int i=0;i<4;++i)addPair({});invalidate();
    }
    void addPair(const orbyx::ControlPointPair& pair){const int row=table_->rowCount();table_->insertRow(row);table_->setCellWidget(row,0,coordinate(pair.moving.x));table_->setCellWidget(row,1,coordinate(pair.moving.y));table_->setCellWidget(row,2,coordinate(pair.reference.x));table_->setCellWidget(row,3,coordinate(pair.reference.y));table_->setItem(row,4,new QTableWidgetItem);invalidate();}
    void setPairs(const std::vector<orbyx::ControlPointPair>& pairs){table_->setRowCount(0);for(const auto& pair:pairs)addPair(pair);invalidate();}
    std::vector<orbyx::ControlPointPair> pairs()const{std::vector<orbyx::ControlPointPair> result;for(int row=0;row<table_->rowCount();++row){auto value=[&](int column){return qobject_cast<QDoubleSpinBox*>(table_->cellWidget(row,column))->value();};result.push_back({{value(0),value(1)},{value(2),value(3)}});}return result;}
    bool solve(){
        try{solution_=orbyx::alignFromControlPoints(moving_,reference_,pairs());}
        catch(const std::exception& error){solution_.reset();applyButton_->setEnabled(false);summary_->setText(t("Cannot calculate: ","Impossibile calcolare: ")+QString::fromUtf8(error.what()));return false;}
        const auto& solved=*solution_;for(int row=0;row<table_->rowCount();++row){const bool inlier=std::find(solved.inliers.begin(),solved.inliers.end(),size_t(row))!=solved.inliers.end();auto* item=new QTableWidgetItem(QString::number(solved.residuals[size_t(row)],'f',2));if(!inlier)item->setBackground(QColor("#8b2635"));table_->setItem(row,4,item);}
        QString summary=t("Accepted points: ","Punti accettati: ")+QString::number(solved.inliers.size())+"/"+QString::number(solved.residuals.size())+t(" · median error: "," · errore mediano: ")+QString::number(solved.medianError,'f',2)+t(" px · maximum inlier error: "," px · errore massimo inlier: ")+QString::number(solved.maxError,'f',2)+t(" px · spatial coverage moving/reference: "," px · copertura spaziale mobile/riferimento: ")+QString::number(solved.movingCoverage*100.,'f',1)+"% / "+QString::number(solved.referenceCoverage*100.,'f',1)+"%";
        if(solved.spatiallyConcentrated)summary+=t("\nWarning: accepted points are concentrated in a small area. Add well-spread points before trusting rotation or scale.","\nAttenzione: i punti accettati sono concentrati in una zona ridotta. Aggiungi punti ben distribuiti prima di considerare affidabili rotazione o scala.");
        summary_->setText(summary);setProperty("spatialWarning",solved.spatiallyConcentrated);applyButton_->setEnabled(true);return true;
    }
    const orbyx::ControlPointSolution& solution()const{if(!solution_)throw std::runtime_error("Control points not solved");return *solution_;}
};
