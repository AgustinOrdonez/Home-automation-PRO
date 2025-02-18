#include "port.h"
#include <QThread>

port::port() {
    m_Puerto = new QSerialPort();
    m_Puerto->setBaudRate(QSerialPort::Baud9600);
    m_Puerto->setDataBits(QSerialPort::Data8);
    m_Puerto->setFlowControl(QSerialPort::NoFlowControl);
    m_Puerto->setStopBits(QSerialPort::OneStop);
    m_Puerto->setParity(QSerialPort::NoParity);
}

port::port(QString PortNew): m_PortName{PortNew}{
    m_Puerto = new QSerialPort(this);
    m_Puerto->setPortName(PortNew);
    m_Puerto->setBaudRate(QSerialPort::Baud9600);
    m_Puerto->setDataBits(QSerialPort::Data8);
    m_Puerto->setFlowControl(QSerialPort::NoFlowControl);
    m_Puerto->setStopBits(QSerialPort::OneStop);
    m_Puerto->setParity(QSerialPort::NoParity);
}


void port::SetPort(QString Puerto){
    m_PortName = Puerto;
    m_Puerto->setPortName(Puerto);
}

bool port::OpenPort()
{

    if (m_Puerto->open(QIODevice::ReadWrite)) {
        // Si el puerto se abrió correctamente, conectar la señal readyRead al slot onReadyRead
        connect(m_Puerto, &QSerialPort::readyRead, this, &port::onReadyRead);
        qDebug() << "Puerto serie abierto:" << m_PortName;
        return true;
    } else {
        // Si hubo un error al abrir el puerto, mostrar un mensaje de error y limpiar la instancia de QSerialPort
        qDebug() << "Error al abrir el puerto serie:" << m_PortName;
        this->ClosePort();
    }
    return false;
}

port::~port()
{
    this->ClosePort();
}

QList<QSerialPortInfo> port::GetTotalsPorts()
{
    m_Totals_Ports = QSerialPortInfo::availablePorts();
    return m_Totals_Ports;
}

void port::onReadyRead() {
    QString Lectura = QString::fromUtf8(m_Puerto->readAll());
    int cantLectura = Lectura.length();
    
    if (m_Puerto && m_Puerto->isOpen()) {
        
        for (int i = 0; i < cantLectura; i++)
        {
            m_Dato[m_IndexRead].append(Lectura[i]);
            if(Lectura[i] == FOOTER){
                m_IndexRead++;
                if(m_IndexRead == DATO_BUFFER){
                    m_IndexRead = 0;
                }
            }
        }



        /* for (int i = 0; i < 3; i++)
        {
            if(m_Dato[i].contains('$') && m_Dato[i].contains('%')){
                continue;
            }

            if(Lectura.contains('%')){
                int posicion = Lectura.indexOf('%');
                m_Dato[i].append(Lectura.left(posicion));
            }
            if()

        } 
        



        if(m_Dato == nullptr){
            m_Dato = "";
        }
        if((m_Dato.contains('$') || Lectura.contains('$') && !(m_Dato.contains('%')) && m_Flag == false)){
            m_Dato.append(Lectura);
        }
        if(m_Dato.contains('%')){
            m_Flag = true;
        }
        */
    }
}




void port::ClosePort() {
    if (m_Puerto && m_Puerto->isOpen()) {
        m_Puerto->close();
        QObject::disconnect(m_Puerto, &QSerialPort::readyRead, this, &port::onReadyRead);
        qDebug() << "Puerto serie cerrado.";
    }
}


bool port::GetPortStatus(){
    if(m_Puerto->isOpen()){
        return 1;
    }else{
        return 0;
    }
}

bool port::SendData(QString Data)
{
    for(int i = 0; i < 2; i++){
        qint64 bytesWritten = m_Puerto->write(Data.toUtf8());
    
        if (bytesWritten == -1) {
            qDebug() << "Error al escribir en el puerto serial:" << m_Puerto->errorString();
            return 0;
        } else if (bytesWritten != Data.size()) {
            qDebug() << "No se escribieron todos los bytes en el puerto serial.";
            return 0;
        } else {
            qDebug() << "Escritura exitosa. Esperando confirmación...";
            QThread::msleep(30);

            /*
            if (m_Puerto->waitForBytesWritten(3000)) {  // Espera un máximo de 3000 milisegundos (3 segundos)
                return 1;
                qDebug() << "Confirmación recibida. Escritura completada.";
            } else {
                qDebug() << "Error: No se recibió confirmación dentro del tiempo especificado.";
                return 0;
            }
            */
        }
    }
    return 1;
}

SerialParams port::GetDato()
{
    SerialParams Valor;
    Valor.Info = nullptr;
    Valor.Param = nullptr;

    if(m_IndexRead != m_IndexWrite){
        QString datoStr = m_Dato[m_IndexWrite];
        m_Dato[m_IndexWrite] = "";
        m_IndexWrite++;
        //qDebug() << datoStr;
        if(m_IndexWrite == DATO_BUFFER){
            m_IndexWrite = 0;
        }

        int index = datoStr.indexOf(':');

        if (index != -1 && datoStr.at(0) == HEADER && datoStr.at(datoStr.length() - 1) == FOOTER) {
            // Separa el dato antes y después de ':'
            Valor.Param = datoStr.mid(1, index - 1);
            Valor.Info = datoStr.mid(index + 1).chopped(1);
        }
        qDebug() << Valor.Param << "write:" << Valor.Info;

    }
    return Valor;
}
