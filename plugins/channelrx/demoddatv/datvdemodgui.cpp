///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2018 F4HKW                                                      //
// for F4EXB / SDRAngel                                                          //
// using LeanSDR Framework (C) 2016 F4DAV                                        //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
// (at your option) any later version.                                           //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#include <QDockWidget>
#include <QMainWindow>
#include <QMediaMetaData>

#include "datvdemodgui.h"

#include "device/devicesourceapi.h"
#include "device/deviceuiset.h"
#include "device/devicesourceapi.h"
#include "dsp/downchannelizer.h"

#include "dsp/threadedbasebandsamplesink.h"
#include "ui_datvdemodgui.h"
#include "plugin/pluginapi.h"
#include "util/simpleserializer.h"
#include "util/db.h"
#include "dsp/dspengine.h"
#include "gui/crightclickenabler.h"
#include "gui/audioselectdialog.h"
#include "mainwindow.h"

const QString DATVDemodGUI::m_strChannelID = "sdrangel.channel.demoddatv";

DATVDemodGUI* DATVDemodGUI::create(PluginAPI* objPluginAPI,
        DeviceUISet *deviceUISet,
        BasebandSampleSink *rxChannel)
{
    DATVDemodGUI* gui = new DATVDemodGUI(objPluginAPI, deviceUISet, rxChannel);
    return gui;
}

void DATVDemodGUI::destroy()
{
    delete this;
}

void DATVDemodGUI::setName(const QString& strName)
{
    setObjectName(strName);
}

QString DATVDemodGUI::getName() const
{
    return objectName();
}

qint64 DATVDemodGUI::getCenterFrequency() const
{
    return m_objChannelMarker.getCenterFrequency();
}

void DATVDemodGUI::setCenterFrequency(qint64 intCenterFrequency)
{
    m_objChannelMarker.setCenterFrequency(intCenterFrequency);
    applySettings();
}

void DATVDemodGUI::resetToDefaults()
{
    m_settings.resetToDefaults();
    displaySettings();
	applySettings(true);
}

QByteArray DATVDemodGUI::serialize() const
{
    return m_settings.serialize();
}

bool DATVDemodGUI::deserialize(const QByteArray& arrData)
{
    if (m_settings.deserialize(arrData))
    {
        displaySettings();
        applySettings(true);
        return true;
    }
    else
    {
        resetToDefaults();
        return false;
    }
}

bool DATVDemodGUI::handleMessage(const Message& objMessage)
{
    (void) objMessage;
    return false;
}

void DATVDemodGUI::channelMarkerChangedByCursor()
{
    ui->deltaFrequency->setValue(m_objChannelMarker.getCenterFrequency());

    if(m_intCenterFrequency!=m_objChannelMarker.getCenterFrequency())
    {
        m_intCenterFrequency=m_objChannelMarker.getCenterFrequency();
        applySettings();
    }
}

void DATVDemodGUI::channelMarkerHighlightedByCursor()
{
    setHighlighted(m_objChannelMarker.getHighlighted());
}


void DATVDemodGUI::onWidgetRolled(QWidget* widget, bool rollDown)
{
    (void) widget;
    (void) rollDown;
}

void DATVDemodGUI::onMenuDoubleClicked()
{
}

