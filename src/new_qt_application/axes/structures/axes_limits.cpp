#include "axes/structures/axes_limits.h"

#include "lumos/math.h"

AxesLimits::AxesLimits(const Vec3d& min_vec, const Vec3d& max_vec)
{
    lim_min_ = min_vec;
    tick_begin_ = min_vec;
    lim_max_ = max_vec;
}

void AxesLimits::setTickBegin(const Vec3d& tick_begin_vec)
{
    tick_begin_ = tick_begin_vec;
}

void AxesLimits::setMin(const Vec3d& min_vec)
{
    lim_min_ = min_vec;
}

void AxesLimits::setMax(const Vec3d& max_vec)
{
    lim_max_ = max_vec;
}

void AxesLimits::incrementMinMax(const Vec3d& dv)
{
    lim_min_ = lim_min_ + dv;
    lim_max_ = lim_max_ + dv;
}

Vec3d AxesLimits::getMin() const
{
    return lim_min_;
}

Vec3d AxesLimits::getMax() const
{
    return lim_max_;
}

Vec3d AxesLimits::getTickBegin() const
{
    return tick_begin_;
}

Vec3d AxesLimits::getAxesCenter() const
{
    return (getMin() + getMax()) / 2.0;
}

Vec3d AxesLimits::getAxesScale() const
{
    return getMax() - getMin();
}
