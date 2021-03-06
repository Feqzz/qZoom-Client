#include "udpsockethandler.h"

/**
 * @brief UdpSocketHandler::UdpSocketHandler
 * @param bufferSize
 * @param _port
 * @param inputStreamHandler
 * @param streamId
 * @param roomId
 * @param address
 * @param parent
 */
UdpSocketHandler::UdpSocketHandler(int bufferSize, int _port, InputStreamHandler* inputStreamHandler,
                             QString streamId, QString roomId, QHostAddress address, QObject *parent) : QObject(parent)
{
    mBufferSize = bufferSize;
    mInputStreamHandler = inputStreamHandler;
    mAddress = address;
    mPort = _port;
    mStreamId = streamId;
    mRoomId = roomId;
    initSocket();

    if ((mCppUdpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        exit(1);
    }

    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(mPort);

    qDebug() << "UDPSOCKET" << mAddress.toString().toUtf8();

    if (inet_aton(mAddress.toString().toUtf8() , &si_other.sin_addr) == 0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
}

/**
 * @brief UdpSocketHandler::~UdpSocketHandler
 */
UdpSocketHandler::~UdpSocketHandler()
{
    delete mUdpSocket;
    mUdpSocket = nullptr;
}
/**
 * Creates a new QUdepSocket and makes it listen to port and QHostAddress::Any.
 * When we used mAddress(ip address to the server) it did not recieve any datagrams.
 * It also connects the readyRead signal to the readPendingDatagram function.
 */
void UdpSocketHandler::initSocket()
{
    mUdpSocket = new QUdpSocket(this);
    mUdpSocket->bind(QHostAddress::Any, mPort, QAbstractSocket::ShareAddress);
    connect(mUdpSocket, &QUdpSocket::readyRead, this, &UdpSocketHandler::readPendingDatagrams);
}

/**
 * @brief UdpSocketHandler::closeSocket
 */
void UdpSocketHandler::closeSocket()
{
    qDebug() << "Closing SocketHandler";
    mUdpSocket->abort();
    mUdpSocket->close();
}

/**
 * This function will run when theQUdpSocket send the readyRead signal
 * It will find and remove the streamId from the datagram, then do the same with
 * a single byte which will let us know if the datagram is audio or video.
 * The function will use the streamId to send the datagram to the correct buffer/playbackhandler
 */
void UdpSocketHandler::readPendingDatagrams()
{
    while (mUdpSocket->hasPendingDatagrams())
    {
        QNetworkDatagram datagram = mUdpSocket->receiveDatagram();
        if(datagram.senderAddress().toIPv4Address() != mAddress.toIPv4Address())
        {
            continue;
        }
        QByteArray data = datagram.data();

        const int streamIdLength = data[0];
        data.remove(0, 1);

        //Finds the streamId header, stores it and removes it;
        QByteArray streamIdArray = QByteArray(data, streamIdLength);
        QString streamId(streamIdArray);
        data.remove(0, streamIdLength);

        const int index = mInputStreamHandler->findStreamIdIndex(streamId);
        if(index < 0)
        {
            continue;
        }

        //Checks the first byte in the datagram to determine if the datagram is audio or video
        const int audioOrVideoInt = data[0];
        data.remove(0, 1);

        if(audioOrVideoInt == 0)
        {
            mInputStreamHandler->lockAudioMutex(index);
            mInputStreamHandler->appendToAudioBuffer(index, data);
            mInputStreamHandler->unlockAudioMutex(index);

            if (!mInputStreamHandler->audioPlaybackStarted(index) && mInputStreamHandler->getAudioBufferSize(index) >= 1024*1)
            {
                *mInputStreamHandler->getAudioFutures(index) = QtConcurrent::run(mInputStreamHandler->getAudioPlaybackHandler(index), &AudioPlaybackHandler::start);
                qDebug() << "Starting AudioPlaybackHandler for streamId: " << streamId;
                mInputStreamHandler->setAudioPlaybackStarted(index, true);
            }
        }
        else if (audioOrVideoInt == 1)
        {
            mInputStreamHandler->lockVideoMutex(index);

            if(!mInputStreamHandler->videoHeaderVectorIsEmpty(index))
            {
                mInputStreamHandler->appendToVideoBuffer(index, data);
            }

            mInputStreamHandler->unlockVideoMutex(index);

            if(!mInputStreamHandler->videoPlaybackStarted(index))
            {
                *mInputStreamHandler->getVideoFutures(index) = QtConcurrent::run(mInputStreamHandler->getVideoPlaybackHandler(index), &VideoPlaybackHandler::start);
                qDebug() << "STARTING VIDEOPLAYBACKHANDLER!!!";
                mInputStreamHandler->setVideoPlaybackStarted(index, true);
            }
        }
        else
        {
            qDebug() << "UDP Header byte was not 1 or 0 in socketHandler, stopping program";
            exit(-1);
        }
    }
    mSignalCount++;
}
/**
 * Prints how many bytes were sent during the last x seconds
 * @param bytes
 */
void UdpSocketHandler::printBytesPerSecond(int bytes)
{
    const int secondsBetweenPrints = 1;
    static int totalSent = 0;
    static qint64 lastTime = 0;
    qint64 seconds = QDateTime::currentSecsSinceEpoch();
    //First time sending a datagram
    if(lastTime == 0)
    {
        lastTime = seconds;
        totalSent += bytes;
        return;
    }
    //If enough time has passed, print and reset
    if(seconds - lastTime >= secondsBetweenPrints)
    {

        //qDebug() << "\nCurrently sending datagrams with a "<< totalSent/(1000*secondsBetweenPrints) << "kB/s\n";
        totalSent = 0;
        lastTime = seconds;
    }
    //Append to total
    else
    {
        totalSent += bytes;
    }
}

/**
 * Send the QByteArray through the current udpSocket,
 * if the size is larger than 512, it will be divided into smaller arrays.
 * The function will also prepend streamId and roomId from the SessionHandler
 * to the arrays.
 * @param arr QByteArray to be sent
 * @return Error code (0 if successful)
 */
int UdpSocketHandler::sendDatagram(QByteArray arr)
{
    const int datagramMaxSize = 2*512;
    if(mUdpSocket == nullptr)
    {
        return AVERROR_EXIT;
    }
    int ret = 0;
    /*
     * In order for the dividing to work, we need to remove the audioOrVideo byte
     * at the start of the arr, and prepend it to the smaller arrays
    */
    //qDebug() << arr.size();

    //Creats a new QByteArray from the first byte in arr, which should be the audioOrVideo byte.
    //Then it removes the byte from arr
    QByteArray arrToPrepend = QByteArray(arr, 1);
    arr.remove(0, 1);

    //Puts the streamId and its size at the front of the array,
    //so the server knows where to send the stream
    arrToPrepend.prepend(mStreamId.toLocal8Bit().data());
    arrToPrepend.prepend(mStreamId.size());

    //Puts the roomId and its size at the front of the array
    arrToPrepend.prepend(mRoomId.toLocal8Bit().data());
    arrToPrepend.prepend(mRoomId.size());

    while (arr.size() > 0)
    {
        if (arr.size() > (datagramMaxSize - arrToPrepend.size()))
        {
            /*
             * Creates a deep copy of x bytes in arr.
             * Prepends the QByteArray containing roomId, streamId, audioOrVideoByte.
             * Then removes x bytes from arr.
             */
            QByteArray temp = QByteArray(arr, (datagramMaxSize - arrToPrepend.size()));
            temp.prepend(arrToPrepend);
            arr.remove(0, (datagramMaxSize - arrToPrepend.size()));
            printBytesPerSecond(temp.size());
            ret += sendto(mCppUdpSocket, temp, temp.size(), 0 , (struct sockaddr *) &si_other, slen);
        }
        else
        {
            /* We do not remove anything from arr if it is
             * smaller than (datagramMaxSize - arrToPrepend.size()),
             * so we need the break to leave the while loop
            */
            arr.prepend(arrToPrepend);
            printBytesPerSecond(arr.size());
            ret += sendto(mCppUdpSocket, arr, arr.size(), 0 , (struct sockaddr *) &si_other, slen);
            break;
        }

        if(ret < 0)
        {
            //TODO this should also print c++ socket error, could end up here with
            //mUdpSocket->error(); not printing anything usefull
            qDebug() << mUdpSocket->error();
            qDebug() << ret;
            break;
        }
    }
    return ret;
}
/**
 * On most routers, if we do not "open" the port by sending a datagram with QAbstractSocket
 * it will block incoming datagrams, and the program will recieve nothing
 * @param data QByteArray containing data to be sent
 * @return int with how many bytes sent, or the error code
 */
void UdpSocketHandler::openPortHack()
{
    QByteArray data;
    data.append(int(0));
    const int error = mUdpSocket->writeDatagram(data, data.size(), mAddress, mPort);
    if(error < 0)
    {
        qDebug() << mUdpSocket->error();
        qDebug() << error;
    }
}

