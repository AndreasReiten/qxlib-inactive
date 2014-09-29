#include "framecontainer.h"

ImageInfo::ImageInfo()
{
    p_selection = Selection(0,0,64,64);
    p_background = Selection(64,0,64,64);
}

ImageInfo::ImageInfo(const ImageInfo & other)
{
    p_selection = other.selection();
    p_background = other.background();
    p_path = other.path();
}

ImageInfo::~ImageInfo()
{
    ;
}

void ImageInfo::setPath(QString str)
{
    p_path = str;
}

const QString ImageInfo::path() const
{
    return p_path;
}

void ImageInfo::setSelection(Selection rect)
{
    p_selection = rect;
}

void ImageInfo::setBackground(Selection rect)
{
    p_background = rect;
}

Selection ImageInfo::selection() const
{
    return p_selection;
}

Selection ImageInfo::background() const
{
    return p_background;
}

ImageInfo& ImageInfo::operator = (ImageInfo other)
{
    p_path = other.path();
    p_selection = other.selection();
    p_background = other.background();

    return * this;
}

QDebug operator<<(QDebug dbg, const ImageInfo &image)
{
    dbg.nospace() << "Image()" << image.path() << image.selection() << image.background();
    return dbg.maybeSpace();
}

QDataStream &operator<<(QDataStream &out, const ImageInfo &image)
{
    out << image.path() << image.selection() << image.background();

    return out;
}

QDataStream &operator>>(QDataStream &in, ImageInfo &image)
{
    QString path;
    Selection selection;
    Selection background;

    in >> path >> selection >> background;
    image.setPath(path);
    image.setSelection(selection);
    image.setBackground(background);

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


QStringList ImageFolder::paths()
{
    QStringList paths;

    for (int j = 0; j < p_images.size(); j++)
    {
        paths << p_images[j].path();
    }

    return paths;
}

void ImageFolder::setImages(QList<ImageInfo> list)
{
    p_images = list;
    p_i = 0;
    p_i_memory = 0;
}

void ImageFolder::clear()
{
    p_images.clear();

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

void ImageFolder::append(ImageInfo image)
{
    p_images.append(image);
}

ImageInfo * ImageFolder::current()
{
    return &p_images[p_i];
}

ImageInfo * ImageFolder::next()
{
    if (p_i < p_images.size() - 1)
    {
        p_i++;
    }

    return &p_images[p_i];
}

ImageInfo * ImageFolder::at(int value)
{
    if (value >= p_images.size())
    {
        value = p_images.size() - 1;
    }
    if (value < 0)
    {
        p_i = 0;
    }

    p_i = value;
    
    return &p_images[p_i];
}


ImageInfo * ImageFolder::previous()
{
    if ((p_i > 0) && (p_images.size() > 0))
    {
        p_i--;
    }

    return &p_images[p_i];
}

ImageInfo * ImageFolder::begin()
{
    p_i = 0;
    return &p_images[p_i];
}

const QList<ImageInfo> & ImageFolder::images() const
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
    QList<ImageInfo> images;

    in >> path >> images;
    image_folder.setPath(path);
    image_folder.setImages(images);

    return in;
}

ImageFolder &operator<<(ImageFolder &image_folder, const ImageInfo &image)
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

ImageFolder FolderSet::onefolder()
{
    ImageFolder folder;
    
    for (int i = 0; i < p_folders.size(); i++)
    {
        for (int j = 0; j < p_folders[i].images().size(); j++)
        {
            folder << p_folders[i].images()[j];
        }
    }

    return folder;
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
