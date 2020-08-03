#ifndef SESSIONHANDLER_H
#define SESSIONHANDLER_H

#include <QObject>
#include "core/database.h"
#include "handlers/userhandler.h"
#include <QHostInfo>
#include <QHostAddress>
#include <QNetworkInterface>
#include "imagehandler.h"
#include "inputstreamhandler.h"
#include "sockethandler.h"
#include "tcpsockethandler.h"
#include "streamhandler.h"

class SessionHandler : public QObject
{
    Q_OBJECT
public:
    explicit SessionHandler(Database* _db, UserHandler* _user,
                            ImageHandler* imageHandler,
                            Settings* settings, int bufferSize,
                            QHostAddress address, int port,
                            QObject *parent = nullptr);
    Q_INVOKABLE void updateDisplayName();
    Q_INVOKABLE void disableVideo();
    Q_INVOKABLE void disableAudio();
    Q_INVOKABLE bool joinSession(QString _roomId, QString _roomPassword);
    Q_INVOKABLE bool createSession(QString _roomId, QString _roomPassword);
    Q_INVOKABLE bool leaveSession();
    Q_INVOKABLE bool isGuest();
    Q_INVOKABLE bool enableVideo();
    Q_INVOKABLE bool enableAudio();
    Q_INVOKABLE bool checkVideoEnabled();
    Q_INVOKABLE bool checkAudioEnabled();
    Q_INVOKABLE QString getRoomId();
    Q_INVOKABLE QString getRoomPassword();
    Q_INVOKABLE QString getRoomHostUsername();
    Q_INVOKABLE QVariantList getAudioInputDevices();
    UserHandler* getUser();

private:
    void addUser();
    void initOtherStuff();
    void closeOtherStuff();
    void getUserRoom();
    void setDefaultRoomID();
    bool addGuestUserToDatabase();
    bool mUserHasRoom;
    bool mSessionIsActive;
    int mBufferSize;
    int mPort;
    QString mRoomId;
    QString mRoomPassword;
    QString mRoomHostUsername;
    QString mIpAddress;
    QHostAddress mAddress;
    Database* mDb;
    UserHandler* mUser;
    Settings* mSettings;
    StreamHandler* mStreamHandler;
    ImageHandler* mImageHandler;
    InputStreamHandler* mInputStreamHandler;
    SocketHandler* mSocketHandler;
    TcpSocketHandler* mTcpSocketHandler;

signals:

};

#endif // SESSIONHANDLER_H