//DATVDemodGUI::DATVDemodGUI(PluginAPI* objPluginAPI, DeviceSourceAPI *objDeviceAPI, QWidget* objParent) :
DATVDemodGUI::DATVDemodGUI(PluginAPI* objPluginAPI, DeviceUISet *deviceUISet, BasebandSampleSink *rxChannel, QWidget* objParent) :
        RollupWidget(objParent),
        ui(new Ui::DATVDemodGUI),
        m_objPluginAPI(objPluginAPI),
        m_deviceUISet(deviceUISet),
        m_objChannelMarker(this),
        m_blnBasicSettingsShown(false),
        m_blnDoApplySettings(true)
{
    ui->setupUi(this);
    ui->screenTV->setColor(true);
    setAttribute(Qt::WA_DeleteOnClose, true);
    connect(this, SIGNAL(widgetRolled(QWidget*,bool)), this, SLOT(onWidgetRolled(QWidget*,bool)));

    m_objDATVDemod = (DATVDemod*) rxChannel;
    m_objDATVDemod->setMessageQueueToGUI(getInputMessageQueue());

    m_objDATVDemod->SetTVScreen(ui->screenTV);

    connect(m_objDATVDemod->SetVideoRender(ui->screenTV_2), &DATVideostream::onDataPackets, this, &DATVDemodGUI::on_StreamDataAvailable);
    connect(ui->screenTV_2, &DATVideoRender::onMetaDataChanged, this, &DATVDemodGUI::on_StreamMetaDataChanged);

    m_intPreviousDecodedData=0;
    m_intLastDecodedData=0;
    m_intLastSpeed=0;
    m_intReadyDecodedData=0;
    m_objTimer.setInterval(1000);
    connect(&m_objTimer, SIGNAL(timeout()), this, SLOT(tick()));
    m_objTimer.start();

    ui->fullScreen->setVisible(false);

    ui->deltaFrequencyLabel->setText(QString("%1f").arg(QChar(0x94, 0x03)));
    ui->deltaFrequency->setColorMapper(ColorMapper(ColorMapper::GrayGold));
    ui->deltaFrequency->setValueRange(false, 7, -9999999, 9999999);

    ui->rfBandwidth->setColorMapper(ColorMapper(ColorMapper::GrayYellow));
    ui->rfBandwidth->setValueRange(true, 7, 0, 9999999);

    m_objChannelMarker.blockSignals(true);
    m_objChannelMarker.setColor(Qt::magenta);
    m_objChannelMarker.setBandwidth(6000000);
    m_objChannelMarker.setCenterFrequency(0);
    m_objChannelMarker.blockSignals(false);
    m_objChannelMarker.setVisible(true);

    connect(&m_objChannelMarker, SIGNAL(changedByCursor()), this, SLOT(channelMarkerChangedByCursor()));
    connect(&m_objChannelMarker, SIGNAL(highlightedByCursor()), this, SLOT(channelMarkerHighlightedByCursor()));

    m_deviceUISet->registerRxChannelInstance(DATVDemod::m_channelIdURI, this);
    m_deviceUISet->addChannelMarker(&m_objChannelMarker);
    m_deviceUISet->addRollupWidget(this);

    // QPixmap pixmapTarget = QPixmap(":/film.png");
    // pixmapTarget = pixmapTarget.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    // ui->videoPlay->setAlignment(Qt::AlignCenter);
    // ui->videoPlay->setPixmap(pixmapTarget);

	CRightClickEnabler *audioMuteRightClickEnabler = new CRightClickEnabler(ui->audioMute);
	connect(audioMuteRightClickEnabler, SIGNAL(rightClick(const QPoint &)), this, SLOT(audioSelect()));

    resetToDefaults(); // does applySettings()
}

DATVDemodGUI::~DATVDemodGUI()
{
    m_deviceUISet->removeRxChannelInstance(this);
    delete m_objDATVDemod;
    delete ui;
}

void DATVDemodGUI::blockApplySettings(bool blnBlock)
{
    m_blnDoApplySettings = !blnBlock;
}

