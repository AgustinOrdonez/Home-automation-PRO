#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
QMainWindow(parent),
ui(new Ui::MainWindow) {
    
    ui->setupUi(this);
    QList<QSerialPortInfo> Puertos = Puerto.GetTotalsPorts();
    for (int index = 0; index < Puertos.size(); index++) {
        ui->comboBox->addItem(Puertos.at(index).portName());
    }
    if(ui->pushButton->text() == "Conectar"){
        ui->Login->setEnabled(false);
    }else{
        ui->Login->setEnabled(true);
    }
    Dial = new Dialog(this);


    // Cargar el archivo GIF
    movie = new QMovie(":/loader.gif");

    // Crear un QLabel y configurar el GIF
    label = new QLabel(this);
    movie->setScaledSize(QSize(50, 50));
    label->setMovie(movie);
    label->hide();
    // Iniciar la animación del GIF


    // Agregar el QLabel a la ventana principal
    int x = (this->width() - label->width()) / 2;
    int y = ((this->height() - label->height()) / 2) - 50;
    label->move(x, y);
    label->setFixedSize(75, 75); // Establecer un tamaño fijo

    inicializateTimer(PuertoConnectTimer, &MainWindow::verificatePort);

    inicializateTimer(TimerPort, &MainWindow::PortsAvailable);
    ui->PortConfirm->setStyleSheet("background-color: red;");
    connect(ui->lineEdit_user, &QLineEdit::textChanged, this, &MainWindow::CreateUserEnable);
    connect(ui->lineEdit_password, &QLineEdit::textChanged, this, &MainWindow::CreateUserEnable);
    ui->CreateUser->setEnabled(false);

    //if(Puertos.size() > 0){
    //    Puerto.SetPort(Puertos.at(0).portName());
    //}else{
    //    qDebug() << "No hay un puerto disponible";
    //}
    Dial->setWindowTitle("Home Controller");
    ui->label_confirmLogin->setEnabled(false);

}

MainWindow::~MainWindow() {
    Puerto.ClosePort();
    CancelateTimer(TimerPort);
    CancelateTimer(PuertoConnectTimer);
    //delete Puerto;
    delete Dial;
    delete ui;


    /*
    puerto->close();
    delete puerto;
    delete ui;
    */
}



void MainWindow::inicializateTimer(QTimer *timer, void (MainWindow::*func)())
{
    timer = new QTimer(this);
    timer->setInterval(50);
    connect(timer,  &QTimer::timeout, this, func);
    timer->start();
}


void MainWindow::CancelateTimer(QTimer *timer)
{
    timer->stop();
    delete timer;
    timer = nullptr;
}




/*
void MainWindow::onDatosRecibidos() {

    QByteArray bytes;
   // int cant = m_Puerto->m_Puerto->bytesAvailable();
   //bytes.resize(cant);
   // m_Puerto->m_Puerto->read(bytes.data(), bytes.size());
   // ui->recieveLINE->insert(bytes);
    // m_datos_recibidos += bytes;
    // m_datos_recibidos = bytes;

}
*/




/*
void MainWindow::on_connectBTN_clicked()
{

    if(Puerto.GetPortStatus()){
        Puerto.ClosePort();
        //delete Puerto;
        //Puerto = nullptr;
        //ui->statusLB->setStyleSheet("font-weight: bold; color: black; background-color: lightgray;");
       //ui->statusLB->setText("NO CONNECTED");
        //ui->connectBTN->setText("CONNECT");
    }else{
        //m_Puerto->SetPort(ui->selectionCOM->currentText());
        if(Puerto.OpenPort()){
            //ui->statusLB->setStyleSheet("font-weight: bold; color: black; background-color: lightgreen;");
            //ui->statusLB->setText("CONNECTED");
            //ui->connectBTN->setText("DISCONNECT");
        }
    }


    if (!puerto) {
        puerto = new QSerialPort(ui->selectionCOM->currentText());
        puerto->setBaudRate(QSerialPort::Baud9600);
        puerto->setDataBits(QSerialPort::Data8);
        puerto->setFlowControl(QSerialPort::NoFlowControl);
        puerto->setStopBits(QSerialPort::OneStop);
        puerto->setParity(QSerialPort::NoParity);

        if (puerto->open(QIODevice::ReadWrite)) {
            ui->statusLB->setStyleSheet("font-weight: bold; color: black; background-color: lightgreen;");
            ui->statusLB->setText("CONNECTED");
            ui->connectBTN->setText("DISCONNECT");
            connect(puerto, SIGNAL(readyRead()), this, SLOT(onDatosRecibidos()));
        }
    } else {
        puerto->close();
        delete puerto;
        puerto = nullptr;
        ui->statusLB->setStyleSheet("font-weight: bold; color: black; background-color: lightgray;");
        ui->statusLB->setText("NO CONNECTED");
        ui->connectBTN->setText("CONNECT");
    }

}
*/
/*
void MainWindow::on_sendBTN_clicked()
{
    QByteArray send;
    if (Puerto.GetPortStatus()) {
        //send += (ui->transmitLINE->text()).toLocal8Bit();
        if(Puerto.SendData(send)){
            //ui->transmitLINE->clear();
        }else{
            qDebug() << "Error";
        }
    }
}
*/



