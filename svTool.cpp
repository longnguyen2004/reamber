#include "svTool.h"
#include "ui_svTool.h"
#include <QObject>
#include <QString>
#include <QVector>
#include <QSplitter>
#include <QStringList>
#include <QRegExp>
#include <QList>

svTool::svTool(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::svTool)
{
    ui->setupUi(this);
}


svTool::~svTool()
{
    delete ui;
}

//Validation Button
void svTool::on_home_validateButton_clicked()
{
    //Copy over to Debug
    ui->debug_rawInputBox->setPlainText(ui->home_rawInputBox->toPlainText());

    QStringList partVector, rawInputVector;
    QString partString, rawInputString;
    QString noOfKeys;

    //validateType indicates the input type
    enum class inputType
    {
        nullInput, //0
        editorHitObjectInput, //1
        hitObjectInput, //2
        timingPointInput  //3
    };
    inputType validateType;
    validateType = inputType::nullInput;

    //Set keys
    noOfKeys = ui->home_keysSpinBox->text();
    ui->debug_procInputBox->append(ui->home_keysSpinBox->text());

    //Splits the rawInputBox input into a Vector
    rawInputVector = ui->home_rawInputBox->toPlainText().split("\n");

    //Clears the procInputBox
    ui->debug_procInputBox->clear();

    /*
     * rawInputLine.indexOf("(",1) == -1 <TRUE: Editor Hit Object Input>
     * rawInputLine.indexOf(")",1) == -1 <TRUE: Editor Hit Object Input>
     * rawInputLine.count(QRegExp(",")) > 5) <TRUE: Timing Point Input>
     * rawInputLine.count(QRegExp(",")) <= 5) <TRUE: Hit Object Input>
     */

    //Detect Input
    if ((rawInputVector.at(0)).indexOf("(", 1) != -1 &&
        (rawInputVector.at(0)).indexOf(")",1) != -1)
    {
        validateType = inputType::editorHitObjectInput;
        ui->home_statusLabel->setText("STATUS: Editor Hit Object Format detected.");
        ui->home_statusLabel->setStyleSheet("QLabel { color:green; }");

        //.count = 4: Normal Note
        //.count = 5: Long Note
    } else if (((rawInputVector.at(0)).count(QRegExp(",")) == 4 ||
                (rawInputVector.at(0)).count(QRegExp(",")) == 5) &&
                (rawInputVector.at(0)).indexOf("|",1) == -1)
    {
        validateType = inputType::hitObjectInput;
        ui->home_statusLabel->setText("STATUS: Hit Object Format detected.");
        ui->home_statusLabel->setStyleSheet("QLabel { color:green; }");

    } else if ((rawInputVector.at(0)).count(QRegExp(",")) == 7 &&
               (rawInputVector.at(0)).indexOf("|",1) == -1)
    {
        validateType = inputType::timingPointInput;
        ui->home_statusLabel->setText("STATUS: Timing Point Format detected.");
        ui->home_statusLabel->setStyleSheet("QLabel { color:green; }");

    } else
    {
        validateType = inputType::nullInput;
        ui->home_statusLabel->setText("STATUS: No Format detected.");
        ui->home_statusLabel->setStyleSheet("QLabel { color:red; }");
    }

    //Switch to generate all data
    /*
     * Format to use:
     *
     * hitObject/editorHitObject:
         * HITOBJECT|NO_OF_KEYS|OFFSET|KEY|LN_END_OFFSET
         * NO_OF_KEYS: 1 - 18

     * timingPoint:
         * TIMINGPOINT|OFFSET|VALUE|TYPE
         * VALUE: The processed value of BPM or SV
         * TYPE: SV/BPM
     *
     */
    switch (validateType){

    case inputType::nullInput:
        break;

    case inputType::editorHitObjectInput:
        //Reads each line from the rawInputVector
        foreach (rawInputString, rawInputVector) {

            //Skips any blank lines
            if(rawInputString.isEmpty()){
                continue;
            }

            //Mid trims the current line from '(' and ')'
            //Split then splits them by ',' into different notes
            partVector = rawInputString.mid(rawInputString.indexOf("(",1) + 1,
                                          rawInputString.indexOf(")",1) - rawInputString.indexOf("(",1) - 1
                                          ).split(",", QString::SkipEmptyParts);

            //For each note append according to format
            foreach (partString, partVector){
                ui->debug_procInputBox->append(
                            QString("HITOBJECT|")
                            .append(QString(noOfKeys))
                            .append(QString("|"))
                            .append(partString)
                            .append(QString("|"))
                            // -1 represents Normal Note
                            .append(QString("-1")));
            }
        }
        break;
    case inputType::hitObjectInput:
        //Reads each line from the rawInputVector
        foreach (rawInputString, rawInputVector) {

            //Skips any blank lines
            if(rawInputString.isEmpty()){
                continue;
            }

            //Splits them into parameters
            partVector = rawInputString.split(",",QString::SkipEmptyParts);

            double keyColumn;

            //Gets the key column through calculation and rounds to 0 D.P.
            keyColumn = round((partVector.at(0).toDouble() / 512 * noOfKeys.toDouble() * 2 + 1) / 2) - 1;

            if(rawInputString.count(QRegExp(",")) == 4){

                //Detected as Normal Note
                //For each note append according to format
                ui->debug_procInputBox->append(
                            QString("HITOBJECT|")
                            .append(QString(noOfKeys))
                            .append(QString("|"))
                            .append(partVector.at(2))
                            .append(QString("|"))
                            .append(QString::number(keyColumn))
                            .append(QString("|"))
                            // -1 represents Normal Note
                            .append(QString("-1")));

            } else if (rawInputString.count(QRegExp(",")) == 5){

                //Detected as Long Note
                //For each note append according to format
                ui->debug_procInputBox->append(
                            QString("HITOBJECT|")
                            .append(QString(noOfKeys))
                            .append(QString("|"))
                            .append(partVector.at(2))
                            .append(QString("|"))
                            .append(QString::number(keyColumn))
                            .append(QString("|"))
                            .append(partVector.at(5)
                                    .mid(0,partVector.at(5).indexOf(":",1))));
            }
        }
        break;

    case inputType::timingPointInput:
        //Reads each line from the rawInputVector
        foreach (rawInputString, rawInputVector)
        {

            //Skips any blank lines
            if(rawInputString.isEmpty())
                continue;

            //Splits them into parameters
            partVector = rawInputString.split(",",QString::SkipEmptyParts);

            QString timingPointValue;

            //Check Type and calculate timingPointValue
            if (partVector.at(6) == QString("0"))
            {
                //Detected as an SV Timing Point
                timingPointValue = QString::number(-100.0 / partVector.at(1).toDouble());
            } else if (partVector.at(6) == QString("1"))
            {
                //Detected as a BPM Timing Point
                timingPointValue = QString::number(60000.0 / partVector.at(1).toDouble());
            }

            //Detected as Normal Note
            //For each note append according to format
            ui->debug_procInputBox->append(
                        QString("TIMINGPOINT|")
                        .append(partVector.at(0))
                        .append(QString("|"))
                        .append(timingPointValue)
                        .append(QString("|"))
                        .append(partVector.at(6) == QString("0") ? QString("SV") : QString("BPM")));
        }
        break;
    }

    //Copy over to other boxes
    ui->stutter_procInputBox->setPlainText(ui->debug_procInputBox->toPlainText());
}