void DATVDemodGUI::displaySettings()
{
    m_objChannelMarker.blockSignals(true);
    blockApplySettings(true);

    m_objChannelMarker.setCenterFrequency(m_settings.m_centerFrequency);
    ui->deltaFrequency->setValue(m_settings.m_centerFrequency);
    m_objChannelMarker.setColor(m_settings.m_rgbColor);
    ui->chkAllowDrift->setChecked(m_settings.m_allowDrift);
    ui->chkFastlock->setChecked(m_settings.m_fastLock);
    ui->cmbFilter->setCurrentIndex((int) m_settings.m_filter);

    displayRRCParameters(((int) m_settings.m_filter == 2));

    ui->chkHardMetric->setChecked(m_settings.m_hardMetric);
    ui->spiRollOff->setValue((int) (m_settings.m_rollOff * 100.0f));
    ui->chkViterbi->setChecked(m_settings.m_viterbi);
    ui->audioMute->setChecked(m_settings.m_audioMute);
    ui->cmbFEC->setCurrentIndex((int) m_settings.m_fec);
    ui->cmbModulation->setCurrentIndex((int) m_settings.m_modulation);
    ui->cmbStandard->setCurrentIndex((int) m_settings.m_standard);
    ui->spiNotchFilters->setValue(m_settings.m_notchFilters);
    ui->rfBandwidth->setValue(m_settings.m_rfBandwidth);
    ui->spiSymbolRate->setValue(m_settings.m_symbolRate);
    ui->spiExcursion->setValue(m_settings.m_excursion);
    ui->audioVolume->setValue(m_settings.m_audioVolume);
    ui->audioVolumeText->setText(tr("%1").arg(m_settings.m_audioVolume));
    ui->videoMute->setChecked(m_settings.m_videoMute);

    blockApplySettings(false);
    m_objChannelMarker.blockSignals(false);
}

void DATVDemodGUI::applySettings(bool force)
{
    QString strStandard;
    QString strModulation;
    QString strFEC;

    if (m_blnDoApplySettings)
    {
        //Bandwidth and center frequency
        m_objChannelMarker.setCenterFrequency(ui->deltaFrequency->getValueNew());
        m_objChannelMarker.setBandwidth(ui->rfBandwidth->getValueNew());

        DATVDemod::MsgConfigureChannelizer *msgChan = DATVDemod::MsgConfigureChannelizer::create(m_objChannelMarker.getCenterFrequency());
        m_objDATVDemod->getInputMessageQueue()->push(msgChan);

        setTitleColor(m_objChannelMarker.getColor());

        strStandard = ui->cmbStandard->currentText();

        if(strStandard=="DVB-S") {
            m_settings.m_standard = DATVDemodSettings::DVB_S;
        } else if (strStandard=="DVB-S2") {
            m_settings.m_standard = DATVDemodSettings::DVB_S2;
        } else {
            m_settings.m_standard = DATVDemodSettings::DVB_S;
        }

        //BPSK, QPSK, PSK8, APSK16, APSK32, APSK64E, QAM16, QAM64, QAM256

        strModulation = ui->cmbModulation->currentText();

        if(strModulation=="BPSK") {
            m_settings.m_modulation = DATVDemodSettings::BPSK;
        }
        else if(strModulation=="QPSK") {
            m_settings.m_modulation = DATVDemodSettings::QPSK;
        }
        else if(strModulation=="8PSK") {
            m_settings.m_modulation = DATVDemodSettings::PSK8;
        }
        else if(strModulation=="16APSK") {
            m_settings.m_modulation = DATVDemodSettings::APSK16;
        }
        else if(strModulation=="32APSK") {
            m_settings.m_modulation = DATVDemodSettings::APSK32;
        }
        else if(strModulation=="64APSKE") {
            m_settings.m_modulation = DATVDemodSettings::APSK64E;
        }
        else if(strModulation=="16QAM") {
            m_settings.m_modulation = DATVDemodSettings::QAM16;
        }
        else if(strModulation=="64QAM") {
            m_settings.m_modulation = DATVDemodSettings::QAM64;
        }
        else if(strModulation=="256QAM") {
            m_settings.m_modulation = DATVDemodSettings::QAM256;
        } else {
            m_settings.m_modulation = DATVDemodSettings::BPSK;
        }

        //Viterbi only for BPSK et QPSK
        if ((m_settings.m_modulation != DATVDemodSettings::BPSK)
            && (m_settings.m_modulation != DATVDemodSettings::QPSK))
        {
            ui->chkViterbi->setChecked(false);
        }

        strFEC = ui->cmbFEC->currentText();

        if (strFEC == "1/2") {
            m_settings.m_fec = leansdr::FEC12;
        } else if (strFEC == "2/3") {
            m_settings.m_fec = leansdr::FEC23;
        } else if (strFEC == "3/4") {
            m_settings.m_fec = leansdr::FEC34;
        } else if (strFEC == "5/6") {
            m_settings.m_fec = leansdr::FEC56;
        } else if (strFEC == "7/8") {
            m_settings.m_fec = leansdr::FEC78;
        } else if (strFEC == "4/5") {
            m_settings.m_fec = leansdr::FEC45;
        } else if (strFEC == "8/9") {
            m_settings.m_fec = leansdr::FEC89;
        } else if (strFEC == "9/10") {
            m_settings.m_fec = leansdr::FEC910;
        } else {
            m_settings.m_fec = leansdr::FEC12;
        }

        if (ui->cmbFilter->currentIndex() == 0) {
            m_settings.m_filter = DATVDemodSettings::SAMP_LINEAR;
        } else if (ui->cmbFilter->currentIndex() == 1) {
            m_settings.m_filter = DATVDemodSettings::SAMP_NEAREST;
        } else {
            m_settings.m_filter = DATVDemodSettings::SAMP_RRC;
        }

        m_settings.m_rfBandwidth = m_objChannelMarker.getBandwidth();
        m_settings.m_centerFrequency = m_objChannelMarker.getCenterFrequency();
        m_settings.m_symbolRate = ui->spiSymbolRate->value();
        m_settings.m_notchFilters = ui->spiNotchFilters->value();
        m_settings.m_allowDrift = ui->chkAllowDrift->isChecked();
        m_settings.m_fastLock = ui->chkFastlock->isChecked();
        m_settings.m_hardMetric = ui->chkHardMetric->isChecked();
        m_settings.m_rollOff = ((float)ui->spiRollOff->value()) / 100.0f;
        m_settings.m_viterbi = ui->chkViterbi->isChecked();
        m_settings.m_excursion = ui->spiExcursion->value();
        m_settings.m_audioMute = ui->audioMute->isChecked();
        m_settings.m_audioVolume = ui->audioVolume->value();
        m_settings.m_videoMute = ui->videoMute->isChecked();

        QString msg = tr("DATVDemodGUI::applySettings: force: %1").arg(force);
        m_settings.debug(msg);

        DATVDemod::MsgConfigureDATVDemod* message = DATVDemod::MsgConfigureDATVDemod::create(m_settings, force);
	    m_objDATVDemod->getInputMessageQueue()->push(message);
    }
}

