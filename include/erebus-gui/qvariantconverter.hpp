#pragma once

#include <erebus-gui/erebus-gui.hpp>

#include <QVariant>

namespace Erc
{
 
template <typename T>
class QVariantConverter;


template <typename T>
class QVariantConverterBase
{
public:
    static QVariant convert(const T& v) { return QVariant(v); }
};


template <>
class QVariantConverter<bool>
    : public QVariantConverterBase<bool>
{
public:
    static bool convert(const QVariant& v) { return v.toBool(); }
};

template <>
class QVariantConverter<int>
    : public QVariantConverterBase<int>
{
public:
    static int convert(const QVariant& v) { return v.toInt(); }
};

template <>
class QVariantConverter<unsigned>
    : public QVariantConverterBase<unsigned>
{
public:
    static unsigned int convert(const QVariant& v) { return v.toUInt(); }
};

template <>
class QVariantConverter<long long>
    : public QVariantConverterBase<long long>
{
public:
    static long long convert(const QVariant& v) { return v.toLongLong(); }
};

template <>
class QVariantConverter<unsigned long long>
    : public QVariantConverterBase<unsigned long long>
{
public:
    static unsigned long long convert(const QVariant& v) { return v.toULongLong(); }
};

template <>
class QVariantConverter<QString>
    : public QVariantConverterBase<QString>
{
public:
    static QString convert(const QVariant& v) { return v.toString(); }
};

template <>
class QVariantConverter<QStringList>
    : public QVariantConverterBase<QStringList>
{
public:
    static QStringList convert(const QVariant& v) { return v.toStringList(); }
};

template <>
class QVariantConverter<QByteArray>
    : public QVariantConverterBase<QByteArray>
{
public:
    static QByteArray convert(const QVariant& v) { return v.toByteArray(); }
};
 
    
} // namespace Erc {}