//Connect Widgets
void svTool::on_stutter_initSVSlider_valueChanged(int value)
{
    ui->stutter_initSVSpinBox->setValue((double) value / 100);
}
void svTool::on_stutter_thresholdSlider_valueChanged(int value)
{
    ui->stutter_thresholdSpinBox->setValue((double) value * 100);
}

void svTool::on_stutter_initSVSpinBox_valueChanged(double arg1)
{
    ui->stutter_initSVSlider->setValue((int) arg1);



}

void svTool::on_stutter_thresholdSpinBox_valueChanged(double arg1)
{
    ui->stutter_thresholdSlider->setValue((int) arg1);

    //averageSV = initialSV * threshold + secondSV * (1 - threshold);
    //initSV is an abstract value, we can just set averageSV and threshold which are concrete values then use initSV calculate

    double maxInitSV, minInitSV;
    double currentAverageSV, currentThreshold;

    currentAverageSV = ui->stutter_averageSVSpinBox->value();
    currentThreshold = ui->stutter_thresholdSpinBox->value();

    /* INITIALSV CALCULATION
     * solve for minInitialSV by substitution;
     *
     * find initialSV in terms of secondSV and threshold
     * averageSV = initialSV * threshold + (secondSV - secondSV * threshold)
     * initialSV = [ averageSV - secondSV * ( 1 - threshold ) ] / threshold
     */

    // Where secondSV = 0.1
    maxInitSV = (currentAverageSV - 0.1 * (1 - currentThreshold)) / currentThreshold > 10.0
                   ? 10.0
                   : (currentAverageSV - 0.1 * (1 - currentThreshold)) / currentThreshold;

    // Where secondSV = 10.0
    minInitSV = (currentAverageSV - 0.1 * (1 - currentThreshold)) / currentThreshold > 10.0
                   ? 10.0
                   : (currentAverageSV - 0.1 * (1 - currentThreshold)) / currentThreshold;

    // STOPPED
    ui->stutter_initSVSlider->setMaximum(maxInitSV * 100);
    ui->stutter_initSVSpinBox->setMaximum(maxInitSV);

    ui->stutter_initSVSlider->setMinimum(minInitSV * 100);
    ui->stutter_initSVSpinBox->setMinimum(minInitSV);

}