void DATVDemodGUI::leaveEvent(QEvent*)
{
    blockApplySettings(true);
    m_objChannelMarker.setHighlighted(false);
    blockApplySettings(false);
}

void DATVDemodGUI::enterEvent(QEvent*)
{
    blockApplySettings(true);
    m_objChannelMarker.setHighlighted(true);
    blockApplySettings(false);
}

void DATVDemodGUI::audioSelect()
{
    qDebug("AMDemodGUI::audioSelect");
    AudioSelectDialog audioSelect(DSPEngine::instance()->getAudioDeviceManager(), m_settings.m_audioDeviceName);
    audioSelect.exec();

    if (audioSelect.m_selected)
    {
        m_settings.m_audioDeviceName = audioSelect.m_audioDeviceName;
        applySettings();
    }
}

void DATVDemodGUI::tick()
{
    if (m_objDATVDemod)
    {
        m_objMagSqAverage(m_objDATVDemod->getMagSq());
        double magSqDB = CalcDb::dbPower(m_objMagSqAverage / (SDR_RX_SCALED*SDR_RX_SCALED));
        ui->channePowerText->setText(tr("%1 dB").arg(magSqDB, 0, 'f', 1));
    }

    if((m_intLastDecodedData-m_intPreviousDecodedData)>=0)
    {
        m_intLastSpeed = 8*(m_intLastDecodedData-m_intPreviousDecodedData);
        ui->lblRate->setText(QString("Speed: %1b/s").arg(formatBytes(m_intLastSpeed)));
    }

    if (m_objDATVDemod->audioActive())
    {
        if (m_objDATVDemod->audioDecodeOK()) {
            ui->audioMute->setStyleSheet("QToolButton { background-color : green; }");
        } else {
            ui->audioMute->setStyleSheet("QToolButton { background-color : red; }");
        }
    }
    else
    {
        ui->audioMute->setStyleSheet("QToolButton { background:rgb(79,79,79); }");
    }

    if (m_objDATVDemod->videoActive())
    {
		if (m_objDATVDemod->videoDecodeOK()) {
			ui->videoMute->setStyleSheet("QToolButton { background-color : green; }");
		} else {
			ui->videoMute->setStyleSheet("QToolButton { background-color : red; }");
		}
    }
    else
    {
        ui->videoMute->setStyleSheet("QToolButton { background:rgb(79,79,79); }");
    }

    m_intPreviousDecodedData = m_intLastDecodedData;

    //Try to start video rendering
    m_objDATVDemod->PlayVideo(false);

    return;
}

