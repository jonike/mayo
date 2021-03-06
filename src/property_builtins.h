/****************************************************************************
** Copyright (c) 2018, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#pragma once

#include "property.h"
#include "quantity.h"
#include <QtCore/QDateTime>
#include <gp_Trsf.hxx>
#include <Quantity_Color.hxx>
#include <type_traits>

namespace Mayo {

template<typename T> struct PropertyDefault
{ static T initValue() { return T(); } };

template<> struct PropertyDefault<bool>
{ static bool initValue() { return false; } };

template<> struct PropertyDefault<int>
{ static int initValue() { return 0; } };

template<> struct PropertyDefault<double>
{ static double initValue() { return 0.; } };

template<typename T>
class GenericProperty : public Property {
public:
    using ValueType = T;

    GenericProperty(PropertyOwner* owner, const QString& label);

    const T& value() const;
    void setValue(const T& val);

    const char* dynTypeName() const override;
    static const char TypeName[];

protected:
    T m_value = PropertyDefault<T>::initValue();
};

template<typename T>
class PropertyScalarConstraints {
    static_assert(std::is_scalar<T>::value, "Requires scalar type");
public:
    using ValueType = T;

    PropertyScalarConstraints() = default;
    PropertyScalarConstraints(T minimum, T maximum, T singleStep);

    bool constraintsEnabled() const;
    void setConstraintsEnabled(bool on);

    T minimum() const;
    void setMinimum(T val);

    T maximum() const;
    void setMaximum(T val);

    void setRange(T minVal, T maxVal);

    T singleStep() const;
    void setSingleStep(T step);

private:
    T m_minimum;
    T m_maximum;
    T m_singleStep;
    bool m_constraintsEnabled = false;
};

template<typename T>
class GenericScalarProperty :
        public GenericProperty<T>,
        public PropertyScalarConstraints<T>
{
public:
    using ValueType = T;
    GenericScalarProperty(PropertyOwner* owner, const QString& label);
    GenericScalarProperty(
            PropertyOwner* owner, const QString& label,
            T minimum, T maximum, T singleStep);
};

template<Unit UNIT>
class GenericPropertyQuantity : public Property {
public:
    using QuantityType = Quantity<UNIT>;

    GenericPropertyQuantity(PropertyOwner* owner, const QString& label);

    const QuantityType& quantity() const;
    void setQuantity(const QuantityType& qty);

    const char* dynTypeName() const override;
    static const char TypeName[];

private:
    QuantityType m_quantity;
};

using PropertyBool = GenericProperty<bool>;
using PropertyInt = GenericScalarProperty<int>;
using PropertyDouble = GenericScalarProperty<double>;
using PropertyQByteArray = GenericProperty<QByteArray>;
using PropertyQString = GenericProperty<QString>;
using PropertyQDateTime = GenericProperty<QDateTime>;
using PropertyOccColor = GenericProperty<Quantity_Color>;
using PropertyOccPnt = GenericProperty<gp_Pnt>;
using PropertyOccTrsf = GenericProperty<gp_Trsf>;

using PropertyLength = GenericPropertyQuantity<Unit::Length>;
using PropertyArea = GenericPropertyQuantity<Unit::Area>;
using PropertyVolume = GenericPropertyQuantity<Unit::Volume>;
using PropertyMass = GenericPropertyQuantity<Unit::Mass>;
using PropertyTime = GenericPropertyQuantity<Unit::Time>;
using PropertyAngle = GenericPropertyQuantity<Unit::Angle>;
using PropertyVelocity = GenericPropertyQuantity<Unit::Velocity>;

// --
// -- Implementation
// --

// GenericProperty<>

template<typename T>
GenericProperty<T>::GenericProperty(PropertyOwner* owner, const QString& label)
    : Property(owner, label)
{ }

template<typename T> const T& GenericProperty<T>::value() const
{ return m_value; }

template<typename T> void GenericProperty<T>::setValue(const T& val)
{
    m_value = val;
    this->notifyChanged();
}

template<typename T> const char* GenericProperty<T>::dynTypeName() const
{ return TypeName; }

const char PropertyBool::TypeName[] = "Mayo::PropertyBool";
const char PropertyInt::TypeName[] = "Mayo::PropertyInt";
const char PropertyDouble::TypeName[] = "Mayo::PropertyDouble";
const char PropertyQByteArray::TypeName[] = "Mayo::PropertyQByteArray";
const char PropertyQString::TypeName[] = "Mayo::PropertyQString";
const char PropertyQDateTime::TypeName[] = "Mayo::PropertyQDateTime";
const char PropertyOccColor::TypeName[] = "Mayo::PropertyOccColor";
const char PropertyOccPnt::TypeName[] = "Mayo::PropertyOccPnt";
const char PropertyOccTrsf::TypeName[] = "Mayo::PropertyOccTrsf";

// PropertyScalarConstraints<>

template<typename T>
PropertyScalarConstraints<T>::PropertyScalarConstraints(
        T minimum, T maximum, T singleStep)
    : m_minimum(minimum),
      m_maximum(maximum),
      m_singleStep(singleStep),
      m_constraintsEnabled(true)
{ }

template<typename T> bool PropertyScalarConstraints<T>::constraintsEnabled() const
{ return m_constraintsEnabled; }

template<typename T> void PropertyScalarConstraints<T>::setConstraintsEnabled(bool on)
{ m_constraintsEnabled = on; }

template<typename T> T PropertyScalarConstraints<T>::minimum() const
{ return m_minimum; }

template<typename T> void PropertyScalarConstraints<T>::setMinimum(T val)
{ m_minimum = val; }

template<typename T> T PropertyScalarConstraints<T>::maximum() const
{ return m_maximum; }

template<typename T> void PropertyScalarConstraints<T>::setMaximum(T val)
{ m_maximum = val; }

template<typename T>
void PropertyScalarConstraints<T>::setRange(T minVal, T maxVal)
{
    this->setMinimum(minVal);
    this->setMaximum(maxVal);
}

template<typename T> T PropertyScalarConstraints<T>::singleStep() const
{ return m_singleStep; }

template<typename T> void PropertyScalarConstraints<T>::setSingleStep(T step)
{ m_singleStep = step; }

// GenericScalarProperty<>

template<typename T>
GenericScalarProperty<T>::GenericScalarProperty(
        PropertyOwner* owner, const QString& label)
    : GenericProperty(owner, label)
{ }

template<typename T>
GenericScalarProperty<T>::GenericScalarProperty(
            PropertyOwner* owner, const QString& label,
            T minimum, T maximum, T singleStep)
    : GenericProperty(owner, label),
      PropertyScalarConstraints(minimum, maximum, singleStep)
{ }

// GenericPropertyQuantity<>

template<Unit UNIT>
GenericPropertyQuantity<UNIT>::GenericPropertyQuantity(
        PropertyOwner* owner, const QString& label)
    : Property(owner, label)
{ }

template<Unit UNIT>
const Quantity<UNIT>& GenericPropertyQuantity<UNIT>::quantity() const
{ return m_quantity; }

template<Unit UNIT>
void GenericPropertyQuantity<UNIT>::setQuantity(const Quantity<UNIT>& qty)
{
    m_quantity = qty;
    this->notifyChanged();
}

template<Unit UNIT>
const char* GenericPropertyQuantity<UNIT>::dynTypeName() const
{ return TypeName; }

const char PropertyLength::TypeName[] = "Mayo::PropertyLength";
const char PropertyArea::TypeName[] = "Mayo::PropertyArea";
const char PropertyVolume::TypeName[] = "Mayo::PropertyVolume";
const char PropertyMass::TypeName[] = "Mayo::PropertyMass";
const char PropertyTime::TypeName[] = "Mayo::PropertyTime";
const char PropertyAngle::TypeName[] = "Mayo::PropertyAngle";
const char PropertyVelocity::TypeName[] = "Mayo::PropertyVelocity";

} // namespace Mayo
