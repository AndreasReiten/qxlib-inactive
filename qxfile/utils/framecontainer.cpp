#include "framecontainer.h"

Image::Image()
{
    p_selection = QRectF(0,0,5000,5000);
}

Image::Image(const Image & other)
{
    p_selection = other.selection();
    p_path = other.path();
}

Image::~Image()
{
    ;
}

void Image::setPath(QString str)
{
    p_path = str;
}

const QString Image::path() const
{
    return p_path;
}

void Image::setSelection(QRectF rect)
{
    p_selection = rect;
}

const QRectF Image::selection() const
{
    return p_selection;
}

QDebug operator<<(QDebug dbg, const Image &image)
{
    dbg.nospace() << "Image()" << image.path();
    return dbg.maybeSpace();
}

QDataStream &operator<<(QDataStream &out, const Image &image)
{
    out << image.path() << image.selection();

    return out;
}

QDataStream &operator>>(QDataStream &in, Image &image)
{
    QString path;
    QRectF rect;

    in >> path >> rect;
    image.setPath(path);
    image.setSelection(rect);

    return in;
}


ImageFolder::ImageFolder()
{
    p_i = 0;
    p_i_memory = 0;
}
ImageFolder::ImageFolder(const ImageFolder & other)
{
    p_images = other.images();
    p_path = other.path();
    p_i = 0;
    p_i_memory = 0;
    
}
ImageFolder::~ImageFolder()
{
    ;
}

void ImageFolder::setPath(QString str)
{
    p_path = str;
}

const QString ImageFolder::path() const
{
    return p_path;
}

int ImageFolder::size() const
{
    return p_images.size();
}

int ImageFolder::i() const
{
    return p_i;
}

void ImageFolder::setImages(QList<Image> list)
{
    p_images = list;
    p_i = 0;
    p_i_memory = 0;
}

void ImageFolder::removeCurrent()
{
    p_images.removeAt(p_i);

    if (p_i > 0)
    {
        p_i--;
    }
}

void ImageFolder::rememberCurrent()
{
    p_i_memory = p_i;
}

void ImageFolder::restoreMemory()
{
    if (p_i_memory < images().size())
    {
        p_i = p_i_memory;
    }
}

void ImageFolder::append(Image image)
{
    p_images.append(image);
}

Image * ImageFolder::current()
{
    return &p_images[p_i];
}

Image * ImageFolder::next()
{
    if (p_i < p_images.size() - 1)
    {
        p_i++;
    }

    return &p_images[p_i];
}

Image * ImageFolder::previous()
{
    if ((p_i > 0) && (p_images.size() > 0))
    {
        p_i--;
    }

    return &p_images[p_i];
}

Image * ImageFolder::begin()
{
    p_i = 0;
    return &p_images[p_i];
}

const QList<Image> & ImageFolder::images() const
{
    return p_images;
}

bool ImageFolder::operator ==(const ImageFolder& other)
{
    if (this->path() == other.path()) return true;
    else return false;
}

QDebug operator<<(QDebug dbg, const ImageFolder &image_folder)
{
    dbg.nospace() << "ImageFolder()" << image_folder.path();
    return dbg.maybeSpace();
}

QDataStream &operator<<(QDataStream &out, const ImageFolder &image_folder)
{
    out << image_folder.path() << image_folder.images();

    return out;
}

QDataStream &operator>>(QDataStream &in, ImageFolder &image_folder)
{
    QString path;
    QList<Image> images;

    in >> path >> images;
    image_folder.setPath(path);
    image_folder.setImages(images);

    return in;
}

ImageFolder &operator<<(ImageFolder &image_folder, const Image &image)
{
    image_folder.append(image);

    return image_folder;
}

FolderSet::FolderSet()
{
    p_i = 0;
    p_i_memory = 0;
}
FolderSet::FolderSet(const FolderSet & other)
{
    p_folders = other.folders();
    p_i = 0;
    p_i_memory = 0;
    
}
FolderSet::~FolderSet()
{

}
ImageFolder * FolderSet::current()
{
    return &p_folders[p_i];
}
ImageFolder * FolderSet::next()
{
    if (p_i < p_folders.size() - 1)
    {
        p_i++;
    }

    return &p_folders[p_i];
}
ImageFolder * FolderSet::previous()
{
    if ((p_i > 0) && (p_folders.size() > 0))
    {
        p_i--;
    }

    return &p_folders[p_i];
}

ImageFolder * FolderSet::begin()
{
    p_i = 0;
    return &p_folders[p_i];
}

void FolderSet::setFolders(QList<ImageFolder> list)
{
    p_folders = list;
}

void FolderSet::rememberCurrent()
{
    p_i_memory = p_i;
}

void FolderSet::restoreMemory()
{
    if (p_i_memory < folders().size())
    {
        p_i = p_i_memory;
    }
}

void FolderSet::clear()
{
    p_folders.clear();

    p_i = 0;
    p_i_memory = 0;
}

void FolderSet::removeCurrent()
{
    p_folders.removeAt(p_i);

    if (p_i > 0)
    {
        p_i--;
    }
}

QStringList FolderSet::paths()
{
    QStringList paths;

    for (int i = 0; i < p_folders.size(); i++)
    {
        for (int j = 0; j < p_folders[i].images().size(); j++)
        {
            paths << p_folders[i].images()[j].path();
        }
    }

    return paths;
}

int FolderSet::size() const
{
    return p_folders.size();
}

const QList<ImageFolder> &FolderSet::folders() const
{
    return p_folders;
}

void FolderSet::append(ImageFolder image_folder)
{
    p_folders.append(image_folder);
}

QDebug operator<<(QDebug dbg, const FolderSet &folder_set)
{
    dbg.nospace() << "FolderSet()" << folder_set.size();
    return dbg.maybeSpace();
}

QDataStream &operator<<(QDataStream &out, const FolderSet &folder_set)
{
    out << folder_set.folders();

    return out;
}

QDataStream &operator>>(QDataStream &in, FolderSet &folder_set)
{
    QList<ImageFolder> folder_list;

    in >> folder_list;
    folder_set.setFolders(folder_list);

    return in;
}

FolderSet &operator<<(FolderSet &folder_set, const ImageFolder &image_folder)
{
    folder_set.append(image_folder);

    return folder_set;
}