void DATVDemodGUI::on_cmbStandard_currentIndexChanged(const QString &arg1)
{
    (void) arg1;
    applySettings();
}

void DATVDemodGUI::on_cmbModulation_currentIndexChanged(const QString &arg1)
{
    (void) arg1;
    QString strModulation;
    QString strFEC;

    strFEC = ui->cmbFEC->currentText();

    strModulation = ui->cmbModulation->currentText();

    if(strModulation=="16APSK")
    {
        if((strFEC!="2/3")
            && (strFEC!="3/4")
            && (strFEC!="4/5")
            && (strFEC!="5/6")
            && (strFEC!="4/6")
            && (strFEC!="8/9")
            && (strFEC!="9/10"))
        {
            //Reset modulation to 2/3

            ui->cmbFEC->setCurrentIndex(1);
        }
        else
        {
            applySettings();
        }
    }
    else if(strModulation=="32APSK")
    {
        if((strFEC!="3/4")
            && (strFEC!="4/5")
            && (strFEC!="5/6")
            && (strFEC!="8/9")
            && (strFEC!="9/10"))
        {
            //Reset modulation to 3/4

            ui->cmbFEC->setCurrentIndex(2);
        }
        else
        {
            applySettings();
        }
    }
    else
    {
        applySettings();
    }

}

void DATVDemodGUI::on_cmbFEC_currentIndexChanged(const QString &arg1)
{
    (void) arg1;
    QString strFEC;

    strFEC = ui->cmbFEC->currentText();

    if(ui->cmbModulation->currentText()=="16APSK")
    {
        if((strFEC!="2/3")
            && (strFEC!="3/4")
            && (strFEC!="4/5")
            && (strFEC!="5/6")
            && (strFEC!="4/6")
            && (strFEC!="8/9")
            && (strFEC!="9/10"))
        {
            //Reset modulation to BPSK

            ui->cmbModulation->setCurrentIndex(0);
        }
        else
        {
            applySettings();
        }
    }
    else if(ui->cmbModulation->currentText()=="32APSK")
    {
        if((strFEC!="3/4")
            && (strFEC!="4/5")
            && (strFEC!="5/6")
            && (strFEC!="8/9")
            && (strFEC!="9/10"))
        {
            //Reset modulation to BPSK

            ui->cmbModulation->setCurrentIndex(0);
        }
        else
        {
            applySettings();
        }
    }
    else
    {
        applySettings();
    }

}

void DATVDemodGUI::on_chkViterbi_clicked()
{
    applySettings();
}

void DATVDemodGUI::on_chkHardMetric_clicked()
{
    applySettings();
}

void DATVDemodGUI::on_resetDefaults_clicked()
{
    resetToDefaults();
}

void DATVDemodGUI::on_spiSymbolRate_valueChanged(int arg1)
{
    (void) arg1;
    applySettings();
}

void DATVDemodGUI::on_spiNotchFilters_valueChanged(int arg1)
{
    (void) arg1;
    applySettings();
}

void DATVDemodGUI::on_chkAllowDrift_clicked()
{
     applySettings();
}

void DATVDemodGUI::on_fullScreen_clicked()
{
    ui->screenTV_2->SetFullScreen(true);
}

