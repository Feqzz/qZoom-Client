#ifndef SOCKETHANDLER_H
#define SOCKETHANDLER_H

#include <QObject>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QNetworkDatagram>
#include <QProcess>
#include <mutex>
#include "handlers/imagehandler.h"
#include "handlers/videoplaybackhandler.h"
#include "handlers/audioplaybackhandler.h"

class VideoPlaybackHandler;
class AudioPlaybackHandler;
class InputStreamHandler;
class UdpSocketHandler : public QObject
{
    Q_OBJECT
public:
    explicit UdpSocketHandler(int bufferSize, int port, InputStreamHandler* inputStreamHandler, QString streamId, QString roomId, QHostAddress address, QObject *parent = nullptr);
    void initSocket();
    void closeSocket();
    QTcpSocket* mTCPSocket;
    QUdpSocket* mUdpSocket;
    int sendDatagram(QByteArray arr);
    QHostAddress mAddress;
    int mPort;

public slots:
    void readPendingDatagrams();

private:
    void addStreamToVector(QString, int);
    int findStreamIdIndex(QString);
    int mBufferSize;
    QString mRoomId;
    QString mStreamId;
    InputStreamHandler* mInputStreamHandler;
    uint signalCount = 0;
    struct mBufferAndLockStruct
    {
        QByteArray* buffer;
        std::mutex* writeLock;
    };
};

#endif // SOCKETHANDLER_H