#include "selection.h"

Selection::Selection()
{
    p_integral = 0;
    p_weighted_x = 0;
    p_weighted_y = 0;
}

Selection::Selection(const Selection & other)
{
    this->setRect(other.x(),other.y(),other.width(),other.height());
    p_integral = other.integral();
    p_weighted_x = other.weighted_x();
    p_weighted_y = other.weighted_y();
}
Selection::~Selection()
{

}


double Selection::integral() const
{
    return p_integral;
}

double Selection::weighted_x() const
{
    return p_weighted_x;
}
double Selection::weighted_y() const
{
    return p_weighted_y;
}

void Selection::setSum(double value)
{
    p_integral = value;
}
void Selection::setWeightedX(double value)
{
    p_weighted_x = value;
}
void Selection::setWeightedY(double value)
{
    p_weighted_y = value;
}

Selection& Selection::operator = (QRect other)
{
    this->setRect(other.x(),other.y(),other.width(),other.height());

    return * this;
}

Selection& Selection::operator = (Selection other)
{
    this->setRect(other.x(),other.y(),other.width(),other.height());

    p_integral = other.integral();
    p_weighted_x = other.weighted_x();
    p_weighted_y = other.weighted_y();

    return * this;
}

QDebug operator<<(QDebug dbg, const Selection &selection)
{
    QRect tmp;
    tmp.setRect(selection.x(),selection.y(),selection.width(),selection.height());

    dbg.nospace() << "Selection()" << tmp;
    return dbg.maybeSpace();
}

QDataStream &operator<<(QDataStream &out, const Selection &selection)
{
    out << selection << selection.integral() << selection.weighted_x() << selection.weighted_y();

    return out;
}

QDataStream &operator>>(QDataStream &in, Selection &selection)
{
    Selection tmp;
    double integral;
    double weighted_x;
    double weighted_y;

    in >> tmp >> integral >> weighted_x >> weighted_y;
    selection.setSum(integral);
    selection.setWeightedX(weighted_x);
    selection.setWeightedY(weighted_y);

    return in;
}