void MainWindow::on_pushButton_clicked()
{

    if(ui->pushButton->text() == "Conectar"){
        Puerto.SetPort(ui->comboBox->currentText());
        Puerto.OpenPort();
        ui->pushButton->setText("Desconectar");

    }else if(ui->pushButton->text() == "Desconectar"){
        Puerto.ClosePort();
        ui->pushButton->setText("Conectar");
        ui->comboBox->setEnabled(true);
        ui->Login->setEnabled(false);
    }
}


void MainWindow::on_Login_clicked()
{
    QString Usuario = ui->lineEdit_user->text();
    QString Contraseña = ui->lineEdit_password->text();
    if(Database.LoginUser(Usuario, Contraseña)){
        movie->start();
        label->show();
        Puerto.SendData("$connect:ok%");
        inicializateTimer(WaitingConnectTimer, &MainWindow::WaitingConnect);
        ui->lineEdit_password->clear();
        ui->lineEdit_user->clear();
        DialogActive = false;
    }else{
        ui->label_confirmLogin->setEnabled(true);
        ui->label_confirmLogin->setStyleSheet(" color: #D8000C;"
                                                "font-size: 14px;"
                                                "font-weight: bold;"
                                                "background-color: #FFD2D2;"
                                                "border: 1px solid #B30000; "
                                                "border-radius: 5px; "
                                                "padding: 5px;");
        ui->label_confirmLogin->setText("Su usuario o contraseña es incorrecta");
    }
}


void MainWindow::on_CreateUser_clicked()
{
    Validator = new ValidatorModal(this, QString(ui->lineEdit_user->text()), QString(ui->lineEdit_password->text()));
    Validator->show();
}

void MainWindow::CreateUserEnable()
{
    if(ui->lineEdit_user->text().length() > 0 && ui->lineEdit_password->text().length() > 0){
        ui->CreateUser->setEnabled(true);
    }else{
        ui->CreateUser->setEnabled(false);
    }
}




void MainWindow::WaitingConnect()
{
    if(!DialogActive){
        Confirm = Puerto.GetDato();
    }
    if(Confirm.Info == "ok" && Confirm.Param == "connect" ){
        movie->stop();
        label->close();
        Confirm.Info = "";
        Confirm.Param = "";
        //CancelateTimer(TimerPort);
        //CancelateTimer(WaitingConnectTimer);
        //CancelateTimer(WaitingConnectTimer);
        Dial->show();
        DialogActive = true;
    }
}


void MainWindow::PortsAvailable(){

    bool change = false;
    QList<QSerialPortInfo> Puertos = Puerto.GetTotalsPorts();
    if(ui->comboBox->count() == Puertos.size()){
        for (int index = 0; index < ui->comboBox->count(); index++) {
            if (ui->comboBox->itemText(index) != Puertos.at(index).portName()) {
                change = true;
                break; // Salir del bucle si se encuentra un cambio
            }
        }
    }else{
        change = true;
    }


    if(change){
        ui->pushButton->setText("Conectar");
        ui->PortConfirm->setStyleSheet("background-color: red;");
        ui->PortConfirm->setText("Desconectado");
        if(Puerto.GetPortStatus()){
            Puerto.ClosePort();
        }
        ui->comboBox->clear();
        for (int index = 0; index < Puertos.size(); index++) {
            ui->comboBox->addItem(Puertos.at(index).portName());
        }
    }
    if(ui->comboBox->count() == 0){
        ui->Login->setEnabled(false);
        ui->pushButton->setEnabled(false);
        ui->comboBox->setEnabled(false);
    }else{
        ui->pushButton->setEnabled(true);
        ui->comboBox->setEnabled(true);
    }
}


void MainWindow::verificatePort(){

    if(Puerto.GetPortStatus()){
        ui->PortConfirm->setStyleSheet("background-color: green;");
        ui->PortConfirm->setText("Conectado");
        ui->comboBox->setEnabled(false);
        ui->Login->setEnabled(true);
        if(flagTime == false){
            flagTime = true;
            int Time = Database.GetTime();
            //qDebug() << Time;
            Puerto.SendData(QString("$time:%1%").arg(Time));
        }
    }else{
        ui->PortConfirm->setStyleSheet("background-color: red;");
        ui->PortConfirm->setText("Desconectado");
        flagTime = false;
    }
}