void svTool::on_stutter_averageSVSpinBox_valueChanged(double arg1)
{

}



//Stutter Generate Button
void svTool::on_pushButton_clicked()
{
    QStringList partVector, rawInputVector;
    QString partString, rawInputString;
    QList<double> uniqueOffsetList;
    double uniqueOffset;

    //Clears the procOutputBox
    ui->stutter_procOutputBox->clear();

    //Set input vector
    rawInputVector = ui->debug_procInputBox->toPlainText().split("\n");

    //Set up uniqueOffsetList
    foreach (rawInputString, rawInputVector)
    {
        partVector = rawInputString.split("|");
        if (!uniqueOffsetList.contains(partVector.at(2).toDouble()))
            uniqueOffsetList.append(partVector.at(2).toDouble());
    }

    foreach (uniqueOffset, uniqueOffsetList)
    {
        double rThreshold, pThreshold, minThreshold, maxThreshold;
        double initialSV, secondSV, minInitialSV, maxInitialSV;

        /* THRESHOLD CALCULATION
         * solve for threshold
         * 1 = 10 * threshold + (0.1 - 0.1 * threshold)
         * 0.9 = 9.9 * threshold
         * 0.09090909090909090909090909090909 = threshold
         * threshold min: 9.09, threshold max: 90.90, range: 81.81
         *
         * convert from raw threshold to proc threshold:
         * pThreshold = (rThreshold / 100 * 81.81) + 9.09
         */


        rThreshold = ui->stutter_thresholdSpinBox->value();
        minThreshold = 9.09;
        maxThreshold = 90.9;

        //Calculate Processed Threshold
        pThreshold = (rThreshold / 100 * (maxThreshold - minThreshold)) + minThreshold;

        //Find minInitialSV and maxInitialSV



        maxInitialSV = (1 - 0.1 * (1 - pThreshold)) / pThreshold > 10.0
                       ? 10.0
                       : (1 - 0.1 * (1 - pThreshold)) / pThreshold;
        minInitialSV = (1 - 10.0 * (1 - pThreshold)) / pThreshold < 0.1
                       ? 0.1
                       : (1 - 10.0 * (1 - pThreshold)) / pThreshold;


    }
}


