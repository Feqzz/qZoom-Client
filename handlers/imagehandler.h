#ifndef IMAGEHANDLER_H
#define IMAGEHANDLER_H

#include <QObject>
#include <QImage>
#include <QBrush>
#include <QPainter>
#include <QQuickImageProvider>
#include <QtConcurrent/QtConcurrent>
#include "core/settings.h"
#include "core/participant.h"
extern "C" {
#include "libavutil/frame.h"
#include "libavformat/avformat.h"
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}


class ImageHandler : public QObject, public QQuickImageProvider
{
    Q_OBJECT
public:
    explicit ImageHandler(Settings* settings);
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
    void readPacket(uint8_t *buffer, int buffer_size);
    void readImage(AVCodecContext* codecContext, AVFrame* scaledFrame, uint8_t index);
    void addPeer(uint8_t index, const QString& displayName);
    void removePeer(uint8_t index);
    void updatePeerDisplayName(uint8_t index, const QString& displayName);
    void removeAllPeers();
    void setPeerVideoAsDisabled(uint8_t index);
    void setPeerAudioIsDisabled(uint8_t index, bool val);
    void toggleBorder(bool talking, int index);
    void kickYourself();
    Q_INVOKABLE int getNumberOfScreens() const;
    Q_INVOKABLE bool getIsTalking(int index) const;
    Q_INVOKABLE bool getAudioIsDisabled(int index) const;
    Q_INVOKABLE QList<QString> getAllParticipantsDisplayNames() const;

public slots:
    void updateImage(const QImage &image, uint8_t index);

signals:
    void imageChanged();
    void refreshScreens();
    void forceLeaveSession();

private:
    std::mutex mImageLock;
    QImage generateGenericImage(const QString& username) const;
    uint8_t getCorrectIndex(int index) const;
    std::map<uint8_t, Participant*> mImageMap;
    QImage mDefaultImage;
    Settings* mSettings;
};

#endif // IMAGEHANDLER_H