void DATVDemodGUI::on_mouseEvent(QMouseEvent* obj)
{
    (void) obj;
}

QString DATVDemodGUI::formatBytes(qint64 intBytes)
{
    if(intBytes<1024) {
        return QString("%1").arg(intBytes);
    } else if(intBytes<1024*1024) {
        return QString("%1 K").arg((float)(10*intBytes/1024)/10.0f);
    } else if(intBytes<1024*1024*1024) {
        return QString("%1 M").arg((float)(10*intBytes/(1024*1024))/10.0f);
    }

    return QString("%1 G").arg((float)(10*intBytes/(1024*1024*1024))/10.0f);
}


void DATVDemodGUI::on_StreamDataAvailable(int *intPackets, int *intBytes, int *intPercent, qint64 *intTotalReceived)
{
    (void) intPackets;
    ui->lblStatus->setText(QString("Data: %1B").arg(formatBytes(*intTotalReceived)));
    m_intLastDecodedData = *intTotalReceived;

    if((*intPercent)<100) {
        ui->prgSynchro->setValue(*intPercent);
    } else {
        ui->prgSynchro->setValue(100);
    }

    m_intReadyDecodedData = *intBytes;
}

void DATVDemodGUI::on_spiBandwidth_valueChanged(int arg1)
{
    (void) arg1;
    applySettings();
}

void DATVDemodGUI::on_deltaFrequency_changed(qint64 value)
{
    (void) value;
    applySettings();
}

void DATVDemodGUI::on_rfBandwidth_changed(qint64 value)
{
    (void) value;
    applySettings();
}

void DATVDemodGUI::on_chkFastlock_clicked()
{
    applySettings();
}

void DATVDemodGUI::on_audioMute_toggled(bool checked)
{
    (void) checked;
	applySettings();
}

void DATVDemodGUI::on_videoMute_toggled(bool checked)
{
    (void) checked;
	applySettings();
}

void DATVDemodGUI::on_audioVolume_valueChanged(int value)
{
    ui->audioVolumeText->setText(tr("%1").arg(value));
    applySettings();
}

void DATVDemodGUI::on_StreamMetaDataChanged(DataTSMetaData2 *objMetaData)
{
    QString strMetaData="";

    if (objMetaData != nullptr)
    {
        if (objMetaData->OK_TransportStream == true)
        {
            strMetaData.sprintf("PID: %d - Width: %d - Height: %d\r\n%s%s\r\nCodec: %s\r\n",
                objMetaData->PID,
                objMetaData->Width,
                objMetaData->Height,
                objMetaData->Program.toStdString().c_str(),
                objMetaData->Stream.toStdString().c_str(),
                objMetaData->CodecDescription.toStdString().c_str());
        }

        ui->textEdit->setText(strMetaData);
        ui->chkData->setChecked(objMetaData->OK_Data);
        ui->chkTS->setChecked(objMetaData->OK_TransportStream);
        ui->chkVS->setChecked(objMetaData->OK_VideoStream);
        ui->chkDecoding->setChecked(objMetaData->OK_Decoding);

        if (objMetaData->Height > 0) {
            ui->screenTV_2->setFixedWidth((int)objMetaData->Width*(270.0f/(float)objMetaData->Height));
        }
    }
}

void DATVDemodGUI::displayRRCParameters(bool blnVisible)
{
    ui->spiRollOff->setVisible(blnVisible);
    ui->spiExcursion->setVisible(blnVisible);
    ui->rollOffLabel->setVisible(blnVisible);
    ui->excursionLabel->setVisible(blnVisible);
}

void DATVDemodGUI::on_cmbFilter_currentIndexChanged(int index)
{
    (void) index;
    displayRRCParameters((ui->cmbFilter->currentIndex() == 2));
    applySettings();
}

void DATVDemodGUI::on_spiRollOff_valueChanged(int arg1)
{
    (void) arg1;
    applySettings();
}

void DATVDemodGUI::on_spiExcursion_valueChanged(int arg1)
{
    (void) arg1;
    applySettings();
}
