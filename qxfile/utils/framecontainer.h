#ifndef FRAMECONTAINER_H
#define FRAMECONTAINER_H

//#include <QMetaType>
#include <QString>
#include <QRect>
#include <QDebug>
#include <QList>
#include "selection.h"


class ImageInfo
{
public:
    ImageInfo();
    ImageInfo(const ImageInfo & other);
    ~ImageInfo();

    void setPath(QString str);
    const QString path() const;

    void setSelection(Selection rect);
    void setBackground(Selection rect);
    Selection selection() const;
    Selection background() const;

    ImageInfo &operator =(ImageInfo other);

private:
    QString p_path;
    Selection p_selection;
    Selection p_background;
};

Q_DECLARE_METATYPE(ImageInfo);

QDebug operator<<(QDebug dbg, const ImageInfo &image);

QDataStream &operator<<(QDataStream &out, const ImageInfo &image);
QDataStream &operator>>(QDataStream &in, ImageInfo &image);

class ImageSeries
{
public:
    ImageSeries();
    ImageSeries(const ImageSeries & other);
    ~ImageSeries();

    void setPath(QString str);
    const QString path() const;
    int size() const;
    int i() const;
    
    QStringList paths();
    
    void setSelection(Selection rect);
    void setImages(QList<ImageInfo> list);
    void clear();
    void removeCurrent();
    void append(ImageInfo image);
    void rememberCurrent();
    void restoreMemory();
    const QList<ImageInfo> &images() const;

    ImageInfo * current();
    ImageInfo * next();
    ImageInfo * previous();
    ImageInfo * begin();
    ImageInfo * at(int value);

    bool operator == (const ImageSeries&);

private:
    QString p_path;
    QList<ImageInfo> p_images;
    int p_i;
    int p_i_memory;

};

Q_DECLARE_METATYPE(ImageSeries);

QDebug operator<<(QDebug dbg, const ImageSeries &image_series);

QDataStream &operator<<(QDataStream &out, const ImageSeries &image_series);
QDataStream &operator>>(QDataStream &in, ImageSeries &image_series);

ImageSeries &operator<<(ImageSeries &image_series, const ImageInfo &image);

class SeriesSet
{
public:
    SeriesSet();
    SeriesSet(const SeriesSet & other);
    ~SeriesSet();

    ImageSeries * current();
    ImageSeries * next();
    ImageSeries * previous();
    ImageSeries * begin();
    

    QStringList paths();
    ImageSeries oneSeries();
    
    bool isEmpty();

    void rememberCurrent();
    void restoreMemory();
    void clear();
    void append(ImageSeries image_series);
    void removeCurrent();
    void setFolders(QList<ImageSeries> list);
    const QList<ImageSeries> &series() const;
    int size() const;
    
private:
    QList<ImageSeries> p_seriess;
    int p_i;
    int p_i_memory;
};

Q_DECLARE_METATYPE(SeriesSet);

QDebug operator<<(QDebug dbg, const SeriesSet &series_set);

QDataStream &operator<<(QDataStream &out, const SeriesSet &series_set);
QDataStream &operator>>(QDataStream &in, SeriesSet &series_set);

SeriesSet &operator<<(SeriesSet &series_set, const ImageSeries &image_series);

#endif // FRAMECONTAINER_H
