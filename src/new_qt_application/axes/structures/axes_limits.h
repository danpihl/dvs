#ifndef MAIN_APPLICATION_AXES_STRUCTURES_AXES_LIMITS_H_
#define MAIN_APPLICATION_AXES_STRUCTURES_AXES_LIMITS_H_

#include "lumos/math.h"

using namespace lumos;

class AxesLimits
{
private:
    Vec3d lim_min_;
    Vec3d lim_max_;
    Vec3d tick_begin_;

public:
    AxesLimits() = default;
    AxesLimits(const Vec3d& min_vec, const Vec3d& max_vec);

    void setTickBegin(const Vec3d& tick_begin_vec);

    void setMin(const Vec3d& min_vec);
    void setMax(const Vec3d& max_vec);

    void incrementMinMax(const Vec3d& dv);

    Vec3d getMin() const;
    Vec3d getMax() const;

    Vec3d getTickBegin() const;

    Vec3d getAxesCenter() const;

    Vec3d getAxesScale() const;
};

#endif  // MAIN_APPLICATION_AXES_STRUCTURES_AXES_LIMITS_H_
