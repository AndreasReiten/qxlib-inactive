#ifndef FRAMECONTAINER_H
#define FRAMECONTAINER_H

#include <QMetaType>
#include <QString>
#include <QRect>
#include <QDebug>
#include <QList>
#include "selection.h"


class Image
{
public:
    Image();
    Image(const Image & other);
    ~Image();

    void setPath(QString str);
    const QString path() const;

    void setSelection(Selection rect);
    void setBackground(Selection rect);
    const Selection selection() const;
    const Selection background() const;

private:
    QString p_path;
    Selection p_selection;
    Selection p_background;
};

Q_DECLARE_METATYPE(Image);

QDebug operator<<(QDebug dbg, const Image &image);

QDataStream &operator<<(QDataStream &out, const Image &image);
QDataStream &operator>>(QDataStream &in, Image &image);

class ImageFolder
{
public:
    ImageFolder();
    ImageFolder(const ImageFolder & other);
    ~ImageFolder();

    void setPath(QString str);
    const QString path() const;
    int size() const;
    int i() const;

    void setImages(QList<Image> list);
    void removeCurrent();
    void append(Image image);
    void rememberCurrent();
    void restoreMemory();
    const QList<Image> &images() const;

    Image * current();
    Image * next();
    Image * previous();
    Image * begin();

    bool operator == (const ImageFolder&);

private:
    QString p_path;
    QList<Image> p_images;
    int p_i;
    int p_i_memory;

};

Q_DECLARE_METATYPE(ImageFolder);

QDebug operator<<(QDebug dbg, const ImageFolder &image_folder);

QDataStream &operator<<(QDataStream &out, const ImageFolder &image_folder);
QDataStream &operator>>(QDataStream &in, ImageFolder &image_folder);

ImageFolder &operator<<(ImageFolder &image_folder, const Image &image);

class FolderSet
{
public:
    FolderSet();
    FolderSet(const FolderSet & other);
    ~FolderSet();

    ImageFolder * current();
    ImageFolder * next();
    ImageFolder * previous();
    ImageFolder * begin();

    QStringList paths();

    void rememberCurrent();
    void restoreMemory();
    void clear();
    void append(ImageFolder image_folder);
    void removeCurrent();
    void setFolders(QList<ImageFolder> list);
    const QList<ImageFolder> &folders() const;
    int size() const;
    
private:
    QList<ImageFolder> p_folders;
    int p_i;
    int p_i_memory;
};

Q_DECLARE_METATYPE(FolderSet);

QDebug operator<<(QDebug dbg, const FolderSet &folder_set);

QDataStream &operator<<(QDataStream &out, const FolderSet &folder_set);
QDataStream &operator>>(QDataStream &in, FolderSet &folder_set);

FolderSet &operator<<(FolderSet &folder_set, const ImageFolder &image_folder);

#endif // FRAMECONTAINER_H
