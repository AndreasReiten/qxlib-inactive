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


ImageSeries::ImageSeries()
{
    p_i = 0;
    p_i_memory = 0;
}
ImageSeries::ImageSeries(const ImageSeries & other)
{
    p_images = other.images();
    p_path = other.path();
    p_i = 0;
    p_i_memory = 0;
    
}
ImageSeries::~ImageSeries()
{
    ;
}

void ImageSeries::setPath(QString str)
{
    p_path = str;
}

const QString ImageSeries::path() const
{
    return p_path;
}

int ImageSeries::size() const
{
    return p_images.size();
}

int ImageSeries::i() const
{
    return p_i;
}


QStringList ImageSeries::paths()
{
    QStringList paths;

    for (int j = 0; j < p_images.size(); j++)
    {
        paths << p_images[j].path();
    }

    return paths;
}

void ImageSeries::setImages(QList<ImageInfo> list)
{
    p_images = list;
    p_i = 0;
    p_i_memory = 0;
}

void ImageSeries::clear()
{
    p_images.clear();

    p_i = 0;
    p_i_memory = 0;
}

void ImageSeries::removeCurrent()
{
    p_images.removeAt(p_i);

    if (p_i > 0)
    {
        p_i--;
    }
}

void ImageSeries::rememberCurrent()
{
    p_i_memory = p_i;
}

void ImageSeries::restoreMemory()
{
    if (p_i_memory < images().size())
    {
        p_i = p_i_memory;
    }
}

void ImageSeries::append(ImageInfo image)
{
    p_images.append(image);
}

ImageInfo * ImageSeries::current()
{
    return &p_images[p_i];
}

ImageInfo * ImageSeries::next()
{
    if (p_i < p_images.size() - 1)
    {
        p_i++;
    }

    return &p_images[p_i];
}

ImageInfo * ImageSeries::at(int value)
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


ImageInfo * ImageSeries::previous()
{
    if ((p_i > 0) && (p_images.size() > 0))
    {
        p_i--;
    }

    return &p_images[p_i];
}

ImageInfo * ImageSeries::begin()
{
    p_i = 0;
    return &p_images[p_i];
}

const QList<ImageInfo> & ImageSeries::images() const
{
    return p_images;
}

bool ImageSeries::operator ==(const ImageSeries& other)
{
    if (this->path() == other.path()) return true;
    else return false;
}

QDebug operator<<(QDebug dbg, const ImageSeries &image_series)
{
    dbg.nospace() << "ImageFolder()" << image_series.path();
    return dbg.maybeSpace();
}

QDataStream &operator<<(QDataStream &out, const ImageSeries &image_series)
{
    out << image_series.path() << image_series.images();

    return out;
}

QDataStream &operator>>(QDataStream &in, ImageSeries &image_series)
{
    QString path;
    QList<ImageInfo> images;

    in >> path >> images;
    image_series.setPath(path);
    image_series.setImages(images);

    return in;
}

ImageSeries &operator<<(ImageSeries &image_series, const ImageInfo &image)
{
    image_series.append(image);

    return image_series;
}

SeriesSet::SeriesSet()
{
    p_i = 0;
    p_i_memory = 0;
}
SeriesSet::SeriesSet(const SeriesSet & other)
{
    p_seriess = other.seriess();
    p_i = 0;
    p_i_memory = 0;
    
}
SeriesSet::~SeriesSet()
{

}
ImageSeries * SeriesSet::current()
{
    return &p_seriess[p_i];
}
ImageSeries * SeriesSet::next()
{
    if (p_i < p_seriess.size() - 1)
    {
        p_i++;
    }

    return &p_seriess[p_i];
}
ImageSeries * SeriesSet::previous()
{
    if ((p_i > 0) && (p_seriess.size() > 0))
    {
        p_i--;
    }

    return &p_seriess[p_i];
}

ImageSeries * SeriesSet::begin()
{
    p_i = 0;
    return &p_seriess[p_i];
}

void SeriesSet::setFolders(QList<ImageSeries> list)
{
    p_seriess = list;
}

bool SeriesSet::isEmpty()
{
    for (int i = 0; i < p_seriess.size(); i++)
    {
        if (p_seriess[i].size()) return false;
    }

    return true;
}

void SeriesSet::rememberCurrent()
{
    p_i_memory = p_i;
}

void SeriesSet::restoreMemory()
{
    if (p_i_memory < seriess().size())
    {
        p_i = p_i_memory;
    }
}

void SeriesSet::clear()
{
    p_seriess.clear();

    p_i = 0;
    p_i_memory = 0;
}

void SeriesSet::removeCurrent()
{
    p_seriess.removeAt(p_i);

    if (p_i > 0)
    {
        p_i--;
    }
}

QStringList SeriesSet::paths()
{
    QStringList paths;

    for (int i = 0; i < p_seriess.size(); i++)
    {
        for (int j = 0; j < p_seriess[i].images().size(); j++)
        {
            paths << p_seriess[i].images()[j].path();
        }
    }

    return paths;
}

ImageSeries SeriesSet::oneSeries()
{
    ImageSeries series;
    
    for (int i = 0; i < p_seriess.size(); i++)
    {
        for (int j = 0; j < p_seriess[i].images().size(); j++)
        {
            series << p_seriess[i].images()[j];
        }
    }

    return series;
}


int SeriesSet::size() const
{
    return p_seriess.size();
}

const QList<ImageSeries> &SeriesSet::seriess() const
{
    return p_seriess;
}

void SeriesSet::append(ImageSeries image_series)
{
    p_seriess.append(image_series);
}

QDebug operator<<(QDebug dbg, const SeriesSet &series_set)
{
    dbg.nospace() << "FolderSet()" << series_set.size();
    return dbg.maybeSpace();
}

QDataStream &operator<<(QDataStream &out, const SeriesSet &series_set)
{
    out << series_set.seriess();

    return out;
}

QDataStream &operator>>(QDataStream &in, SeriesSet &series_set)
{
    QList<ImageSeries> series_list;

    in >> series_list;
    series_set.setFolders(series_list);

    return in;
}

SeriesSet &operator<<(SeriesSet &series_set, const ImageSeries &image_series)
{
    series_set.append(image_series);

    return series_set;
}
